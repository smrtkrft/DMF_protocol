#include "mail_functions.h"

#include <LittleFS.h>
#include <base64.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

void MailAgent::begin(ConfigStore *storePtr, DMFNetworkManager *netMgrPtr, const String &deviceIdStr) {
    store = storePtr;
    netManager = netMgrPtr;
    deviceId = deviceIdStr;
    if (store) {
        settings = store->loadMailSettings();
    }
}

void MailAgent::updateConfig(const MailSettings &config) {
    settings = config;
    if (store) {
        store->saveMailSettings(settings);
    }
}

String MailAgent::base64Encode(const String &input) {
    return base64::encode(input);
}

String MailAgent::smtpReadLine(WiFiClientSecure &client, uint32_t timeoutMs) {
    String line = "";
    unsigned long start = millis();
    
    while (millis() - start < timeoutMs) {
        if (client.available()) {
            char c = client.read();
            if (c == '\n') break;
            if (c != '\r') line += c;
        }
        delay(1);
    }
    
    return line;
}

bool MailAgent::smtpConnect(WiFiClientSecure &client, String &errorMessage) {
    Serial.printf("[SMTP] Connecting: %s:%d\n", settings.smtpServer.c_str(), settings.smtpPort);
    
    client.setInsecure();
    client.setTimeout(15);
    
    if (settings.smtpPort == 587) {
        Serial.println(F("[SMTP] Port 587 (STARTTLS) not supported. Use 465!"));
        errorMessage = "Port 587 not supported. Use port 465";
        return false;
    }
    
    IPAddress serverIP;
    if (!WiFi.hostByName(settings.smtpServer.c_str(), serverIP)) {
        errorMessage = "DNS failed: " + settings.smtpServer;
        Serial.println(F("[SMTP] DNS resolution failed"));
        return false;
    }
    
    if (!client.connect(settings.smtpServer.c_str(), settings.smtpPort)) {
        errorMessage = "Connection failed";
        Serial.println(F("[SMTP] Connection failed"));
        return false;
    }
    
    String response = smtpReadLine(client);
    Serial.printf("[SMTP] << %s\n", response.c_str());
    
    if (!response.startsWith("220")) {
        errorMessage = "Server greeting failed";
        client.stop();
        return false;
    }
    
    Serial.println(F("[SMTP] Connected"));
    return true;
}

bool MailAgent::smtpAuth(WiFiClientSecure &client, String &errorMessage) {
    Serial.println(F("[SMTP] Kimlik doğrulaması yapılıyor..."));
    
    // EHLO gönder
    String ehloCmd = "EHLO " + String(WiFi.getHostname()) + "\r\n";
    client.print(ehloCmd);
    
    // EHLO yanıtlarını oku (250 ile başlayan satırlar)
    bool foundAuth = false;
    for (int i = 0; i < 10; i++) {
        String response = smtpReadLine(client);
        Serial.printf("[SMTP] << %s\n", response.c_str());
        if (response.indexOf("AUTH") >= 0) foundAuth = true;
        if (response.startsWith("250 ")) break; // Son satır
    }
    
    if (!foundAuth) {
        errorMessage = "SMTP AUTH desteklenmiyor";
        return false;
    }
    
    // AUTH LOGIN
    client.print("AUTH LOGIN\r\n");
    String response = smtpReadLine(client);
    Serial.printf("[SMTP] << %s\n", response.c_str());
    
    if (!response.startsWith("334")) {
        errorMessage = "AUTH LOGIN reddedildi";
        return false;
    }
    
    // Kullanıcı adı (base64)
    client.println(base64Encode(settings.username));
    response = smtpReadLine(client);
    Serial.printf("[SMTP] << %s\n", response.c_str());
    
    if (!response.startsWith("334")) {
        errorMessage = "Kullanıcı adı reddedildi";
        return false;
    }
    
    // Şifre (base64)
    client.println(base64Encode(settings.password));
    response = smtpReadLine(client);
    Serial.printf("[SMTP] << %s\n", response.c_str());
    
    if (!response.startsWith("235")) {
        errorMessage = "Kimlik doğrulama başarısız - Şifre yanlış";
        return false;
    }
    
    Serial.println(F("[SMTP] Kimlik doğrulama başarılı"));
    return true;
}

bool MailAgent::sendWarning(uint8_t alarmIndex, const ScheduleSnapshot &snapshot, String &errorMessage) {
    String subject = settings.warning.subject;
    // Yeni format: {DEVICE_ID}, {TIMESTAMP}, {REMAINING}
    subject.replace("{DEVICE_ID}", deviceId);
    subject.replace("{TIMESTAMP}", formatHeader());
    subject.replace("{REMAINING}", formatElapsed(snapshot));
    // Geriye uyumluluk için eski format da desteklenir
    subject.replace("%ALARM_INDEX%", String(alarmIndex + 1));
    subject.replace("%TOTAL_ALARMS%", String(snapshot.totalAlarms));
    subject.replace("%REMAINING%", formatElapsed(snapshot));

    String body = settings.warning.body;
    body.replace("{DEVICE_ID}", deviceId);
    body.replace("{TIMESTAMP}", formatHeader());
    body.replace("{REMAINING}", formatElapsed(snapshot));
    body.replace("%ALARM_INDEX%", String(alarmIndex + 1));
    body.replace("%TOTAL_ALARMS%", String(snapshot.totalAlarms));
    body.replace("%REMAINING%", formatElapsed(snapshot));

    bool mailSuccess = sendEmail(subject, body, true, errorMessage);
    
    // URL tetikleme (mail başarısız olsa bile çalıştır - NON-BLOCKING)
    if (settings.warning.getUrl.length() > 0 && WiFi.status() == WL_CONNECTED) {
        Serial.printf("[Warning URL] Tetikleniyor: %s\n", settings.warning.getUrl.c_str());
        
        // Task oluştur - main loop'u bloke etmez
        xTaskCreate([](void* param) {
            String url = *((String*)param);
            
            // Kısa delay - mail client'ın cleanup olması için
            vTaskDelay(pdMS_TO_TICKS(500));
            
            HTTPClient http;
            
            // HTTP veya HTTPS'e göre client seç
            if (url.startsWith("https://")) {
                WiFiClientSecure* client = new WiFiClientSecure();
                client->setInsecure();
                
                if (http.begin(*client, url)) {
                    http.setTimeout(8000);
                    http.setConnectTimeout(3000);
                    
                    int httpCode = http.GET();
                    Serial.printf("[Warning URL] Sonuç: %d\n", httpCode);
                    
                    if (httpCode > 0) {
                        String response = http.getString();
                        Serial.printf("[Warning URL] Yanıt: %d bytes\n", response.length());
                        if (response.length() < 150) {
                            Serial.printf("[Warning URL] %s\n", response.c_str());
                        }
                    } else {
                        Serial.printf("[Warning URL] HATA: %s\n", http.errorToString(httpCode).c_str());
                    }
                    http.end();
                }
                delete client;
            } else {
                WiFiClient* client = new WiFiClient();
                
                if (http.begin(*client, url)) {
                    http.setTimeout(8000);
                    http.setConnectTimeout(3000);
                    
                    int httpCode = http.GET();
                    Serial.printf("[Warning URL] Sonuç: %d\n", httpCode);
                    
                    if (httpCode > 0) {
                        String response = http.getString();
                        Serial.printf("[Warning URL] Yanıt: %d bytes\n", response.length());
                        if (response.length() < 150) {
                            Serial.printf("[Warning URL] %s\n", response.c_str());
                        }
                    } else {
                        Serial.printf("[Warning URL] HATA: %s\n", http.errorToString(httpCode).c_str());
                    }
                    http.end();
                }
                delete client;
            }
            
            delete (String*)param;
            vTaskDelete(NULL);
        }, "WarningURLTask", 8192, new String(settings.warning.getUrl), 1, NULL);
        
        Serial.println(F("[Warning URL] Task başlatıldı (non-blocking)"));
    } else {
        if (settings.warning.getUrl.length() == 0) {
            Serial.println(F("[Warning URL] URL tanımlanmamış, tetiklenmedi"));
        } else {
            Serial.println(F("[Warning URL] ATLANDΙ - WiFi bağlantısı yok"));
        }
    }
    
    return mailSuccess;
}

bool MailAgent::sendFinal(const ScheduleSnapshot &snapshot, String &errorMessage) {
    String subject = settings.finalContent.subject;
    subject.replace("{DEVICE_ID}", deviceId);
    subject.replace("{TIMESTAMP}", formatHeader());
    subject.replace("{REMAINING}", "0");
    subject.replace("%REMAINING%", "0");

    String body = settings.finalContent.body;
    body.replace("{DEVICE_ID}", deviceId);
    body.replace("{TIMESTAMP}", formatHeader());
    body.replace("{REMAINING}", "0");
    body.replace("%REMAINING%", "0");

    // Final mail: Final attachments gönder (warning=false)
    bool mailSuccess = sendEmail(subject, body, false, errorMessage);
    
    // URL tetikleme (mail başarısız olsa bile çalıştır - NON-BLOCKING)
    if (settings.finalContent.getUrl.length() > 0 && WiFi.status() == WL_CONNECTED) {
        Serial.printf("[Final URL] Tetikleniyor: %s\n", settings.finalContent.getUrl.c_str());
        
        // Task oluştur - main loop'u bloke etmez
        xTaskCreate([](void* param) {
            String url = *((String*)param);
            
            // Kısa delay - mail client'ın cleanup olması için
            vTaskDelay(pdMS_TO_TICKS(500));
            
            HTTPClient http;
            
            // HTTP veya HTTPS'e göre client seç
            if (url.startsWith("https://")) {
                WiFiClientSecure* client = new WiFiClientSecure();
                client->setInsecure();
                
                if (http.begin(*client, url)) {
                    http.setTimeout(8000);
                    http.setConnectTimeout(3000);
                    
                    int httpCode = http.GET();
                    Serial.printf("[Final URL] Sonuç: %d\n", httpCode);
                    
                    if (httpCode > 0) {
                        String response = http.getString();
                        Serial.printf("[Final URL] Yanıt: %d bytes\n", response.length());
                        if (response.length() < 150) {
                            Serial.printf("[Final URL] %s\n", response.c_str());
                        }
                    } else {
                        Serial.printf("[Final URL] HATA: %s\n", http.errorToString(httpCode).c_str());
                    }
                    http.end();
                }
                delete client;
            } else {
                WiFiClient* client = new WiFiClient();
                
                if (http.begin(*client, url)) {
                    http.setTimeout(8000);
                    http.setConnectTimeout(3000);
                    
                    int httpCode = http.GET();
                    Serial.printf("[Final URL] Sonuç: %d\n", httpCode);
                    
                    if (httpCode > 0) {
                        String response = http.getString();
                        Serial.printf("[Final URL] Yanıt: %d bytes\n", response.length());
                        if (response.length() < 150) {
                            Serial.printf("[Final URL] %s\n", response.c_str());
                        }
                    } else {
                        Serial.printf("[Final URL] HATA: %s\n", http.errorToString(httpCode).c_str());
                    }
                    http.end();
                }
                delete client;
            }
            
            delete (String*)param;
            vTaskDelete(NULL);
        }, "FinalURLTask", 8192, new String(settings.finalContent.getUrl), 1, NULL);
        
        Serial.println(F("[Final URL] Task başlatıldı (non-blocking)"));
    } else {
        if (settings.finalContent.getUrl.length() == 0) {
            Serial.println(F("[Final URL] URL tanımlanmamış, tetiklenmedi"));
        } else {
            Serial.println(F("[Final URL] ATLANDΙ - WiFi bağlantısı yok"));
        }
    }
    
    return mailSuccess;
}

// TEST FONKSIYONLARI - Sadece gönderen adrese mail atar
bool MailAgent::sendWarningTest(const ScheduleSnapshot &snapshot, String &errorMessage) {
    String subject = "[TEST UYARI] " + settings.warning.subject;
    subject.replace("{DEVICE_ID}", deviceId);
    subject.replace("{TIMESTAMP}", formatHeader());
    subject.replace("{REMAINING}", formatElapsed(snapshot));
    subject.replace("%ALARM_INDEX%", "1");
    subject.replace("%TOTAL_ALARMS%", String(snapshot.totalAlarms));
    subject.replace("%REMAINING%", formatElapsed(snapshot));

    String body = settings.warning.body;
    body.replace("{DEVICE_ID}", deviceId);
    body.replace("{TIMESTAMP}", formatHeader());
    body.replace("{REMAINING}", formatElapsed(snapshot));
    body.replace("%ALARM_INDEX%", "1");
    body.replace("%TOTAL_ALARMS%", String(snapshot.totalAlarms));
    body.replace("%REMAINING%", formatElapsed(snapshot));

    // SMTP kullanıcı adına (kendine) gönder - Warning test
    bool mailSuccess = sendEmailToSelf(subject, body, true, errorMessage);
    Serial.printf("[MAIL TEST] Warning mail gönderimi: %s\n", mailSuccess ? "BAŞARILI" : "BAŞARISIZ");
    
    // URL tetikleme (Warning test - NON-BLOCKING)
    if (settings.warning.getUrl.length() > 0 && WiFi.status() == WL_CONNECTED) {
        Serial.printf("[TEST Warning URL] Tetikleniyor: %s\n", settings.warning.getUrl.c_str());
        
        // Task oluştur - main loop'u bloke etmez
        xTaskCreate([](void* param) {
            String url = *((String*)param);
            
            // Kısa delay - mail client'ın cleanup olması için
            vTaskDelay(pdMS_TO_TICKS(500));
            
            HTTPClient http;
            
            // HTTP veya HTTPS'e göre client seç
            if (url.startsWith("https://")) {
                WiFiClientSecure* client = new WiFiClientSecure();
                client->setInsecure();
                
                if (http.begin(*client, url)) {
                    http.setTimeout(8000);
                    http.setConnectTimeout(3000);
                    
                    int httpCode = http.GET();
                    Serial.printf("[TEST Warning URL] Sonuç: %d\n", httpCode);
                    
                    if (httpCode > 0) {
                        String response = http.getString();
                        Serial.printf("[TEST Warning URL] Yanıt: %d bytes\n", response.length());
                        if (response.length() < 150) {
                            Serial.printf("[TEST Warning URL] %s\n", response.c_str());
                        }
                    } else {
                        Serial.printf("[TEST Warning URL] HATA: %s\n", http.errorToString(httpCode).c_str());
                    }
                    http.end();
                }
                delete client;
            } else {
                WiFiClient* client = new WiFiClient();
                
                if (http.begin(*client, url)) {
                    http.setTimeout(8000);
                    http.setConnectTimeout(3000);
                    
                    int httpCode = http.GET();
                    Serial.printf("[TEST Warning URL] Sonuç: %d\n", httpCode);
                    
                    if (httpCode > 0) {
                        String response = http.getString();
                        Serial.printf("[TEST Warning URL] Yanıt: %d bytes\n", response.length());
                        if (response.length() < 150) {
                            Serial.printf("[TEST Warning URL] %s\n", response.c_str());
                        }
                    } else {
                        Serial.printf("[TEST Warning URL] HATA: %s\n", http.errorToString(httpCode).c_str());
                    }
                    http.end();
                }
                delete client;
            }
            
            delete (String*)param;
            vTaskDelete(NULL);
        }, "WarningURLTask", 8192, new String(settings.warning.getUrl), 1, NULL);
        
        Serial.println(F("[TEST Warning URL] Task başlatıldı (non-blocking)"));
    } else {
        if (settings.warning.getUrl.length() == 0) {
            Serial.println(F("[TEST Warning URL] ATLANDΙ - URL boş"));
        } else {
            Serial.println(F("[TEST Warning URL] ATLANDΙ - WiFi bağlantısı yok"));
        }
    }
    
    return mailSuccess;
}

bool MailAgent::sendFinalTest(const ScheduleSnapshot &snapshot, String &errorMessage) {
    String subject = "[TEST DMF] " + settings.finalContent.subject;
    subject.replace("{DEVICE_ID}", deviceId);
    subject.replace("{TIMESTAMP}", formatHeader());
    subject.replace("{REMAINING}", "0");
    subject.replace("%REMAINING%", "0");

    String body = settings.finalContent.body;
    body.replace("{DEVICE_ID}", deviceId);
    body.replace("{TIMESTAMP}", formatHeader());
    body.replace("{REMAINING}", "0");
    body.replace("%REMAINING%", "0");

    // DMF test: MAİL LİSTESİNE gönder + Final attachments (warning=false)
    bool mailSuccess = sendEmail(subject, body, false, errorMessage);
    Serial.printf("[MAIL TEST] Final/DMF mail gönderimi (listeye + Final attachments): %s\n", mailSuccess ? "BAŞARILI" : "BAŞARISIZ");
    
    // URL tetikleme (Final test - NON-BLOCKING)
    if (settings.finalContent.getUrl.length() > 0 && WiFi.status() == WL_CONNECTED) {
        Serial.printf("[TEST Final URL] Tetikleniyor: %s\n", settings.finalContent.getUrl.c_str());
        
        // Task oluştur - main loop'u bloke etmez
        xTaskCreate([](void* param) {
            String url = *((String*)param);
            
            // Kısa delay - mail client'ın cleanup olması için
            vTaskDelay(pdMS_TO_TICKS(500));
            
            HTTPClient http;
            
            // HTTP veya HTTPS'e göre client seç
            if (url.startsWith("https://")) {
                WiFiClientSecure* client = new WiFiClientSecure();
                client->setInsecure();
                
                if (http.begin(*client, url)) {
                    http.setTimeout(8000);
                    http.setConnectTimeout(3000);
                    
                    int httpCode = http.GET();
                    Serial.printf("[TEST Final URL] Sonuç: %d\n", httpCode);
                    
                    if (httpCode > 0) {
                        String response = http.getString();
                        Serial.printf("[TEST Final URL] Yanıt: %d bytes\n", response.length());
                        if (response.length() < 150) {
                            Serial.printf("[TEST Final URL] %s\n", response.c_str());
                        }
                    } else {
                        Serial.printf("[TEST Final URL] HATA: %s\n", http.errorToString(httpCode).c_str());
                    }
                    http.end();
                }
                delete client;
            } else {
                WiFiClient* client = new WiFiClient();
                
                if (http.begin(*client, url)) {
                    http.setTimeout(8000);
                    http.setConnectTimeout(3000);
                    
                    int httpCode = http.GET();
                    Serial.printf("[TEST Final URL] Sonuç: %d\n", httpCode);
                    
                    if (httpCode > 0) {
                        String response = http.getString();
                        Serial.printf("[TEST Final URL] Yanıt: %d bytes\n", response.length());
                        if (response.length() < 150) {
                            Serial.printf("[TEST Final URL] %s\n", response.c_str());
                        }
                    } else {
                        Serial.printf("[TEST Final URL] HATA: %s\n", http.errorToString(httpCode).c_str());
                    }
                    http.end();
                }
                delete client;
            }
            
            delete (String*)param;
            vTaskDelete(NULL);
        }, "FinalURLTask", 8192, new String(settings.finalContent.getUrl), 1, NULL);
        
        Serial.println(F("[TEST Final URL] Task başlatıldı (non-blocking)"));
    } else {
        if (settings.finalContent.getUrl.length() == 0) {
            Serial.println(F("[TEST Final URL] ATLANDΙ - URL boş"));
        } else {
            Serial.println(F("[TEST Final URL] ATLANDΙ - WiFi bağlantısı yok"));
        }
    }
    
    return mailSuccess;
}

bool MailAgent::smtpSendMail(WiFiClientSecure &client, const String &subject, const String &body, bool includeAttachments, String &errorMessage) {
    // Debug: Mail listesini yazdır
    Serial.printf("[SMTP DEBUG] Recipient count: %d\n", settings.recipientCount);
    for (uint8_t i = 0; i < settings.recipientCount; ++i) {
        Serial.printf("[SMTP DEBUG] Recipient[%d]: %s\n", i, settings.recipients[i].c_str());
    }
    Serial.printf("[SMTP DEBUG] Subject: %s\n", subject.c_str());
    Serial.printf("[SMTP DEBUG] Body (first 100 chars): %.100s\n", body.c_str());
    
    // MAIL FROM
    String mailFrom = "MAIL FROM:<" + settings.username + ">\r\n";
    client.print(mailFrom);
    String response = smtpReadLine(client);
    Serial.printf("[SMTP] << %s\n", response.c_str());
    
    if (!response.startsWith("250")) {
        errorMessage = "MAIL FROM reddedildi";
        return false;
    }
    
    // RCPT TO (her alıcı için)
    for (uint8_t i = 0; i < settings.recipientCount; ++i) {
        if (settings.recipients[i].length() == 0) continue;
        
        String rcptTo = "RCPT TO:<" + settings.recipients[i] + ">\r\n";
        client.print(rcptTo);
        response = smtpReadLine(client);
        Serial.printf("[SMTP] << %s\n", response.c_str());
        
        if (!response.startsWith("250")) {
            errorMessage = "Alıcı reddedildi: " + settings.recipients[i];
            return false;
        }
    }
    
    // DATA
    client.print("DATA\r\n");
    response = smtpReadLine(client);
    Serial.printf("[SMTP] << %s\n", response.c_str());
    
    if (!response.startsWith("354")) {
        errorMessage = "DATA komutu reddedildi";
        return false;
    }
    
    // MIME mesajı oluştur ve gönder
    String mime = buildMimeMessage(subject, body, includeAttachments);
    client.print(mime);
    client.print("\r\n.\r\n");
    
    response = smtpReadLine(client);
    Serial.printf("[SMTP] << %s\n", response.c_str());
    
    if (!response.startsWith("250")) {
        errorMessage = "Mail gönderimi başarısız";
        return false;
    }
    
    Serial.println(F("[SMTP] Mail başarıyla gönderildi"));
    return true;
}

bool MailAgent::smtpCommand(WiFiClientSecure &client, const String &command, const String &expectCode, String &errorMessage) {
    if (command.length() > 0) {
        client.print(command);
    }
    
    String response = smtpReadLine(client);
    Serial.printf("[SMTP] << %s\n", response.c_str());
    
    if (!response.startsWith(expectCode)) {
        errorMessage = "Beklenmeyen yanıt: " + response;
        return false;
    }
    
    return true;
}

bool MailAgent::sendEmail(const String &subject, const String &body, bool includeWarningAttachments, String &errorMessage) {
    if (settings.recipientCount == 0) {
        errorMessage = "Mail listesi boş";
        return false;
    }

    if (settings.smtpServer.length() == 0 || settings.username.length() == 0) {
        errorMessage = "SMTP ayarları eksik";
        return false;
    }

    if (!netManager->ensureConnected(true)) {
        errorMessage = "İnternet bağlantısı yok";
        return false;
    }

    WiFiClientSecure client;

    if (!smtpConnect(client, errorMessage)) {
        client.stop();
        return false;
    }

    if (!smtpAuth(client, errorMessage)) {
        client.stop();
        return false;
    }

    if (!smtpSendMail(client, subject, body, includeWarningAttachments, errorMessage)) {
        client.stop();
        return false;
    }

    // QUIT
    client.print("QUIT\r\n");
    String response = smtpReadLine(client);
    Serial.printf("[SMTP] << %s\n", response.c_str());
    
    client.stop();
    Serial.println(F("[SMTP] Bağlantı kapatıldı"));
    
    return true;
}

// Test için - sadece gönderen adrese mail atar
bool MailAgent::sendEmailToSelf(const String &subject, const String &body, bool includeWarningAttachments, String &errorMessage) {
    if (settings.smtpServer.length() == 0 || settings.username.length() == 0) {
        errorMessage = "SMTP ayarları eksik";
        return false;
    }

    if (!netManager->ensureConnected(true)) {
        errorMessage = "İnternet bağlantısı yok";
        return false;
    }

    WiFiClientSecure client;

    if (!smtpConnect(client, errorMessage)) {
        client.stop();
        return false;
    }

    if (!smtpAuth(client, errorMessage)) {
        client.stop();
        return false;
    }

    // Debug: Mail detaylarını yazdır
    Serial.printf("[SMTP TEST DEBUG] To: %s (self)\n", settings.username.c_str());
    Serial.printf("[SMTP TEST DEBUG] Subject: %s\n", subject.c_str());
    Serial.printf("[SMTP TEST DEBUG] Body (first 100 chars): %.100s\n", body.c_str());
    
    // MAIL FROM
    String mailFrom = "MAIL FROM:<" + settings.username + ">\r\n";
    client.print(mailFrom);
    String response = smtpReadLine(client);
    Serial.printf("[SMTP TEST] << %s\n", response.c_str());
    
    if (!response.startsWith("250")) {
        errorMessage = "MAIL FROM reddedildi";
        client.stop();
        return false;
    }
    
    // RCPT TO - sadece kendine gönder
    String rcptTo = "RCPT TO:<" + settings.username + ">\r\n";
    client.print(rcptTo);
    response = smtpReadLine(client);
    Serial.printf("[SMTP TEST] << %s\n", response.c_str());
    
    if (!response.startsWith("250")) {
        errorMessage = "Alıcı reddedildi";
        client.stop();
        return false;
    }
    
    // DATA
    client.print("DATA\r\n");
    response = smtpReadLine(client);
    Serial.printf("[SMTP TEST] << %s\n", response.c_str());
    
    if (!response.startsWith("354")) {
        errorMessage = "DATA komutu reddedildi";
        client.stop();
        return false;
    }
    
    // MIME mesajı oluştur - sadece tek alıcı
    String boundary = "----=_SKDMF_TEST_" + String(random(100000, 999999));
    
    String mime = "From: " + settings.username + "\r\n";
    mime += "To: " + settings.username + "\r\n";
    mime += "Subject: " + subject + "\r\n";
    mime += "MIME-Version: 1.0\r\n";
    mime += "Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n";
    mime += "\r\n";
    
    // Body
    mime += "--" + boundary + "\r\n";
    mime += "Content-Type: text/plain; charset=UTF-8\r\n";
    mime += "Content-Transfer-Encoding: 8bit\r\n\r\n";
    mime += body + "\r\n";
    
    // Attachments (eğer varsa)
    if (includeWarningAttachments) {
        for (uint8_t i = 0; i < settings.attachmentCount; ++i) {
            if (!settings.attachments[i].forWarning) continue;
            
            File f = LittleFS.open(settings.attachments[i].storedPath, "r");
            if (!f) continue;
            
            mime += "--" + boundary + "\r\n";
            mime += "Content-Type: application/octet-stream\r\n";
            mime += "Content-Disposition: attachment; filename=\"" + String(settings.attachments[i].displayName) + "\"\r\n";
            mime += "Content-Transfer-Encoding: base64\r\n\r\n";
            
            uint8_t buf[57];
            while (f.available()) {
                int len = f.read(buf, 57);
                if (len > 0) {
                    String chunk = "";
                    for (int j = 0; j < len; ++j) {
                        char hex[3];
                        sprintf(hex, "%02X", buf[j]);
                        chunk += hex;
                    }
                    mime += base64Encode(chunk) + "\r\n";
                }
            }
            f.close();
        }
    }
    
    mime += "--" + boundary + "--\r\n";
    
    client.print(mime);
    client.print("\r\n.\r\n");
    
    response = smtpReadLine(client);
    Serial.printf("[SMTP TEST] << %s\n", response.c_str());
    
    if (!response.startsWith("250")) {
        errorMessage = "Mail gönderimi başarısız";
        client.stop();
        return false;
    }
    
    Serial.println(F("[SMTP TEST] Test maili kendi adresinize gönderildi"));
    
    // QUIT
    client.print("QUIT\r\n");
    response = smtpReadLine(client);
    Serial.printf("[SMTP TEST] << %s\n", response.c_str());
    
    client.stop();
    Serial.println(F("[SMTP TEST] Bağlantı kapatıldı"));
    
    return true;
}

String MailAgent::buildMimeMessage(const String &subject, const String &body, bool includeWarningAttachments) {
    String boundary = "----=_SKDMF_" + String(random(100000, 999999));
    
    String recipients;
    for (uint8_t i = 0; i < settings.recipientCount; ++i) {
        if (settings.recipients[i].length() == 0) continue;
        if (recipients.length()) recipients += ", ";
        recipients += settings.recipients[i];
    }

    String mime;
    mime += "From: " + settings.username + "\r\n";
    mime += "To: " + recipients + "\r\n";
    mime += "Subject: " + subject + "\r\n";
    mime += "MIME-Version: 1.0\r\n";
    mime += "Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n\r\n";

    mime += "--" + boundary + "\r\n";
    mime += "Content-Type: text/plain; charset=UTF-8\r\n";
    mime += "Content-Transfer-Encoding: 8bit\r\n\r\n";
    mime += body + "\r\n\r\n";

    appendAttachments(mime, boundary, includeWarningAttachments);

    mime += "--" + boundary + "--\r\n";
    return mime;
}

void MailAgent::appendAttachments(String &mime, const String &boundary, bool warning) {
    for (uint8_t i = 0; i < settings.attachmentCount; ++i) {
        const auto &meta = settings.attachments[i];
        if (warning && !meta.forWarning) continue;
        if (!warning && !meta.forFinal) continue;
        if (!LittleFS.exists(meta.storedPath)) continue;

        File file = LittleFS.open(meta.storedPath, "r");
        if (!file) continue;

        mime += "--" + boundary + "\r\n";
        mime += "Content-Type: application/octet-stream; name=\"" + String(meta.displayName) + "\"\r\n";
        mime += "Content-Transfer-Encoding: base64\r\n";
        mime += "Content-Disposition: attachment; filename=\"" + String(meta.displayName) + "\"\r\n\r\n";

        while (file.available()) {
            uint8_t buffer[48];
            size_t read = file.read(buffer, sizeof(buffer));
            String encoded = base64::encode(buffer, read);
            mime += encoded + "\r\n";
        }
        file.close();
        mime += "\r\n";
    }
}

String MailAgent::formatHeader() const {
    // millis() tabanlı çalışma süresi (NTP kullanılmıyor)
    unsigned long seconds = millis() / 1000;
    unsigned long days = seconds / 86400;
    seconds %= 86400;
    unsigned long hours = seconds / 3600;
    seconds %= 3600;
    unsigned long minutes = seconds / 60;
    seconds %= 60;
    
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Uptime: %lug %02luh %02lum %02lus", 
             days, hours, minutes, seconds);
    return String(buffer);
}

String MailAgent::formatElapsed(const ScheduleSnapshot &snapshot) const {
    uint32_t rem = snapshot.remainingSeconds;
    uint32_t days = rem / 86400UL;
    rem %= 86400UL;
    uint32_t hours = rem / 3600UL;
    rem %= 3600UL;
    uint32_t minutes = rem / 60UL;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%ud %uh %um", days, hours, minutes);
    return String(buffer);
}

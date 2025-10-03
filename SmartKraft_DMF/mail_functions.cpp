#include "mail_functions.h"

#include <LittleFS.h>
#include <base64.h>

void MailAgent::begin(ConfigStore *storePtr, DMFNetworkManager *netMgrPtr) {
    store = storePtr;
    netManager = netMgrPtr;
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
    Serial.printf("[SMTP] Bağlanıyor: %s:%d\n", settings.smtpServer.c_str(), settings.smtpPort);
    
    client.setInsecure(); // Proton sertifikası için
    
    if (!client.connect(settings.smtpServer.c_str(), settings.smtpPort)) {
        errorMessage = "SMTP sunucusuna bağlanılamadı";
        Serial.println(F("[SMTP] Bağlantı başarısız"));
        return false;
    }
    
    // Sunucu selamını bekle (220)
    String response = smtpReadLine(client);
    Serial.printf("[SMTP] << %s\n", response.c_str());
    
    if (!response.startsWith("220")) {
        errorMessage = "SMTP sunucu selamı alınamadı";
        client.stop();
        return false;
    }
    
    Serial.println(F("[SMTP] Bağlantı başarılı"));
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
    subject.replace("%ALARM_INDEX%", String(alarmIndex + 1));
    subject.replace("%TOTAL_ALARMS%", String(snapshot.totalAlarms));
    subject.replace("%REMAINING%", formatElapsed(snapshot));

    String body = settings.warning.body;
    body.replace("%ALARM_INDEX%", String(alarmIndex + 1));
    body.replace("%TOTAL_ALARMS%", String(snapshot.totalAlarms));
    body.replace("%REMAINING%", formatElapsed(snapshot));

    return sendEmail(subject, body, true, errorMessage);
}

bool MailAgent::sendFinal(const ScheduleSnapshot &snapshot, String &errorMessage) {
    String subject = settings.finalContent.subject;
    subject.replace("%REMAINING%", "0");

    String body = settings.finalContent.body;
    body.replace("%REMAINING%", "0");

    return sendEmail(subject, body, false, errorMessage);
}

bool MailAgent::smtpSendMail(WiFiClientSecure &client, const String &subject, const String &body, bool includeAttachments, String &errorMessage) {
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
    return "SmartKraft-DMF / ESP32-C6";
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

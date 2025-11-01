#include "network_manager.h"
#include <HTTPClient.h>
#include <Update.h>

void DMFNetworkManager::begin(ConfigStore *storePtr) {
    store = storePtr;
    loadConfig();
}

void DMFNetworkManager::loadConfig() {
    if (store) {
        current = store->loadWiFiSettings();
    }
}

void DMFNetworkManager::setConfig(const WiFiSettings &config) {
    current = config;
    if (store) {
        store->saveWiFiSettings(config);
    }
}

bool DMFNetworkManager::ensureConnected(bool escalateForAlarm) {
    if (isConnected()) {
        return true;
    }
    if (connectToKnown()) {
        return true;
    }
    if (escalateForAlarm && current.allowOpenNetworks) {
        return connectToOpen();
    }
    return false;
}

void DMFNetworkManager::disconnect() {
    WiFi.disconnect(true, true);
}

std::vector<DMFNetworkManager::ScanResult> DMFNetworkManager::scanNetworks() {
    std::vector<ScanResult> list;
    int16_t n = WiFi.scanNetworks();
    for (int16_t i = 0; i < n; ++i) {
        ScanResult result;
        result.ssid = WiFi.SSID(i);
        result.rssi = WiFi.RSSI(i);
        result.open = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
        list.push_back(result);
    }
    return list;
}

bool DMFNetworkManager::connectToKnown() {
    // Primary network için daha kısa timeout (5 saniye)
    if (current.primarySSID.length() > 0 && connectTo(current.primarySSID, current.primaryPassword, 5000)) {
        return true;
    }
    // Secondary network için daha kısa timeout (5 saniye)
    if (current.secondarySSID.length() > 0 && connectTo(current.secondarySSID, current.secondaryPassword, 5000)) {
        return true;
    }
    return false;
}

bool DMFNetworkManager::connectTo(const String &ssid, const String &password, uint32_t timeoutMs) {
    if (ssid.isEmpty()) {
        return false;
    }
    Serial.printf("[WiFi] %s ağına bağlanıyor...\n", ssid.c_str());
    
    // Önce mevcut durumu kontrol et - zaten bu ağa bağlıysa skip
    if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == ssid) {
        Serial.println(F("[WiFi] Zaten bağlı"));
        return true;
    }
    
    // Mevcut WiFi modunu koru - eğer AP aktifse dual mode kullan
    wifi_mode_t currentMode = WiFi.getMode();
    if (currentMode == WIFI_AP || currentMode == WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);  // Dual mode - AP'yi koru
    } else {
        WiFi.mode(WIFI_STA);  // Sadece STA
    }
    
    // Statik IP gerekiyorsa uygula
    applyStaticIfNeeded(ssid);
    
    // Disconnect etmeden önce durumu kontrol et
    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED || status == WL_CONNECT_FAILED) {
        WiFi.disconnect(false, false); // WiFi'yi reset etme, sadece disconnect
        delay(50); // 100ms → 50ms (daha hızlı)
    }

    WiFi.begin(ssid.c_str(), password.length() ? password.c_str() : nullptr);
    
    // ⚠️ WiFi UYKU MODUNU KAPAT (bağlantıdan hemen sonra)
    WiFi.setSleep(WIFI_PS_NONE);

    uint32_t start = millis();
    uint32_t lastYield = millis();
    
    while (millis() - start < timeoutMs) {
        wl_status_t currentStatus = WiFi.status();
        
        if (currentStatus == WL_CONNECTED) {
            Serial.printf("[WiFi] Bağlantı başarılı: %s\n", WiFi.localIP().toString().c_str());
            return true;
        }
        
        // Bağlantı başarısız olduysa hemen çık
        if (currentStatus == WL_CONNECT_FAILED || currentStatus == WL_NO_SSID_AVAIL) {
            Serial.printf("[WiFi] Bağlantı hızlı başarısız: %d\n", currentStatus);
            break;
        }
        
        // Yield her 100ms'de bir - responsive tutmak için
        if (millis() - lastYield > 100) {
            yield();
            lastYield = millis();
        }
        
        delay(100); // 250ms → 100ms (daha responsive)
    }
    
    Serial.println(F("[WiFi] Bağlantı zaman aşımı veya başarısız"));
    return false;
}

bool DMFNetworkManager::connectToOpen() {
    Serial.println(F("[WiFi] Açık ağlar taranıyor..."));
    auto networks = scanNetworks();
    for (auto &net : networks) {
        if (!net.open) continue;
        Serial.printf("[WiFi] Açık ağ deneme: %s\n", net.ssid.c_str());
        // Açık ağlar için daha kısa timeout (8 saniye)
        if (connectTo(net.ssid, "", 8000)) {
            // İnternet testi de kısa tutuldu (30 saniye)
            if (testInternet(30000)) {
                Serial.println(F("[WiFi] Açık ağda internet erişimi doğrulandı"));
                return true;
            } else {
                Serial.println(F("[WiFi] İnternet yok, ağdan çıkılıyor"));
                WiFi.disconnect();
            }
        }
    }
    Serial.println(F("[WiFi] Uygun (internetli) açık ağ bulunamadı"));
    return false;
}

bool DMFNetworkManager::testInternet(uint32_t timeoutMs) {
    // Basit yöntem: bir DNS çözümleme + bağlantı denemesi (ör: time.nist.gov veya 1.1.1.1 ping değil).
    // Arduino HTTPClient burada kullanmak istemiyoruz; yalnızca DNS yeterli sinyal verebilir.
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        // DNS testi
        IPAddress ip;
        if (WiFi.hostByName("time.cloudflare.com", ip) == 1) {
            if (ip != IPAddress()) return true;
        }
        delay(1000);
    }
    return false;
}

bool DMFNetworkManager::applyStaticIfNeeded(const String &ssid) {
    bool applied = false;
    if (ssid == current.primarySSID && current.primaryStaticEnabled) {
        IPAddress ip, gw, mask, dns;
        if (ip.fromString(current.primaryIP) && gw.fromString(current.primaryGateway) && mask.fromString(current.primarySubnet)) {
            if (!current.primaryDNS.isEmpty()) dns.fromString(current.primaryDNS); else dns = gw;
            if (WiFi.config(ip, gw, mask, dns)) {
                Serial.println(F("[WiFi] Primary statik IP uygulandı"));
                applied = true;
            } else {
                Serial.println(F("[WiFi] Primary statik IP uygulanamadı"));
            }
        }
    } else if (ssid == current.secondarySSID && current.secondaryStaticEnabled) {
        IPAddress ip, gw, mask, dns;
        if (ip.fromString(current.secondaryIP) && gw.fromString(current.secondaryGateway) && mask.fromString(current.secondarySubnet)) {
            if (!current.secondaryDNS.isEmpty()) dns.fromString(current.secondaryDNS); else dns = gw;
            if (WiFi.config(ip, gw, mask, dns)) {
                Serial.println(F("[WiFi] Secondary statik IP uygulandı"));
                applied = true;
            } else {
                Serial.println(F("[WiFi] Secondary statik IP uygulanamadı"));
            }
        }
    } else {
        // DHCP
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // Reset to DHCP
    }
    return applied;
}

// OTA Update Implementation
bool DMFNetworkManager::checkOTAUpdate(String currentVersion) {
    if (!isConnected()) {
        Serial.println(F("[OTA] WiFi bağlı değil, güncelleme kontrolü atlandı"));
        return false;
    }

    HTTPClient http;
    http.setTimeout(10000); // 10 saniye timeout
    
    const char* versionURL = "https://raw.githubusercontent.com/smrtkrft/DMF_protocol/main/releases/version.txt";
    
    Serial.printf("[OTA] Versiyon kontrolü: %s\n", versionURL);
    http.begin(versionURL);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String latestVersion = http.getString();
        latestVersion.trim(); // Whitespace temizle
        
        Serial.printf("[OTA] Mevcut versiyon: %s, En son versiyon: %s\n", 
                      currentVersion.c_str(), latestVersion.c_str());
        
        http.end();
        
        // Versiyon karşılaştırması
        if (latestVersion != currentVersion && latestVersion.length() > 0) {
            Serial.println(F("[OTA] Yeni versiyon mevcut!"));
            performOTAUpdate();
            return true;
        } else {
            Serial.println(F("[OTA] Güncelleme gerekmiyor"));
            return false;
        }
    } else {
        Serial.printf("[OTA] HTTP hatası: %d\n", httpCode);
        http.end();
        return false;
    }
}

void DMFNetworkManager::performOTAUpdate() {
    if (!isConnected()) {
        Serial.println(F("[OTA] WiFi bağlı değil, güncelleme iptal edildi"));
        return;
    }

    HTTPClient http;
    http.setTimeout(60000); // 60 saniye timeout (firmware indirme için)
    
    const char* firmwareURL = "https://github.com/smrtkrft/DMF_protocol/releases/latest/download/SmartKraft_DMF.ino.bin";
    
    Serial.printf("[OTA] Firmware indiriliyor: %s\n", firmwareURL);
    http.begin(firmwareURL);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        
        if (contentLength > 0) {
            Serial.printf("[OTA] Firmware boyutu: %d bytes\n", contentLength);
            
            bool canBegin = Update.begin(contentLength);
            
            if (canBegin) {
                WiFiClient *stream = http.getStreamPtr();
                
                Serial.println(F("[OTA] Güncelleme başlatılıyor..."));
                
                size_t written = Update.writeStream(*stream);
                
                if (written == contentLength) {
                    Serial.println(F("[OTA] Yazma tamamlandı"));
                } else {
                    Serial.printf("[OTA] Yazma hatası: sadece %d/%d byte yazıldı\n", written, contentLength);
                }
                
                if (Update.end()) {
                    if (Update.isFinished()) {
                        Serial.println(F("[OTA] Güncelleme başarılı! Yeniden başlatılıyor..."));
                        http.end();
                        delay(1000);
                        ESP.restart();
                    } else {
                        Serial.println(F("[OTA] Güncelleme tamamlanamadı"));
                    }
                } else {
                    Serial.printf("[OTA] Hata: %s\n", Update.errorString());
                }
            } else {
                Serial.println(F("[OTA] Yeterli alan yok"));
            }
        } else {
            Serial.println(F("[OTA] Geçersiz içerik boyutu"));
        }
    } else {
        Serial.printf("[OTA] Firmware indirme hatası: %d\n", httpCode);
    }
    
    http.end();
}

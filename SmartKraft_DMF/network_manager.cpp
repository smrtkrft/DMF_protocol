#include "network_manager.h"

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
    if (current.primarySSID.length() > 0 && connectTo(current.primarySSID, current.primaryPassword)) {
        return true;
    }
    if (current.secondarySSID.length() > 0 && connectTo(current.secondarySSID, current.secondaryPassword)) {
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
        delay(100);
    }

    WiFi.begin(ssid.c_str(), password.length() ? password.c_str() : nullptr);

    uint32_t start = millis();
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
        
        delay(250); // 500ms → 250ms (daha responsive)
    }
    
    Serial.println(F("[WiFi] Bağlantı zaman aşımı veya başarısız"));
    // Disconnect'i kaldır - mode zaten ayarlı, tekrar deneyebilir
    return false;
}

bool DMFNetworkManager::connectToOpen() {
    Serial.println(F("[WiFi] Açık ağlar taranıyor..."));
    auto networks = scanNetworks();
    for (auto &net : networks) {
        if (!net.open) continue;
        Serial.printf("[WiFi] Açık ağ deneme: %s\n", net.ssid.c_str());
        if (connectTo(net.ssid, "", 15000)) {
            // İnternet testi 60 saniye
            if (testInternet(60000)) {
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

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
    
    // Mevcut WiFi modunu koru - eğer AP aktifse dual mode kullan
    wifi_mode_t currentMode = WiFi.getMode();
    if (currentMode == WIFI_AP || currentMode == WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);  // Dual mode - AP'yi kapat
    } else {
        WiFi.mode(WIFI_STA);  // Sadece STA
    }
    
    WiFi.begin(ssid.c_str(), password.length() ? password.c_str() : nullptr);

    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFi] Bağlantı başarılı: %s\n", WiFi.localIP().toString().c_str());
            return true;
        }
        delay(500);
    }
    Serial.println(F("[WiFi] Bağlantı zaman aşımı"));
    WiFi.disconnect();
    return false;
}

bool DMFNetworkManager::connectToOpen() {
    Serial.println(F("[WiFi] Açık ağlar taranıyor..."));
    auto networks = scanNetworks();
    for (auto &net : networks) {
        if (!net.open) {
            continue;
        }
        Serial.printf("[WiFi] Açık ağ deneme: %s\n", net.ssid.c_str());
        if (connectTo(net.ssid, "", 15000)) {
            return true;
        }
    }
    Serial.println(F("[WiFi] Uygun açık ağ bulunamadı"));
    return false;
}

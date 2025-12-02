/**
 * ota_manager.h - OTA (Over-The-Air) Güncelleme Yöneticisi
 * 
 * SmartKraft DMF için OTA güncelleme yönetimi.
 * 
 * Özellikler:
 * - GitHub API üzerinden versiyon kontrolü
 * - Rastgele aralıklarda güncelleme kontrolü (24-48 saat)
 * - LittleFS üzerinde zamanlama kalıcılığı
 * - Rate limit koruması
 * 
 * Zamanlama Mantığı:
 * - İlk açılışta: Rastgele 1-5 dakika sonra kontrol (startup süresi için)
 * - Sonrasında: 24 saat + random(1 dk - 24 saat) = 24-48 saat arası
 * 
 * Copyright (c) 2024-2025 SmartKraft
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// OTA Sabitleri
constexpr uint32_t OTA_BASE_INTERVAL_MS = 24UL * 60UL * 60UL * 1000UL;     // 24 saat (temel aralık)
constexpr uint32_t OTA_RANDOM_MAX_MS = 24UL * 60UL * 60UL * 1000UL;        // 24 saat (maksimum random ekleme)
constexpr uint32_t OTA_RANDOM_MIN_MS = 60UL * 1000UL;                       // 1 dakika (minimum random ekleme)
constexpr uint32_t OTA_STARTUP_MIN_MS = 60UL * 1000UL;                      // 1 dakika (startup minimum)
constexpr uint32_t OTA_STARTUP_MAX_MS = 5UL * 60UL * 1000UL;                // 5 dakika (startup maximum)
constexpr uint32_t OTA_HTTP_TIMEOUT_MS = 15000;                             // 15 saniye (API timeout)
constexpr uint32_t OTA_DOWNLOAD_TIMEOUT_MS = 120000;                        // 2 dakika (firmware indirme)

// OTA Durum Dosyası
constexpr const char* OTA_STATE_FILE = "/ota_state.json";

// GitHub API Bilgileri
constexpr const char* OTA_GITHUB_API_URL = "https://api.github.com/repos/smrtkrft/DMF_protocol/releases/latest";
constexpr const char* OTA_GITHUB_REPO_BASE = "https://github.com/smrtkrft/DMF_protocol/releases/download/";
constexpr const char* OTA_FIRMWARE_FILENAME = "SmartKraft_DMF.ino.bin";

/**
 * OTA Durumu
 */
struct OTAState {
    uint32_t nextCheckInterval;     // Sonraki kontrol için bekleme süresi (ms)
    unsigned long lastCheckTime;    // Son kontrol zamanı (boot'tan beri ms)
    uint32_t checkCount;            // Toplam kontrol sayısı
    uint32_t successCount;          // Başarılı güncelleme sayısı
    uint32_t failCount;             // Başarısız güncelleme sayısı
    bool startupCheckDone;          // İlk açılış kontrolü yapıldı mı?
};

/**
 * OTA Manager Sınıfı
 */
class OTAManager {
public:
    OTAManager();
    
    /**
     * OTA yöneticisini başlat
     * @param firmwareVersion Mevcut firmware versiyonu
     */
    void begin(const String& firmwareVersion);
    
    /**
     * Ana loop'ta çağrılacak - zamanlama kontrolü yapar
     * WiFi bağlıysa ve zaman geldiyse otomatik kontrol başlatır
     */
    void loop();
    
    /**
     * OTA kontrolü yap (manuel tetikleme için)
     * @return true: güncelleme bulundu ve uygulandı, false: güncelleme yok veya hata
     */
    bool checkForUpdate();
    
    /**
     * Sonraki kontrol için kalan süreyi döndür (ms)
     */
    uint32_t getTimeToNextCheck() const;
    
    /**
     * OTA durumunu döndür
     */
    OTAState getState() const { return state; }
    
    /**
     * WiFi bağlantı durumunu güncelle
     */
    void setWiFiConnected(bool connected) { wifiConnected = connected; }
    
private:
    String currentVersion;
    OTAState state;
    bool wifiConnected;
    bool initialized;
    
    // Zamanlama
    unsigned long bootTime;
    unsigned long lastLoopTime;
    
    /**
     * Sonraki kontrol aralığını hesapla (24-48 saat arası random)
     */
    uint32_t calculateNextInterval();
    
    /**
     * İlk açılış için kısa aralık hesapla (1-5 dakika)
     */
    uint32_t calculateStartupInterval();
    
    /**
     * Durumu LittleFS'e kaydet
     */
    void saveState();
    
    /**
     * Durumu LittleFS'ten yükle
     */
    void loadState();
    
    /**
     * GitHub API'den en son versiyonu al
     * @param latestVersion [out] En son versiyon (başarılıysa)
     * @return true: başarılı, false: hata
     */
    bool fetchLatestVersion(String& latestVersion);
    
    /**
     * Versiyon karşılaştırması
     * @return -1: current < latest, 0: eşit, 1: current > latest
     */
    int compareVersions(const String& current, const String& latest);
    
    /**
     * Firmware'i indir ve güncelle
     * @param version İndirilecek versiyon
     * @return true: başarılı (cihaz yeniden başlatılacak), false: hata
     */
    bool downloadAndUpdate(const String& version);
};

#endif // OTA_MANAGER_H

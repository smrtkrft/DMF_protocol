#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <DNSServer.h>
#include <esp_pm.h>        // Güç yönetimi için
#include <esp_wifi.h>      // WiFi güç yönetimi için

#include "config_store.h"
#include "scheduler.h"
#include "network_manager.h"
#include "mail_functions.h"
#include "web_handlers.h"

// ⚠️ Debug Seviyeleri (performans için)
// 0 = Sadece kritik hatalar
// 1 = Temel bilgiler (varsayılan)
// 2 = Detaylı debug (geliştirme)
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 1
#endif

#define DEBUG_PRINT(level, ...) if (DEBUG_LEVEL >= level) Serial.printf(__VA_ARGS__)
#define DEBUG_PRINTLN(level, ...) if (DEBUG_LEVEL >= level) Serial.println(__VA_ARGS__)

// Firmware Version
#define FIRMWARE_VERSION "v1.0.3"

// Pin tanımları - XIAO ESP32C6 (GERÇEK TEST EDİLMİŞ DEĞERLER)
// Kaynak: C6-Pin&Gpio.md
// D3 = GPIO21 (BUTTON), D10 = GPIO18 (RELAY)

constexpr uint8_t BUTTON_PIN = 21;   // D3 -> GPIO21
constexpr uint8_t RELAY_PIN = 18;    // D10 -> GPIO18
constexpr uint32_t BUTTON_DEBOUNCE_MS = 200;
constexpr uint32_t STATUS_PERSIST_INTERVAL_MS = 60000;
constexpr uint32_t OTA_CHECK_INTERVAL_MS = 300000; // 5 dakika (12 req/saat - güvenli)
constexpr uint32_t PERIODIC_RESTART_INTERVAL_MS = 12UL * 60UL * 60UL * 1000UL; // 12 saat


ConfigStore configStore;
CountdownScheduler scheduler;
DMFNetworkManager networkManager;
MailAgent mailAgent;
WebServer webServer(80);
WebInterface webUI;
DNSServer dnsServer;

bool relayLatched = false;
unsigned long lastButtonChange = 0;
bool lastButtonState = true;
unsigned long lastPersist = 0;
unsigned long finalMailSentTime = 0; // Final mail gönderilme zamanı
bool finalMailSent = false; // Final mail gönderildi mi?
unsigned long lastOTACheck = 0; // OTA kontrol zamanı
unsigned long bootTime = 0; // Cihaz başlangıç zamanı (periyodik restart için)

String deviceId;

String generateDeviceId() {
    uint64_t mac = ESP.getEfuseMac();
    // Format: "SmartKraft DMF-7FFE" (tire ile bitişik)
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "SmartKraft DMF-%04X", (uint32_t)((mac >> 32) & 0xFFFF));
    return String(buffer);
}

String generateAPName() {
    uint64_t mac = ESP.getEfuseMac();
    // İlk 4 hex karakter (üst 16 bit) her kart için benzersiz
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "SmartKraft-DMF%04X", (uint32_t)((mac >> 32) & 0xFFFF));
    return String(buffer);
}

void initHardware() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    relayLatched = false;
}

void latchRelay(bool state) {
    relayLatched = state;
    digitalWrite(RELAY_PIN, state ? HIGH : LOW);
}

void resetTimerFromButton() {
    latchRelay(false);
    scheduler.reset();
    scheduler.start();
    scheduler.persist();
    
    // Reboot flag'ini sıfırla
    finalMailSent = false;
    finalMailSentTime = 0;
}

void processAlarms() {
    uint8_t alarmIndex = 0;

    if (scheduler.alarmDue(alarmIndex)) {
        ScheduleSnapshot snap = scheduler.snapshot();
        Serial.printf("[Alarm] %u numaralı alarm tetiklendi\n", alarmIndex + 1);
        
        if (!networkManager.isConnected()) {
            Serial.println(F("[Alarm] İnternet yok, bağlantı kuruluyor..."));
            networkManager.ensureConnected(true);
        }
        
        String error;
        if (!mailAgent.sendWarning(alarmIndex, snap, error)) {
            Serial.printf("[Mail] Uyarı maili gönderilemedi: %s\n", error.c_str());
            Serial.println(F("[Mail] ℹ Alarm beklemeye alındı, internet bağlantısı kurulunca tekrar denenecek"));
            // ❌ acknowledgeAlarm() ÇAĞRILMIYOR - Mail başarısız olduğu için
            // Alarm bir sonraki loop'ta tekrar kontrol edilecek
        } else {
            Serial.println(F("[Mail] ✓ Uyarı maili gönderildi"));
            scheduler.acknowledgeAlarm(alarmIndex); // ✅ Sadece başarılı olunca işaretle
            scheduler.persist();
        }
        
        yield();
    }

    if (scheduler.finalDue()) {
        ScheduleSnapshot snap = scheduler.snapshot();
        Serial.println(F("[Final] Süre doldu, röle tetikleniyor"));
        latchRelay(true);
        
        if (!networkManager.isConnected()) {
            Serial.println(F("[Final] İnternet yok, bağlantı kuruluyor..."));
            networkManager.ensureConnected(true);
        }
        
        // Runtime'ı al (grup gönderim durumları için)
        TimerRuntime runtime = scheduler.runtimeState();
        
        String error;
        if (!mailAgent.sendFinal(snap, runtime, error)) {
            Serial.printf("[Mail] Final maili gönderilemedi: %s\n", error.c_str());
            Serial.println(F("[Mail] ℹ Final mail beklemeye alındı, internet bağlantısı kurulunca tekrar denenecek"));
            // Runtime değişmiş olabilir (bazı gruplar gönderilmiş), kaydet
            scheduler.updateRuntime(runtime);
            // ❌ acknowledgeFinal() ÇAĞRILMIYOR - Tüm gruplar başarılı olana kadar
        } else {
            Serial.println(F("[Mail] ✓ Final maili gönderildi"));
            scheduler.acknowledgeFinal(); // ✅ Tüm gruplar başarılı
            scheduler.persist();
            
            if (!finalMailSent) {
                finalMailSent = true;
                finalMailSentTime = millis();
                Serial.println(F("[Reboot] 60 saniye sonra cihaz yeniden başlatılacak"));
            }
        }
        
        yield();
    }
}

void handleButton() {
    bool state = digitalRead(BUTTON_PIN);
    
    if (state != lastButtonState) {
        if (millis() - lastButtonChange > BUTTON_DEBOUNCE_MS) {
            lastButtonChange = millis();
            lastButtonState = state;
            
            if (state == LOW) {
                Serial.println(F("[BUTTON] Fiziksel buton basıldı - Timer sıfırlanıyor"));
                resetTimerFromButton();
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println();
    Serial.println(F("=== SmartKraft DMF Başlıyor ==="));

    deviceId = generateDeviceId();
    Serial.printf("[Cihaz] ID: %s\n", deviceId.c_str());
    randomSeed(esp_random());

    // CPU güç yönetimini kapat (light sleep/modem sleep engellenir)
    esp_pm_config_t pm_config;
    pm_config.max_freq_mhz = 160;
    pm_config.min_freq_mhz = 160;
    pm_config.light_sleep_enable = false;
    esp_pm_configure(&pm_config);

    initHardware();

    if (!configStore.begin()) {
        Serial.println(F("[FS] Dosya sistemi başlatılamadı"));
    }

    scheduler.begin(&configStore);
    networkManager.begin(&configStore);
    mailAgent.begin(&configStore, &networkManager, deviceId);
    
    String apName = generateAPName();
    webUI.begin(&webServer, &configStore, &scheduler, &mailAgent, &networkManager, deviceId, &dnsServer, apName);

    Serial.println(F("[Init] Sistem başlatma tamamlandı"));
    
    ScheduleSnapshot initialSnap = scheduler.snapshot();

    if (initialSnap.timerActive) {
        Serial.println(F("[Timer] Kalan süre ile devam ediliyor"));
        
        // ⚠️ YENİ: Reboot sonrası final durumunu kontrol et
        if (initialSnap.finalTriggered) {
            Serial.println(F("[Timer] Final durumu tespit edildi - röle açık kalacak"));
            latchRelay(true);
            finalMailSent = true; // Tekrar mail gönderilmesini önle
        }
    } else {
        latchRelay(false);
    }

    lastButtonState = digitalRead(BUTTON_PIN);
    lastButtonChange = millis();
    lastPersist = millis();
    lastOTACheck = millis();
    bootTime = millis();
    
    // WebServer başlat (içinde WiFi bağlantısı da yapılıyor)
    webUI.startServer();
    
    // WiFi optimizasyonları
    WiFi.setSleep(WIFI_PS_NONE);
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_max_tx_power(84);
    
    Serial.println(F("[Init] ✅ Sistem hazır - WiFi aktif!"));
}

void loop() {
    static unsigned long lastLoop = 0;
    static unsigned long loopCounter = 0;
    
    // ⚠️ ESP32-C6: Web server handler - BLOKLANMASIN (en yüksek öncelik)
    webUI.loop();
    yield();  // WiFi stack'e zaman tanı
    
    // Core timer functionality
    scheduler.tick();
    handleButton();
    
    // Alarmları daha az sıklıkta kontrol et - her 500ms'de bir
    if (millis() - lastLoop > 500) {
        processAlarms();
        lastLoop = millis();
        yield();  // Uzun işlemlerden sonra yield
    }

    // ⚠️ YENİ: Final mail gönderildikten 60 saniye sonra reboot
    if (finalMailSent) {
        unsigned long elapsed = millis() - finalMailSentTime;
        unsigned long remaining = 60000 - elapsed;
        
        // Her 10 saniyede bir geri sayım mesajı bas
        static unsigned long lastCountdownMsg = 0;
        if (millis() - lastCountdownMsg >= 10000) {
            if (remaining > 1000) {
                Serial.printf("[Reboot] Yeniden başlatmaya %lu saniye kaldı...\n", remaining / 1000);
                lastCountdownMsg = millis();
            }
        }
        
        // 60 saniye doldu - reboot!
        if (elapsed >= 60000) {
            Serial.println(F(""));
            Serial.println(F("========================================"));
            Serial.println(F("[Reboot] 60 saniye doldu!"));
            Serial.println(F("[Reboot] Cihaz yeniden başlatılıyor..."));
            Serial.println(F("========================================"));
            delay(1000); // Serial mesajının gönderilmesi için bekle
            ESP.restart();
        }
    }

    // Dosya sistemine yazma işlemi - daha az sıklıkta (her 60 saniye)
    if (millis() - lastPersist > STATUS_PERSIST_INTERVAL_MS) {
        scheduler.persist();
        lastPersist = millis();
    }
    
    // ⚠️ YENİ: OTA kontrol - her 2 dakikada bir
    if (millis() - lastOTACheck > OTA_CHECK_INTERVAL_MS) {
        if (networkManager.isConnected()) {
            DEBUG_PRINTLN(2, F("[OTA] Version kontrolü yapılıyor..."));
            networkManager.checkOTAUpdate(FIRMWARE_VERSION);
        }
        lastOTACheck = millis();
    }
    
    // ⚠️ YENİ: Periyodik restart - 12 saatte bir
    unsigned long uptime = millis() - bootTime;
    if (uptime >= PERIODIC_RESTART_INTERVAL_MS) {
        Serial.println(F(""));
        Serial.println(F("========================================"));
        Serial.println(F("[Restart] 12 saat doldu - periyodik restart"));
        Serial.println(F("[Restart] Cihaz yeniden başlatılıyor..."));
        Serial.println(F("========================================"));
        scheduler.persist(); // Son durumu kaydet
        delay(1000);
        ESP.restart();
    }
    
    // Her 1 saatte bir uptime bilgisi ver
    static unsigned long lastUptimeReport = 0;
    if (millis() - lastUptimeReport >= 3600000) { // 1 saat
        unsigned long hours = uptime / 3600000;
        unsigned long minutes = (uptime % 3600000) / 60000;
        Serial.printf("[System] Uptime: %lu saat %lu dakika\n", hours, minutes);
        lastUptimeReport = millis();
    }
    
    // Sistem responsive tutmak için
    yield();
    loopCounter++;
    
    // Minimal delay - çok hızlı döngüyü önlemek için
    delayMicroseconds(100);
}

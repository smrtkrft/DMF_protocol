#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <DNSServer.h>
#include <esp_task_wdt.h>  // Watchdog timer için
#include <esp_pm.h>        // Güç yönetimi için
#include <esp_wifi.h>      // WiFi güç yönetimi için

#include "config_store.h"
#include "scheduler.h"
#include "network_manager.h"
#include "mail_functions.h"
#include "web_handlers.h"
#include "test_functions.h"

// Pin tanımları - XIAO ESP32C6 (GERÇEK TEST EDİLMİŞ DEĞERLER)
// Kaynak: C6-Pin&Gpio.md
// D3 = GPIO21 (BUTTON), D10 = GPIO18 (RELAY)

constexpr uint8_t BUTTON_PIN = 21;   // D3 -> GPIO21
constexpr uint8_t RELAY_PIN = 18;    // D10 -> GPIO18
constexpr uint32_t BUTTON_DEBOUNCE_MS = 200;
constexpr uint32_t STATUS_PERSIST_INTERVAL_MS = 60000;

ConfigStore configStore;
CountdownScheduler scheduler;
DMFNetworkManager networkManager;
MailAgent mailAgent;
WebServer webServer(80);
WebInterface webUI;
TestInterface testInterface;
DNSServer dnsServer;

bool relayLatched = false;
unsigned long lastButtonChange = 0;
bool lastButtonState = true;
unsigned long lastPersist = 0;
unsigned long finalMailSentTime = 0; // Final mail gönderilme zamanı
bool finalMailSent = false; // Final mail gönderildi mi?

String deviceId;

String generateDeviceId() {
    uint64_t mac = ESP.getEfuseMac();
    // İlk 4 hex karakter (üst 16 bit) her kart için benzersiz
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "SmartKraft-DMF%04X", (uint32_t)((mac >> 32) & 0xFFFF));
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
    
    Serial.printf("[Init] BUTTON: D3 (GPIO%d)\n", BUTTON_PIN);
    Serial.printf("[Init] RELAY: D10 (GPIO%d)\n", RELAY_PIN);
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
    
    Serial.println(F("[Timer] Fiziksel/sanal buton ile geri sayım yeniden başlatıldı"));
}

// performGetRequest() fonksiyonu KALDIRILDI
// URL tetikleme artık mail_functions.cpp içinde yapılıyor (güvenli + validation)

void processAlarms() {
    uint8_t alarmIndex = 0;

    if (scheduler.alarmDue(alarmIndex)) {
        ScheduleSnapshot snap = scheduler.snapshot();
        Serial.printf("[Alarm] %u numaralı alarm tetiklendi\n", alarmIndex + 1);
        
        if (!networkManager.isConnected()) {
            networkManager.ensureConnected(true);
        }
        
        String error;
        if (!mailAgent.sendWarning(alarmIndex, snap, error)) {
            Serial.printf("[Mail] Uyarı maili gönderilemedi: %s\n", error.c_str());
        } else {
            Serial.println(F("[Mail] Uyarı maili gönderildi"));
        }
        
        scheduler.acknowledgeAlarm(alarmIndex);
        scheduler.persist();
        yield();
    }

    if (scheduler.finalDue()) {
        ScheduleSnapshot snap = scheduler.snapshot();
        Serial.println(F("[Final] Süre doldu, röle tetikleniyor"));
        latchRelay(true);
        
        if (!networkManager.isConnected()) {
            networkManager.ensureConnected(true);
        }
        
        String error;
        if (!mailAgent.sendFinal(snap, error)) {
            Serial.printf("[Mail] Final maili gönderilemedi: %s\n", error.c_str());
        } else {
            Serial.println(F("[Mail] Final maili gönderildi"));
            
            if (!finalMailSent) {
                finalMailSent = true;
                finalMailSentTime = millis();
                Serial.println(F("[Reboot] 60 saniye sonra cihaz yeniden başlatılacak"));
            }
        }
        
        scheduler.acknowledgeFinal();
        scheduler.persist();
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

    // ⚠️ UYKU MODUNU TAMAMEN DEVRE DIŞI BIRAK
    Serial.println(F("[Power] Güç yönetimi devre dışı bırakılıyor..."));
    
    // CPU otomatik güç yönetimini kapat (light sleep/modem sleep engellenir)
    esp_pm_config_t pm_config;
    pm_config.max_freq_mhz = 160;  // Maksimum CPU frekansı
    pm_config.min_freq_mhz = 160;  // Minimum CPU frekansı (AYNI değer = frekans düşürme YOK)
    pm_config.light_sleep_enable = false;  // Light sleep KAPALI
    esp_pm_configure(&pm_config);
    Serial.println(F("[Power] CPU güç yönetimi devre dışı (160MHz sabit)"));

    Serial.println(F("[Init] Donanım başlatılıyor..."));
    initHardware();

    Serial.println(F("[Init] Dosya sistemi başlatılıyor..."));
    if (!configStore.begin()) {
        Serial.println(F("[FS] Dosya sistemi başlatılamadı"));
    }

    Serial.println(F("[Init] Scheduler başlatılıyor..."));
    scheduler.begin(&configStore);
    
    Serial.println(F("[Init] Network manager başlatılıyor..."));
    networkManager.begin(&configStore);
    
    Serial.println(F("[Init] Mail agent başlatılıyor..."));
    mailAgent.begin(&configStore, &networkManager, deviceId);
    
    Serial.println(F("[Init] Web sunucusu başlatılıyor..."));
    String apName = generateAPName();
    webUI.begin(&webServer, &configStore, &scheduler, &mailAgent, &networkManager, deviceId, &dnsServer, apName);
    
    Serial.println(F("[Init] Test arayüzü başlatılıyor..."));
    testInterface.begin(&scheduler, &mailAgent);

    Serial.println(F("[Init] Başlangıç tamamlandı, WiFi devre dışı"));
    
    ScheduleSnapshot initialSnap = scheduler.snapshot();

    if (initialSnap.timerActive) {
        Serial.println(F("[Timer] Kalan süre ile devam ediliyor"));
    } else {
        latchRelay(false);
    }

    lastButtonState = digitalRead(BUTTON_PIN);
    lastButtonChange = millis();
    lastPersist = millis();
    
    Serial.println(F("[Init] WebServer başlatılıyor..."));
    webUI.startServer();
    
    // ⚠️ WiFi GÜÇÜNÜ MAKSIMUMA ÇEK VE UYKU MODUNU DEVRE DIŞI BIRAK
    WiFi.setSleep(WIFI_PS_NONE);  // WiFi uyku modu KAPALI
    esp_wifi_set_ps(WIFI_PS_NONE);  // ESP-IDF seviyesinde WiFi güç tasarrufu KAPALI
    Serial.println(F("[WiFi] Güç tasarrufu KAPALI - WiFi sürekli aktif"));
    
    Serial.println(F("[Init] Sistem hazır!"));
}

void loop() {
    static unsigned long lastLoop = 0;
    static unsigned long loopCounter = 0;
    
    // Web server handler - en yüksek öncelik
    webUI.loop();
    
    // Core timer functionality
    scheduler.tick();
    handleButton();
    
    // Alarmları daha az sıklıkta kontrol et - her 500ms'de bir
    if (millis() - lastLoop > 500) {
        processAlarms();
        lastLoop = millis();
    }
    
    // Test interface'i daha az sıklıkta - her 1 saniyede bir
    if (loopCounter % 100 == 0) { // 10ms x 100 = 1s
        testInterface.processSerial();
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
    
    // Sistem responsive tutmak için
    yield();
    loopCounter++;
    
    // Minimal delay - çok hızlı döngüyü önlemek için
    delayMicroseconds(100);
}

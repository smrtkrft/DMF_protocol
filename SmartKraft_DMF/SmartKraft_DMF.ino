#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
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
#define FIRMWARE_VERSION "v1.0.6" // web_handlers.cpp auch hat Version check , beide müssen übereinstimmen
                                // network_manager.cpp → v1.0.6
// Pin tanımları - XIAO ESP32C6 (GERÇEK TEST EDİLMİŞ DEĞERLER)
// Kaynak: C6-Pin&Gpio.md
// D3 = GPIO21 (BUTTON), D10 = GPIO18 (RELAY)

constexpr uint8_t BUTTON_PIN = 18;   // D10 -> GPIO18
constexpr uint8_t RELAY_PIN = 17;    // D7 -> GPIO17
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
    digitalWrite(RELAY_PIN, LOW);   // ← PC817C için LOW = Kapalı (LED sönük)
    relayLatched = false;
}

void latchRelay(bool state) {
    relayLatched = state;
    digitalWrite(RELAY_PIN, state ? HIGH : LOW);  // ← PC817C: true=HIGH (LED yanar, röle tetiklenir)
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
        
        if (!networkManager.isConnected()) {
            networkManager.ensureConnected(true);
        }
        
        String error;
        if (!mailAgent.sendWarning(alarmIndex, snap, error)) {
            // Mail başarısız, tekrar denenecek
        } else {
            scheduler.acknowledgeAlarm(alarmIndex);
            scheduler.persist();
        }
        
        yield();
    }

    if (scheduler.finalDue()) {
        ScheduleSnapshot snap = scheduler.snapshot();
        latchRelay(true);
        
        if (!networkManager.isConnected()) {
            networkManager.ensureConnected(true);
        }
        
        TimerRuntime runtime = scheduler.runtimeState();
        
        String error;
        if (!mailAgent.sendFinal(snap, runtime, error)) {
            scheduler.updateRuntime(runtime);
        } else {
            scheduler.acknowledgeFinal();
            scheduler.persist();
            
            if (!finalMailSent) {
                finalMailSent = true;
                finalMailSentTime = millis();
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
                resetTimerFromButton();
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);

    deviceId = generateDeviceId();
    randomSeed(esp_random());

    // CPU güç yönetimini kapat (light sleep/modem sleep engellenir)
    esp_pm_config_t pm_config;
    pm_config.max_freq_mhz = 160;
    pm_config.min_freq_mhz = 160;
    pm_config.light_sleep_enable = false;
    esp_pm_configure(&pm_config);

    initHardware();
    configStore.begin();
    scheduler.begin(&configStore);
    networkManager.begin(&configStore);
    mailAgent.begin(&configStore, &networkManager, deviceId);
    
    String apName = generateAPName();
    webUI.begin(&webServer, &configStore, &scheduler, &mailAgent, &networkManager, deviceId, &dnsServer, apName);
    
    latchRelay(false);
    lastButtonState = digitalRead(BUTTON_PIN);
    lastButtonChange = millis();
    lastPersist = millis();
    lastOTACheck = millis();
    bootTime = millis();
    
    webUI.startServer();
    
    WiFi.setSleep(WIFI_PS_NONE);
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_max_tx_power(84);
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

    if (finalMailSent && (millis() - finalMailSentTime >= 60000)) {
        scheduler.persist();
        delay(100);
        ESP.restart();
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
    
    unsigned long uptime = millis() - bootTime;
    if (uptime >= PERIODIC_RESTART_INTERVAL_MS) {
        scheduler.persist();
        delay(100);
        ESP.restart();
    }
    
    yield();
    loopCounter++;
    delayMicroseconds(100);
}

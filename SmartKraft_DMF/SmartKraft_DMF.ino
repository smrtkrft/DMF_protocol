#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <DNSServer.h>

#include "config_store.h"
#include "scheduler.h"
#include "network_manager.h"
#include "mail_functions.h"
#include "web_handlers.h"
#include "test_functions.h"

constexpr uint8_t BUTTON_PIN = 3;
constexpr uint8_t RELAY_PIN = 10;
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
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "SmartKraft-DMF%06llX", (unsigned long long)(mac & 0xFFFFFFULL));
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
    
    Serial.println(F("[Timer] Fiziksel/sanal buton ile geri sayım yeniden başlatıldı"));
}

// performGetRequest() fonksiyonu KALDIRILDI
// URL tetikleme artık mail_functions.cpp içinde yapılıyor (güvenli + validation)

void processAlarms() {
    uint8_t alarmIndex = 0;

    if (scheduler.alarmDue(alarmIndex)) {
        ScheduleSnapshot snap = scheduler.snapshot();
        Serial.printf("[Alarm] %u numaralı alarm tetiklendi\n", alarmIndex + 1);
        networkManager.ensureConnected(true);
        
        // URL tetikleme mail_functions.cpp içinde yapılıyor (güvenli + validation)
        
        String error;
        if (!mailAgent.sendWarning(alarmIndex, snap, error)) {
            Serial.printf("[Mail] Uyarı maili gönderilemedi: %s\n", error.c_str());
        } else {
            Serial.println(F("[Mail] Uyarı maili gönderildi (sadece gönderen adrese)"));
        }
        scheduler.acknowledgeAlarm(alarmIndex);
        scheduler.persist();
    }

    if (scheduler.finalDue()) {
        ScheduleSnapshot snap = scheduler.snapshot();
        Serial.println(F("[Final] Süre doldu, röle tetikleniyor"));
        latchRelay(true);
        networkManager.ensureConnected(true);
        
        // URL tetikleme mail_functions.cpp içinde yapılıyor (güvenli + validation)
        
        String error;
        if (!mailAgent.sendFinal(snap, error)) {
            Serial.printf("[Mail] Final maili gönderilemedi: %s\n", error.c_str());
        } else {
            Serial.println(F("[Mail] Final maili gönderildi (tüm alıcılara ayrı ayrı)"));
            
            // ⚠️ YENİ: Final mail başarıyla gönderildikten sonra 60 saniye bekle ve reboot
            if (!finalMailSent) {
                finalMailSent = true;
                finalMailSentTime = millis();
                Serial.println(F("[Reboot] Final mail gönderildi - 60 saniye sonra cihaz yeniden başlatılacak"));
            }
        }
        scheduler.acknowledgeFinal();
        scheduler.persist();
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
    Serial.println();
    Serial.println(F("=== SmartKraft DMF Başlıyor ==="));

    deviceId = generateDeviceId();
    Serial.printf("[Cihaz] ID: %s\n", deviceId.c_str());
    randomSeed(esp_random());

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
    webUI.begin(&webServer, &configStore, &scheduler, &mailAgent, &networkManager, deviceId, &dnsServer);
    
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
    
    delay(1000);
    Serial.println(F("[Init] WebServer başlatılıyor..."));
    webUI.startServer();
    
    // WiFi sleep mode'u devre dışı bırak (light sleep engellensin)
    WiFi.setSleep(WIFI_PS_NONE);
    Serial.println(F("[Init] WiFi sleep mode: NONE (always awake)"));
    
    Serial.println(F("[Init] Sistem hazır!"));
}

void loop() {
    webUI.loop();
    scheduler.tick();
    handleButton();
    processAlarms();
    testInterface.processSerial();

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

    if (millis() - lastPersist > STATUS_PERSIST_INTERVAL_MS) {
        scheduler.persist();
        lastPersist = millis();
    }
}

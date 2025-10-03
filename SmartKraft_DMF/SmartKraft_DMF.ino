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
    Serial.println(F("[Timer] Fiziksel/sanal buton ile geri sayım yeniden başlatıldı"));
}

bool performGetRequest(const String &url) {
    if (url.length() == 0) {
        return true;
    }
    HTTPClient client;
    WiFiClientSecure secure;
    secure.setInsecure();
    if (url.startsWith("https://")) {
        if (!client.begin(secure, url)) {
            Serial.println(F("[HTTP] HTTPS başlatılamadı"));
            return false;
        }
    } else {
        if (!client.begin(url)) {
            Serial.println(F("[HTTP] HTTP başlatılamadı"));
            return false;
        }
    }
    int code = client.GET();
    client.end();
    Serial.printf("[HTTP] %s -> %d\n", url.c_str(), code);
    return code > 0 && code < 400;
}

void processAlarms() {
    uint8_t alarmIndex = 0;

    if (scheduler.alarmDue(alarmIndex)) {
        ScheduleSnapshot snap = scheduler.snapshot();
        Serial.printf("[Alarm] %u numaralı alarm tetiklendi\n", alarmIndex + 1);
        MailSettings settings = mailAgent.currentConfig();
        networkManager.ensureConnected(true);
        if (settings.warning.getUrl.length()) {
            performGetRequest(settings.warning.getUrl);
        }
        String error;
        if (!mailAgent.sendWarning(alarmIndex, snap, error)) {
            Serial.printf("[Mail] Uyarı maili gönderilemedi: %s\n", error.c_str());
        } else {
            Serial.println(F("[Mail] Uyarı maili gönderildi"));
        }
        scheduler.acknowledgeAlarm(alarmIndex);
        scheduler.persist();
    }

    if (scheduler.finalDue()) {
        ScheduleSnapshot snap = scheduler.snapshot();
        Serial.println(F("[Final] Süre doldu, röle tetikleniyor"));
        latchRelay(true);
        MailSettings settings = mailAgent.currentConfig();
        networkManager.ensureConnected(true);
        if (settings.finalContent.getUrl.length()) {
            performGetRequest(settings.finalContent.getUrl);
        }
        String error;
        if (!mailAgent.sendFinal(snap, error)) {
            Serial.printf("[Mail] Final maili gönderilemedi: %s\n", error.c_str());
        } else {
            Serial.println(F("[Mail] Final maili gönderildi"));
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
    mailAgent.begin(&configStore, &networkManager);
    
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
    
    Serial.println(F("[Init] Sistem hazır!"));
}

void loop() {
    webUI.loop();
    scheduler.tick();
    handleButton();
    processAlarms();
    testInterface.processSerial();

    if (millis() - lastPersist > STATUS_PERSIST_INTERVAL_MS) {
        scheduler.persist();
        lastPersist = millis();
    }
}

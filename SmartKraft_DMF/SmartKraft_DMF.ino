#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <esp_system.h>
#include <DNSServer.h>
#include <esp_task_wdt.h>  // Watchdog timer için

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
    
    finalMailSent = false;
    finalMailSentTime = 0;
}

// performGetRequest() fonksiyonu KALDIRILDI
// URL tetikleme artık mail_functions.cpp içinde yapılıyor (güvenli + validation)

void processAlarms() {
    uint8_t alarmIndex = 0;

    if (scheduler.alarmDue(alarmIndex)) {
        ScheduleSnapshot snap = scheduler.snapshot();
        
        if (!networkManager.isConnected()) {
            networkManager.ensureConnected(true);
        }
        
        String error;
        mailAgent.sendWarning(alarmIndex, snap, error);
        
        scheduler.acknowledgeAlarm(alarmIndex);
        scheduler.persist();
        yield();
    }

    if (scheduler.finalDue()) {
        ScheduleSnapshot snap = scheduler.snapshot();
        latchRelay(true);
        
        if (!networkManager.isConnected()) {
            networkManager.ensureConnected(true);
        }
        
        String error;
        if (mailAgent.sendFinal(snap, error)) {
            if (!finalMailSent) {
                finalMailSent = true;
                finalMailSentTime = millis();
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
                resetTimerFromButton();
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println(F("\n=== SmartKraft DMF v1.0 ==="));

    deviceId = generateDeviceId();
    Serial.printf("ID: %s\n", deviceId.c_str());
    randomSeed(esp_random());

    initHardware();

    if (!configStore.begin()) {
        Serial.println(F("[ERROR] Filesystem failed"));
    }

    scheduler.begin(&configStore);
    networkManager.begin(&configStore);
    mailAgent.begin(&configStore, &networkManager, deviceId);
    
    String apName = generateAPName();
    webUI.begin(&webServer, &configStore, &scheduler, &mailAgent, &networkManager, deviceId, &dnsServer, apName);
    testInterface.begin(&scheduler, &mailAgent);

    ScheduleSnapshot initialSnap = scheduler.snapshot();
    if (!initialSnap.timerActive) {
        latchRelay(false);
    }

    lastButtonState = digitalRead(BUTTON_PIN);
    lastButtonChange = millis();
    lastPersist = millis();
    
    webUI.startServer();
    WiFi.setSleep(WIFI_PS_NONE);
    
    Serial.println(F("[READY]\n"));
}

void loop() {
    static unsigned long lastLoop = 0;
    static unsigned long loopCounter = 0;
    
    webUI.loop();
    scheduler.tick();
    handleButton();
    
    if (millis() - lastLoop > 500) {
        processAlarms();
        lastLoop = millis();
    }
    
    if (loopCounter % 100 == 0) {
        testInterface.processSerial();
    }

    if (finalMailSent && (millis() - finalMailSentTime >= 60000)) {
        Serial.println(F("[REBOOT]"));
        delay(500);
        ESP.restart();
    }

    if (millis() - lastPersist > STATUS_PERSIST_INTERVAL_MS) {
        scheduler.persist();
        lastPersist = millis();
    }
    
    yield();
    loopCounter++;
    delayMicroseconds(100);
}

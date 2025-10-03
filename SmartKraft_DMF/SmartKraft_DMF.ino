#include <Arduino.h>#include <Arduino.h>

#include <WiFi.h>#include <WiFi.h>

#include <HTTPClient.h>#include <HTTPClient.h>

#include <WiFiClientSecure.h>#include <WiFiClientSecure.h>

#include <LittleFS.h>#include <LittleFS.h>

#include <esp_system.h>#include <esp_system.h>

#include <DNSServer.h>#include <DNSServer.h>



#include "config_store.h"#include "config_store.h"

#include "scheduler.h"#include "scheduler.h"

#include "network_manager.h"#include "network_manager.h"

#include "mail_functions.h"#include "mail_functions.h"

#include "web_handlers.h"#include "web_handlers.h"

#include "test_functions.h"#include "test_functions.h"



constexpr uint8_t BUTTON_PIN = 3;        // Dahili buton (GND'ye çekilecek)constexpr uint8_t BUTTON_PIN = 3;        // Dahili buton (GND'ye çekilecek)

constexpr uint8_t RELAY_PIN = 10;        // Röle çıkışıconstexpr uint8_t RELAY_PIN = 10;        // Röle çıkışı

constexpr uint32_t BUTTON_DEBOUNCE_MS = 200;constexpr uint32_t BUTTON_DEBOUNCE_MS = 200;

constexpr uint32_t STATUS_PERSIST_INTERVAL_MS = 60000;constexpr uint32_t STATUS_PERSIST_INTERVAL_MS = 60000;



ConfigStore configStore;ConfigStore configStore;

CountdownScheduler scheduler;CountdownScheduler scheduler;

DMFNetworkManager networkManager;DMFNetworkManager networkManager;

MailAgent mailAgent;MailAgent mailAgent;

WebServer webServer(80);WebServer webServer(80);

WebInterface webUI;WebInterface webUI;

TestInterface testInterface;TestInterface testInterface;

DNSServer dnsServer;DNSServer dnsServer;



bool relayLatched = false;bool relayLatched = false;

unsigned long lastButtonChange = 0;unsigned long lastButtonChange = 0;

bool lastButtonState = true;bool lastButtonState = true;

unsigned long lastPersist = 0;unsigned long lastPersist = 0;



String deviceId;String deviceId;



String generateDeviceId() {String generateDeviceId() {

    uint64_t mac = ESP.getEfuseMac();    uint64_t mac = ESP.getEfuseMac();

    char buffer[32];    char buffer[32];

    snprintf(buffer, sizeof(buffer), "SmartKraft-DMF%06llX", (unsigned long long)(mac & 0xFFFFFFULL));    snprintf(buffer, sizeof(buffer), "SmartKraft-DMF%06llX", (unsigned long long)(mac & 0xFFFFFFULL));

    return String(buffer);    return String(buffer);

}}



void initHardware() {void initHardware() {

    pinMode(BUTTON_PIN, INPUT_PULLUP);    pinMode(BUTTON_PIN, INPUT_PULLUP);

    pinMode(RELAY_PIN, OUTPUT);    pinMode(RELAY_PIN, OUTPUT);

    digitalWrite(RELAY_PIN, LOW);    digitalWrite(RELAY_PIN, LOW);

    relayLatched = false;    relayLatched = false;

}}



void latchRelay(bool state) {void latchRelay(bool state) {

    relayLatched = state;    relayLatched = state;

    digitalWrite(RELAY_PIN, state ? HIGH : LOW);    digitalWrite(RELAY_PIN, state ? HIGH : LOW);

}}



void resetTimerFromButton() {void resetTimerFromButton() {

    latchRelay(false);    latchRelay(false);

    scheduler.reset();    scheduler.reset();

    scheduler.start();    scheduler.start();

    scheduler.persist();    scheduler.persist();

    Serial.println(F("[Timer] Fiziksel/sanal buton ile geri sayım yeniden başlatıldı"));    Serial.println(F("[Timer] Fiziksel/sanal buton ile geri sayım yeniden başlatıldı"));

}}



bool performGetRequest(const String &url) {bool performGetRequest(const String &url) {

    if (url.length() == 0) {    if (url.length() == 0) {

        return true;        return true;

    }    }

    HTTPClient client;    HTTPClient client;

    WiFiClientSecure secure;    WiFiClientSecure secure;

    secure.setInsecure();    secure.setInsecure();

    if (url.startsWith("https://")) {    if (url.startsWith("https://")) {

        if (!client.begin(secure, url)) {        if (!client.begin(secure, url)) {

            Serial.println(F("[HTTP] HTTPS başlatılamadı"));            Serial.println(F("[HTTP] HTTPS başlatılamadı"));

            return false;            return false;

        }        }

    } else {    } else {

        if (!client.begin(url)) {        if (!client.begin(url)) {

            Serial.println(F("[HTTP] HTTP başlatılamadı"));            Serial.println(F("[HTTP] HTTP başlatılamadı"));

            return false;            return false;

        }        }

    }    }

    int code = client.GET();    int code = client.GET();

    client.end();    client.end();

    Serial.printf("[HTTP] %s -> %d\n", url.c_str(), code);    Serial.printf("[HTTP] %s -> %d\n", url.c_str(), code);

    return code > 0 && code < 400;    return code > 0 && code < 400;

}}



void processAlarms() {void processAlarms() {

    uint8_t alarmIndex = 0;    uint8_t alarmIndex = 0;



    if (scheduler.alarmDue(alarmIndex)) {    if (scheduler.alarmDue(alarmIndex)) {

        ScheduleSnapshot snap = scheduler.snapshot();        ScheduleSnapshot snap = scheduler.snapshot();

        Serial.printf("[Alarm] %u numaralı alarm tetiklendi\n", alarmIndex + 1);        Serial.printf("[Alarm] %u numaralı alarm tetiklendi\n", alarmIndex + 1);

        MailSettings settings = mailAgent.currentConfig();        MailSettings settings = mailAgent.currentConfig();

        networkManager.ensureConnected(true);        networkManager.ensureConnected(true);

        if (settings.warning.getUrl.length()) {        if (settings.warning.getUrl.length()) {

            performGetRequest(settings.warning.getUrl);            performGetRequest(settings.warning.getUrl);

        }        }

        String error;        String error;

        if (!mailAgent.sendWarning(alarmIndex, snap, error)) {        if (!mailAgent.sendWarning(alarmIndex, snap, error)) {

            Serial.printf("[Mail] Uyarı maili gönderilemedi: %s\n", error.c_str());            Serial.printf("[Mail] Uyarı maili gönderilemedi: %s\n", error.c_str());

        } else {        } else {

            Serial.println(F("[Mail] Uyarı maili gönderildi"));            Serial.println(F("[Mail] Uyarı maili gönderildi"));

        }        }

        scheduler.acknowledgeAlarm(alarmIndex);        scheduler.acknowledgeAlarm(alarmIndex);

        scheduler.persist();        scheduler.persist();

    }    }



    if (scheduler.finalDue()) {    if (scheduler.finalDue()) {

        ScheduleSnapshot snap = scheduler.snapshot();        ScheduleSnapshot snap = scheduler.snapshot();

        Serial.println(F("[Final] Süre doldu, röle tetikleniyor"));        Serial.println(F("[Final] Süre doldu, röle tetikleniyor"));

        latchRelay(true);        latchRelay(true);

        MailSettings settings = mailAgent.currentConfig();        MailSettings settings = mailAgent.currentConfig();

        networkManager.ensureConnected(true);        networkManager.ensureConnected(true);

        if (settings.finalContent.getUrl.length()) {        if (settings.finalContent.getUrl.length()) {

            performGetRequest(settings.finalContent.getUrl);            performGetRequest(settings.finalContent.getUrl);

        }        }

        String error;        String error;

        if (!mailAgent.sendFinal(snap, error)) {        if (!mailAgent.sendFinal(snap, error)) {

            Serial.printf("[Mail] Final maili gönderilemedi: %s\n", error.c_str());            Serial.printf("[Mail] Final maili gönderilemedi: %s\n", error.c_str());

        } else {        } else {

            Serial.println(F("[Mail] Final maili gönderildi"));            Serial.println(F("[Mail] Final maili gönderildi"));

        }        }

        scheduler.acknowledgeFinal();        scheduler.acknowledgeFinal();

        scheduler.persist();        scheduler.persist();

    }    }

}}



void handleButton() {void handleButton() {

    bool state = digitalRead(BUTTON_PIN);    bool state = digitalRead(BUTTON_PIN);

    if (state != lastButtonState) {    if (state != lastButtonState) {

        if (millis() - lastButtonChange > BUTTON_DEBOUNCE_MS) {        if (millis() - lastButtonChange > BUTTON_DEBOUNCE_MS) {

            lastButtonChange = millis();            lastButtonChange = millis();

            lastButtonState = state;            lastButtonState = state;

            if (state == LOW) {            if (state == LOW) {

                resetTimerFromButton();                resetTimerFromButton();

            }            }

        }        }

    }    }

}}



void setup() {void setup() {

    Serial.begin(115200);    Serial.begin(115200);

    delay(100);    delay(100);

    Serial.println();    Serial.println();

    Serial.println(F("=== SmartKraft DMF Başlıyor ==="));    Serial.println(F("=== SmartKraft DMF Başlıyor ==="));



    deviceId = generateDeviceId();    deviceId = generateDeviceId();

    Serial.printf("[Cihaz] ID: %s\n", deviceId.c_str());    Serial.printf("[Cihaz] ID: %s\n", deviceId.c_str());

    randomSeed(esp_random());    randomSeed(esp_random());



    Serial.println(F("[Init] Donanım başlatılıyor..."));    Serial.println(F("[Init] Donanım başlatılıyor..."));

    initHardware();    initHardware();



    Serial.println(F("[Init] Dosya sistemi başlatılıyor..."));    Serial.println(F("[Init] Dosya sistemi başlatılıyor..."));

    if (!configStore.begin()) {    if (!configStore.begin()) {

        Serial.println(F("[FS] Dosya sistemi başlatılamadı"));        Serial.println(F("[FS] Dosya sistemi başlatılamadı"));

    }    }



    Serial.println(F("[Init] Scheduler başlatılıyor..."));    Serial.println(F("[Init] Scheduler başlatılıyor..."));

    scheduler.begin(&configStore);    scheduler.begin(&configStore);

        

    Serial.println(F("[Init] Network manager başlatılıyor..."));    Serial.println(F("[Init] Network manager başlatılıyor..."));

    networkManager.begin(&configStore);    networkManager.begin(&configStore);

        

    Serial.println(F("[Init] Mail agent başlatılıyor..."));    Serial.println(F("[Init] Mail agent başlatılıyor..."));

    mailAgent.begin(&configStore, &networkManager);    mailAgent.begin(&configStore, &networkManager);

        

    Serial.println(F("[Init] Web sunucusu başlatılıyor..."));    Serial.println(F("[Init] Web sunucusu başlatılıyor..."));

    webUI.begin(&webServer, &configStore, &scheduler, &mailAgent, &networkManager, deviceId, &dnsServer);    webUI.begin(&webServer, &configStore, &scheduler, &mailAgent, &networkManager, deviceId, &dnsServer);

        

    Serial.println(F("[Init] Test arayüzü başlatılıyor..."));    Serial.println(F("[Init] Test arayüzü başlatılıyor..."));

    testInterface.begin(&scheduler, &mailAgent);    testInterface.begin(&scheduler, &mailAgent);



    Serial.println(F("[Init] Başlangıç tamamlandı, WiFi devre dışı"));    Serial.println(F("[Init] Başlangıç tamamlandı, WiFi devre dışı"));

    // WiFi bağlantısını setup'ta başlatmıyoruz - ihtiyaç anında bağlanacak    // WiFi bağlantısını setup'ta başlatmıyoruz - ihtiyaç anında bağlanacak

        

    ScheduleSnapshot initialSnap = scheduler.snapshot();    ScheduleSnapshot initialSnap = scheduler.snapshot();



    if (initialSnap.timerActive) {    if (initialSnap.timerActive) {

        Serial.println(F("[Timer] Kalan süre ile devam ediliyor"));        Serial.println(F("[Timer] Kalan süre ile devam ediliyor"));

    } else {    } else {

        latchRelay(false);        latchRelay(false);

    }    }



    lastButtonState = digitalRead(BUTTON_PIN);    lastButtonState = digitalRead(BUTTON_PIN);

    lastButtonChange = millis();    lastButtonChange = millis();

    lastPersist = millis();    lastPersist = millis();

        

    // WebServer'ı gecikmeli başlat    // WebServer'ı gecikmeli başlat

    delay(1000);    delay(1000);

    Serial.println(F("[Init] WebServer başlatılıyor..."));    Serial.println(F("[Init] WebServer başlatılıyor..."));

    webUI.startServer();    webUI.startServer();

        

    Serial.println(F("[Init] Sistem hazır!"));    Serial.println(F("[Init] Sistem hazır!"));

}}



void loop() {void loop() {

    webUI.loop();    webUI.loop();

    scheduler.tick();    scheduler.tick();

    handleButton();    handleButton();

    processAlarms();    processAlarms();

    testInterface.processSerial();    testInterface.processSerial();



    if (millis() - lastPersist > STATUS_PERSIST_INTERVAL_MS) {    if (millis() - lastPersist > STATUS_PERSIST_INTERVAL_MS) {

        scheduler.persist();        scheduler.persist();

        lastPersist = millis();        lastPersist = millis();

    }    }

}}


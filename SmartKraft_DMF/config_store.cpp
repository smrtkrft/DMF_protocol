#include "config_store.h"

namespace {
constexpr size_t JSON_CAPACITY = 4096;
}

bool ConfigStore::begin() {
    if (LittleFS.begin(false)) {
        ensureDataFolder();
        return true;
    }

    Serial.println(F("[FS] LittleFS mount başarısız, format denenecek"));
    if (LittleFS.begin(true)) {
        Serial.println(F("[FS] LittleFS yeniden biçimlendirildi"));
        ensureDataFolder();
        return true;
    }

    Serial.println(F("[FS] LittleFS format başarısız"));
    return false;
}

TimerSettings ConfigStore::loadTimerSettings() const {
    TimerSettings settings;
    StaticJsonDocument<JSON_CAPACITY> doc;
    if (readJson(TIMER_FILE, doc)) {
        settings.unit = doc["unit"].as<uint8_t>() == 0 ? TimerSettings::HOURS : TimerSettings::DAYS;
        settings.totalValue = doc["totalValue"].as<uint16_t>();
        settings.alarmCount = doc["alarmCount"].as<uint8_t>();
        settings.enabled = doc["enabled"].as<bool>();
    }
    settings.totalValue = constrain(settings.totalValue, (uint16_t)1, (uint16_t)60);
    settings.alarmCount = constrain(settings.alarmCount, (uint8_t)0, (uint8_t)10);
    return settings;
}

void ConfigStore::saveTimerSettings(const TimerSettings &settings) {
    StaticJsonDocument<JSON_CAPACITY> doc;
    doc["unit"] = settings.unit == TimerSettings::HOURS ? 0 : 1;
    doc["totalValue"] = settings.totalValue;
    doc["alarmCount"] = settings.alarmCount;
    doc["enabled"] = settings.enabled;
    writeJson(TIMER_FILE, doc);
}

MailSettings ConfigStore::loadMailSettings() const {
    MailSettings mail;
    StaticJsonDocument<JSON_CAPACITY> doc;
    if (readJson(MAIL_FILE, doc)) {
        mail.smtpServer = doc["smtpServer"].as<String>();
        mail.smtpPort = doc["smtpPort"] | 465;
        mail.username = doc["username"].as<String>();
        mail.password = doc["password"].as<String>();

        if (doc.containsKey("recipients") && doc["recipients"].is<JsonArray>()) {
            auto array = doc["recipients"].as<JsonArray>();
            mail.recipientCount = min((uint8_t)array.size(), (uint8_t)MAX_RECIPIENTS);
            for (uint8_t i = 0; i < mail.recipientCount; ++i) {
                mail.recipients[i] = array[i].as<String>();
            }
        }

        mail.warning.subject = doc["warning"]["subject"].as<String>();
        mail.warning.body = doc["warning"]["body"].as<String>();
        mail.warning.getUrl = doc["warning"]["getUrl"].as<String>();

        mail.finalContent.subject = doc["final"]["subject"].as<String>();
        mail.finalContent.body = doc["final"]["body"].as<String>();
        mail.finalContent.getUrl = doc["final"]["getUrl"].as<String>();

        if (doc.containsKey("attachments") && doc["attachments"].is<JsonArray>()) {
            auto array = doc["attachments"].as<JsonArray>();
            mail.attachmentCount = min((uint8_t)array.size(), (uint8_t)MAX_ATTACHMENTS);
            for (uint8_t i = 0; i < mail.attachmentCount; ++i) {
                auto entry = array[i];
                strlcpy(mail.attachments[i].displayName, entry["displayName"].as<const char*>(), MAX_FILENAME_LEN);
                strlcpy(mail.attachments[i].storedPath, entry["storedPath"].as<const char*>(), MAX_PATH_LEN);
                mail.attachments[i].size = entry["size"].as<uint32_t>();
                mail.attachments[i].forWarning = entry["forWarning"].as<bool>();
                mail.attachments[i].forFinal = entry["forFinal"].as<bool>();
            }
        }
    }
    return mail;
}

void ConfigStore::saveMailSettings(const MailSettings &mail) {
    StaticJsonDocument<JSON_CAPACITY> doc;
    doc["smtpServer"] = mail.smtpServer;
    doc["smtpPort"] = mail.smtpPort;
    doc["username"] = mail.username;
    doc["password"] = mail.password;

    auto recipients = doc.createNestedArray("recipients");
    for (uint8_t i = 0; i < mail.recipientCount; ++i) {
        recipients.add(mail.recipients[i]);
    }

    auto warning = doc.createNestedObject("warning");
    warning["subject"] = mail.warning.subject;
    warning["body"] = mail.warning.body;
    warning["getUrl"] = mail.warning.getUrl;

    auto finalObj = doc.createNestedObject("final");
    finalObj["subject"] = mail.finalContent.subject;
    finalObj["body"] = mail.finalContent.body;
    finalObj["getUrl"] = mail.finalContent.getUrl;

    auto attachments = doc.createNestedArray("attachments");
    for (uint8_t i = 0; i < mail.attachmentCount; ++i) {
        auto entry = attachments.createNestedObject();
        entry["displayName"] = mail.attachments[i].displayName;
        entry["storedPath"] = mail.attachments[i].storedPath;
        entry["size"] = mail.attachments[i].size;
        entry["forWarning"] = mail.attachments[i].forWarning;
        entry["forFinal"] = mail.attachments[i].forFinal;
    }

    writeJson(MAIL_FILE, doc);
}

WiFiSettings ConfigStore::loadWiFiSettings() const {
    WiFiSettings wifi;
    StaticJsonDocument<JSON_CAPACITY> doc;
    if (readJson(WIFI_FILE, doc)) {
        wifi.primarySSID = doc["primarySSID"].as<String>();
        wifi.primaryPassword = doc["primaryPassword"].as<String>();
        wifi.secondarySSID = doc["secondarySSID"].as<String>();
        wifi.secondaryPassword = doc["secondaryPassword"].as<String>();
        wifi.allowOpenNetworks = doc["allowOpenNetworks"].as<bool>();
    }
    return wifi;
}

void ConfigStore::saveWiFiSettings(const WiFiSettings &wifi) {
    StaticJsonDocument<JSON_CAPACITY> doc;
    doc["primarySSID"] = wifi.primarySSID;
    doc["primaryPassword"] = wifi.primaryPassword;
    doc["secondarySSID"] = wifi.secondarySSID;
    doc["secondaryPassword"] = wifi.secondaryPassword;
    doc["allowOpenNetworks"] = wifi.allowOpenNetworks;
    writeJson(WIFI_FILE, doc);
}

TimerRuntime ConfigStore::loadRuntime() const {
    TimerRuntime runtime;
    StaticJsonDocument<JSON_CAPACITY> doc;
    if (readJson(RUNTIME_FILE, doc)) {
        runtime.timerActive = doc["timerActive"].as<bool>();
        runtime.paused = doc["paused"].as<bool>();
        runtime.deadlineMillis = doc["deadlineMillis"].as<uint64_t>();
        runtime.remainingSeconds = doc["remainingSeconds"].as<uint32_t>();
        runtime.nextAlarmIndex = doc["nextAlarmIndex"].as<uint8_t>();
        runtime.finalTriggered = doc["finalTriggered"].as<bool>();
    }
    return runtime;
}

void ConfigStore::saveRuntime(const TimerRuntime &runtime) {
    StaticJsonDocument<JSON_CAPACITY> doc;
    doc["timerActive"] = runtime.timerActive;
    doc["paused"] = runtime.paused;
    doc["deadlineMillis"] = runtime.deadlineMillis;
    doc["remainingSeconds"] = runtime.remainingSeconds;
    doc["nextAlarmIndex"] = runtime.nextAlarmIndex;
    doc["finalTriggered"] = runtime.finalTriggered;
    writeJson(RUNTIME_FILE, doc);
}

void ConfigStore::eraseAll() {
    LittleFS.remove(TIMER_FILE);
    LittleFS.remove(MAIL_FILE);
    LittleFS.remove(WIFI_FILE);
    LittleFS.remove(RUNTIME_FILE);
    File dir = LittleFS.open(dataFolder(), "r");
    if (dir) {
        File file = dir.openNextFile();
        while (file) {
            String path = String(dataFolder()) + "/" + file.name();
            LittleFS.remove(path);
            file = dir.openNextFile();
        }
    }
}

bool ConfigStore::ensureDataFolder() {
    if (!LittleFS.exists(dataFolder())) {
        return LittleFS.mkdir(dataFolder());
    }
    return true;
}

void ConfigStore::writeJson(const char *path, const JsonDocument &doc) {
    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.printf("[FS] %s yazılamadı\n", path);
        return;
    }
    serializeJson(doc, file);
    file.close();
}

bool ConfigStore::readJson(const char *path, JsonDocument &doc) const {
    if (!LittleFS.exists(path)) {
        return false;
    }
    File file = LittleFS.open(path, "r");
    if (!file) {
        Serial.printf("[FS] %s açılamadı\n", path);
        return false;
    }
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    return !err;
}

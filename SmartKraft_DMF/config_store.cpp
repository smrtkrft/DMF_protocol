#include "config_store.h"

namespace {
constexpr size_t JSON_CAPACITY = 4096;
}

bool ConfigStore::begin() {
    // Önce normal mount dene
    if (LittleFS.begin(false)) {
        if (!ensureDataFolder()) {
            Serial.println(F("[FS] UYARI: Data klasörü oluşturulamadı, ancak devam ediliyor"));
        }
        return true;
    }

    // Mount başarısız - format dene
    Serial.println(F("[FS] LittleFS mount başarısız, format denenecek"));
    if (LittleFS.begin(true)) {
        Serial.println(F("[FS] LittleFS yeniden biçimlendirildi"));
        if (!ensureDataFolder()) {
            Serial.println(F("[FS] UYARI: Data klasörü oluşturulamadı"));
        }
        return true;
    }

    // Format da başarısız - kritik hata ama sistem çalışmaya devam etsin
    Serial.println(F("[FS] KRITIK: LittleFS başlatılamadı - ayarlar RAM'de tutulacak"));
    return false; // Sistem çalışmaya devam eder ama ayarlar kaydedilmez
}

TimerSettings ConfigStore::loadTimerSettings() const {
    TimerSettings settings;
    StaticJsonDocument<JSON_CAPACITY> doc;
    if (readJson(TIMER_FILE, doc)) {
        uint8_t unitValue = doc["unit"].as<uint8_t>();
        
        // 0=MINUTES, 1=HOURS, 2=DAYS
        if (unitValue == 0) {
            settings.unit = TimerSettings::MINUTES;
        } else if (unitValue == 1) {
            settings.unit = TimerSettings::HOURS;
        } else {
            settings.unit = TimerSettings::DAYS;
        }
        
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
    
    // 0=MINUTES, 1=HOURS, 2=DAYS
    if (settings.unit == TimerSettings::MINUTES) {
        doc["unit"] = 0;
    } else if (settings.unit == TimerSettings::HOURS) {
        doc["unit"] = 1;
    } else {
        doc["unit"] = 2;
    }
    
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
        
        // apModeEnabled: Eğer JSON'da yoksa varsayılan true, varsa kaydedilen değeri kullan
        if (doc.containsKey("apModeEnabled")) {
            wifi.apModeEnabled = doc["apModeEnabled"].as<bool>();
        } else {
            wifi.apModeEnabled = true; // İlk kurulum için varsayılan
        }

        wifi.primaryStaticEnabled = doc["primaryStaticEnabled"].as<bool>();
        wifi.primaryIP = doc["primaryIP"].as<String>();
        wifi.primaryGateway = doc["primaryGateway"].as<String>();
        wifi.primarySubnet = doc["primarySubnet"].as<String>();
        wifi.primaryDNS = doc["primaryDNS"].as<String>();

        wifi.secondaryStaticEnabled = doc["secondaryStaticEnabled"].as<bool>();
        wifi.secondaryIP = doc["secondaryIP"].as<String>();
        wifi.secondaryGateway = doc["secondaryGateway"].as<String>();
        wifi.secondarySubnet = doc["secondarySubnet"].as<String>();
        wifi.secondaryDNS = doc["secondaryDNS"].as<String>();
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
    doc["apModeEnabled"] = wifi.apModeEnabled;

    doc["primaryStaticEnabled"] = wifi.primaryStaticEnabled;
    doc["primaryIP"] = wifi.primaryIP;
    doc["primaryGateway"] = wifi.primaryGateway;
    doc["primarySubnet"] = wifi.primarySubnet;
    doc["primaryDNS"] = wifi.primaryDNS;

    doc["secondaryStaticEnabled"] = wifi.secondaryStaticEnabled;
    doc["secondaryIP"] = wifi.secondaryIP;
    doc["secondaryGateway"] = wifi.secondaryGateway;
    doc["secondarySubnet"] = wifi.secondarySubnet;
    doc["secondaryDNS"] = wifi.secondaryDNS;
    writeJson(WIFI_FILE, doc);
}

// ⚠️ YENİ: API Ayarlarını Yükle
APISettings ConfigStore::loadAPISettings() const {
    APISettings settings;
    StaticJsonDocument<512> doc;
    if (readJson(API_FILE, doc)) {
        settings.enabled = doc["enabled"] | true;
        settings.endpoint = doc["endpoint"] | "trigger";
        settings.requireToken = doc["requireToken"] | false;
        settings.token = doc["token"] | "";
    }
    return settings;
}

// ⚠️ YENİ: API Ayarlarını Kaydet
void ConfigStore::saveAPISettings(const APISettings &settings) {
    StaticJsonDocument<512> doc;
    doc["enabled"] = settings.enabled;
    doc["endpoint"] = settings.endpoint;
    doc["requireToken"] = settings.requireToken;
    doc["token"] = settings.token;
    writeJson(API_FILE, doc);
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
    // LittleFS mount kontrolü
    if (!LittleFS.begin(false)) {
        Serial.printf("[FS] UYARI: %s yazılamadı - LittleFS erişilemiyor\n", path);
        return;
    }
    
    // Data klasörünü kontrol et ve oluştur
    if (!ensureDataFolder()) {
        Serial.printf("[FS] UYARI: %s yazılamadı - data klasörü oluşturulamadı\n", path);
        return;
    }
    
    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.printf("[FS] UYARI: %s yazılamadı - dosya açılamadı\n", path);
        return;
    }
    
    size_t written = serializeJson(doc, file);
    file.close();
    
    if (written == 0) {
        Serial.printf("[FS] UYARI: %s boş yazıldı\n", path);
    } else {
        Serial.printf("[FS] %s kaydedildi (%d byte)\n", path, written);
    }
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

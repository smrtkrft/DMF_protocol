#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

struct TimerSettings {
    enum Unit : uint8_t { MINUTES = 0, HOURS = 1, DAYS = 2 };

    Unit unit = DAYS;
    uint16_t totalValue = 7; // minutes, hours or days depending on unit
    uint8_t alarmCount = 3;
    bool enabled = true;
};

struct WarningContent {
    String subject = "SmartKraft DMF Uyarısı";
    String body = "Süre dolmak üzere.";
    String getUrl = "";
};

struct FinalContent {
    String subject = "SmartKraft DMF Final";
    String body = "Süre doldu.";
    String getUrl = "";
};

static const size_t MAX_RECIPIENTS = 10;
static const size_t MAX_ATTACHMENTS = 5;
static const size_t MAX_FILENAME_LEN = 48;
static const size_t MAX_PATH_LEN = 64;

struct AttachmentMeta {
    char displayName[MAX_FILENAME_LEN] = {0};
    char storedPath[MAX_PATH_LEN] = {0};
    size_t size = 0;
    bool forWarning = false;
    bool forFinal = true;
};

struct MailSettings {
    String smtpServer = "smtp.protonmail.ch";
    uint16_t smtpPort = 465; // TLS/SSL port (önerilen)
    String username = "";
    String password = ""; // Proton app password

    String recipients[MAX_RECIPIENTS];
    uint8_t recipientCount = 0;

    WarningContent warning;
    FinalContent finalContent;

    AttachmentMeta attachments[MAX_ATTACHMENTS];
    uint8_t attachmentCount = 0;
};

struct WiFiSettings {
    String primarySSID = "";
    String primaryPassword = "";
    String secondarySSID = "";
    String secondaryPassword = "";
    bool allowOpenNetworks = true;
    bool apModeEnabled = true; // Kullanıcı AP modunu kapatabilir

    // Primary statik IP ayarları
    bool primaryStaticEnabled = false;
    String primaryIP = "";       // "192.168.1.50"
    String primaryGateway = "";  // "192.168.1.1"
    String primarySubnet = "";   // "255.255.255.0"
    String primaryDNS = "";      // opsiyonel

    // Secondary statik IP ayarları
    bool secondaryStaticEnabled = false;
    String secondaryIP = "";
    String secondaryGateway = "";
    String secondarySubnet = "";
    String secondaryDNS = "";
};

struct TimerRuntime {
    bool timerActive = false;
    bool paused = false; // new: timer paused state
    uint64_t deadlineMillis = 0; // millis() reference
    uint32_t remainingSeconds = 0; // persisted fallback
    uint8_t nextAlarmIndex = 0;
    bool finalTriggered = false;
};

class ConfigStore {
public:
    bool begin();

    TimerSettings loadTimerSettings() const;
    void saveTimerSettings(const TimerSettings &settings);

    MailSettings loadMailSettings() const;
    void saveMailSettings(const MailSettings &settings);

    WiFiSettings loadWiFiSettings() const;
    void saveWiFiSettings(const WiFiSettings &settings);

    TimerRuntime loadRuntime() const;
    void saveRuntime(const TimerRuntime &runtime);

    void eraseAll();

    bool ensureDataFolder();
    String dataFolder() const { return "/attachments"; }

private:
    static constexpr const char *TIMER_FILE = "/timer.json";
    static constexpr const char *MAIL_FILE = "/mail.json";
    static constexpr const char *WIFI_FILE = "/wifi.json";
    static constexpr const char *RUNTIME_FILE = "/runtime.json";

    void writeJson(const char *path, const JsonDocument &doc);
    bool readJson(const char *path, JsonDocument &doc) const;
};

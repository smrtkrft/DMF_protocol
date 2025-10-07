#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include "config_store.h"
#include "scheduler.h"
#include "network_manager.h"

class MailAgent {
public:
    void begin(ConfigStore *storePtr, DMFNetworkManager *netMgrPtr, const String &deviceIdStr);

    void updateConfig(const MailSettings &config);
    MailSettings currentConfig() const { return settings; }

    bool sendWarning(uint8_t alarmIndex, const ScheduleSnapshot &snapshot, String &errorMessage);
    bool sendFinal(const ScheduleSnapshot &snapshot, String &errorMessage);
    
    // Test fonksiyonları - sadece gönderen adrese mail atar
    bool sendWarningTest(const ScheduleSnapshot &snapshot, String &errorMessage);
    bool sendFinalTest(const ScheduleSnapshot &snapshot, String &errorMessage);

    // URL Validation - SSRF Koruması
    static bool isValidURL(const String &url);

private:
    ConfigStore *store = nullptr;
    DMFNetworkManager *netManager = nullptr;
    MailSettings settings;
    String deviceId;

    bool sendEmail(const String &subject, const String &body, bool includeWarningAttachments, String &errorMessage);
    bool sendEmailToSelf(const String &subject, const String &body, bool includeWarningAttachments, String &errorMessage); // Test için
    bool sendEmailToRecipient(const String &recipient, const String &subject, const String &body, bool includeWarningAttachments, String &errorMessage); // DMF protokolü için
    bool smtpConnect(WiFiClientSecure &client, String &errorMessage);
    bool smtpAuth(WiFiClientSecure &client, String &errorMessage);
    bool smtpSendMail(WiFiClientSecure &client, const String &subject, const String &body, bool includeAttachments, String &errorMessage);
    bool smtpCommand(WiFiClientSecure &client, const String &command, const String &expectCode, String &errorMessage);
    String smtpReadLine(WiFiClientSecure &client, uint32_t timeoutMs = 5000);
    String base64Encode(const String &input);
    String buildMimeMessage(const String &subject, const String &body, bool includeWarningAttachments);
    void appendAttachments(String &mime, const String &boundary, bool warning);
    String formatHeader() const;
    String formatElapsed(const ScheduleSnapshot &snapshot) const;
};

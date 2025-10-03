#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include "config_store.h"
#include "scheduler.h"
#include "network_manager.h"

class MailAgent {
public:
    void begin(ConfigStore *storePtr, DMFNetworkManager *netMgrPtr);

    void updateConfig(const MailSettings &config);
    MailSettings currentConfig() const { return settings; }

    bool sendWarning(uint8_t alarmIndex, const ScheduleSnapshot &snapshot, String &errorMessage);
    bool sendFinal(const ScheduleSnapshot &snapshot, String &errorMessage);

private:
    ConfigStore *store = nullptr;
    DMFNetworkManager *netManager = nullptr;
    MailSettings settings;

    bool sendEmail(const String &subject, const String &body, bool includeWarningAttachments, String &errorMessage);
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

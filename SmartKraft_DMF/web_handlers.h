#pragma once

#include <WebServer.h>
#include <DNSServer.h>
#include "scheduler.h"
#include "mail_functions.h"
#include "network_manager.h"
#include "config_store.h"

class WebInterface {
public:
    void begin(WebServer *server,
               ConfigStore *store,
               CountdownScheduler *scheduler,
               MailAgent *mail,
               DMFNetworkManager *netManager,
               const String &deviceId,
               DNSServer *dns = nullptr);

    void startServer();
    void loop();

    void broadcastStatus();

private:
    WebServer *server = nullptr;
    ConfigStore *store = nullptr;
    CountdownScheduler *scheduler = nullptr;
    MailAgent *mail = nullptr;
    DMFNetworkManager *network = nullptr;
    DNSServer *dnsServer = nullptr;
    String deviceId;

    unsigned long lastStatusPush = 0;

    void handleIndex();
    void handleStatus();
    void handleTimerGet();
    void handleTimerUpdate();
    void handleTimerStart();
    void handleTimerStop();
    void handleTimerResume();
    void handleTimerReset();
    void handleVirtualButton();

    void handleMailGet();
    void handleMailUpdate();
    void handleMailTest();

    void handleWiFiGet();
    void handleWiFiUpdate();
    void handleWiFiScan();

    void handleFactoryReset();
    void handleReboot();

    void handleAttachmentList();
    void handleAttachmentUpload();
    void handleAttachmentDelete();
    
    void handleI18n();

    void handleLogs();
    void sendJson(const JsonDocument &doc);
};

/*
  DMF - Delayed Message & Function System
  ESP32-S3 based countdown timer with relay control and secure messaging
  
  Features:
  - Multi-level WiFi management (secure/open networks)
  - 3-tier relay control system
  - ProtonMail integration with 3 mail groups
  - HTTP GET URL triggering (3 URLs per group)
  - Web interface (dmf-smartkraft.local)
  - Hardware encryption (ESP32-S3 security features)
  - Early warning system (5 alarms)
  - Persistent storage with factory reset
  
  Author: DMF Project
  Version: 1.0
  Date: 2025
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <esp_random.h>
#include <mbedtls/aes.h>
#include <esp_flash_encrypt.h>
#include <esp_secure_boot.h>

// ==================== FUNCTION DECLARATIONS ====================
void completeSetup();
void setupAccessPoint();
bool isFirstBoot();
void initializeSecurity();
void loadConfiguration();
void loadContentData();
void saveConfiguration();
void saveContentData();
void factoryReset();
void initializeWiFi();
void setupWebServer();

// ==================== CONFIGURATION ====================
#define DEVICE_NAME "DMF-SmartKraft"
#define FIRMWARE_VERSION "1.0.0"
#define HOSTNAME "dmf-smartkraft"

// Pin Definitions (ESP32-S3)
#define RELAY_PIN 2          // Relay control pin
#define BUTTON_PIN 0         // Boot button for manual trigger
#define STATUS_LED_PIN 8     // Status LED
#define SETUP_LED_PIN 9      // Setup mode indicator

// Security Settings
#define AES_KEY_SIZE 32
#define DEVICE_ID_SIZE 16
#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64

// System Limits
#define MAX_MAIL_GROUPS 3
#define MAX_URLS_PER_GROUP 3
#define MAX_EMAILS_PER_GROUP 10
#define MAX_EARLY_WARNINGS 5
#define MAX_OPEN_NETWORKS 20

// Timing Constants
#define BUTTON_DEBOUNCE_MS 50
#define WIFI_CONNECT_TIMEOUT 10000
#define INTERNET_TEST_TIMEOUT 5000
#define RELAY_PULSE_DURATION 1000
#define STATUS_UPDATE_INTERVAL 1000

// ==================== DATA STRUCTURES ====================

// Relay Priority Levels
enum RelayPriority {
  RELAY_INTERNET_DEPENDENT = 1,  // Wait for internet and message delivery
  RELAY_MIXED_PRIORITY = 2,      // Trigger relay, send message when internet available
  RELAY_ONLY = 3                 // Trigger relay regardless of internet
};

// WiFi Security Types
enum WiFiSecurityType {
  WIFI_SECURE,
  WIFI_OPEN,
  WIFI_UNKNOWN
};

// System States
enum SystemState {
  STATE_SETUP,
  STATE_CONNECTING,
  STATE_READY,
  STATE_COUNTDOWN,
  STATE_TRIGGERED,
  STATE_ERROR
};

// Early Warning Structure
struct EarlyWarning {
  bool enabled;
  uint32_t minutes_before;
  String message;
  bool triggered;
};

// Mail Group Structure
struct MailGroup {
  String name;
  String emails[MAX_EMAILS_PER_GROUP];
  int email_count;
  String message_subject;
  String message_body;
  String urls[MAX_URLS_PER_GROUP];
  int url_count;
  bool enabled;
};

// WiFi Configuration
struct WiFiConfig {
  String primary_ssid;
  String primary_password;
  String backup_ssids[5];
  String backup_passwords[5];
  int backup_count;
  bool allow_open_networks;
  bool security_mode;
};

// Countdown Configuration
struct CountdownConfig {
  uint32_t duration_minutes;
  bool active;
  uint32_t start_time;
  uint32_t pause_time;
  bool paused;
  RelayPriority relay_priority;
  bool postpone_enabled;
  uint32_t postpone_duration_minutes;
};

// ==================== GLOBAL VARIABLES ====================

// Core System
WebServer server(80);
Preferences preferences;
SystemState current_state = STATE_SETUP;
bool factory_reset_pending = false;
bool device_initialized = false;

// Security
uint8_t device_aes_key[AES_KEY_SIZE];
uint8_t device_id[DEVICE_ID_SIZE];
String device_certificate;
mbedtls_aes_context aes_ctx;

// Configuration Storage
WiFiConfig wifi_config;
CountdownConfig countdown_config;
MailGroup mail_groups[MAX_MAIL_GROUPS];
EarlyWarning early_warnings[MAX_EARLY_WARNINGS];

// Runtime Variables
unsigned long last_button_press = 0;
unsigned long last_status_update = 0;
unsigned long countdown_start_millis = 0;
bool relay_triggered = false;
bool internet_available = false;
WiFiClientSecure secure_client;
String current_network_ssid = "";

// ProtonMail Configuration
struct ProtonMailConfig {
  String api_endpoint = "https://api.protonmail.ch/api";
  String api_key = "";
  String sender_email = "";
  bool configured = false;
} protonmail_config;

// ==================== SECURITY FUNCTIONS ====================

void initializeSecurity() {
  // Initialize hardware random number generator
  esp_random();
  
  // Generate or load device ID
  if (!preferences.getBytesLength("device_id")) {
    esp_fill_random(device_id, DEVICE_ID_SIZE);
    preferences.putBytes("device_id", device_id, DEVICE_ID_SIZE);
    Serial.println("Generated new device ID");
  } else {
    preferences.getBytes("device_id", device_id, DEVICE_ID_SIZE);
    Serial.println("Loaded existing device ID");
  }
  
  // Generate or load AES key
  if (!preferences.getBytesLength("aes_key")) {
    esp_fill_random(device_aes_key, AES_KEY_SIZE);
    preferences.putBytes("aes_key", device_aes_key, AES_KEY_SIZE);
    Serial.println("Generated new AES key");
  } else {
    preferences.getBytes("aes_key", device_aes_key, AES_KEY_SIZE);
    Serial.println("Loaded existing AES key");
  }
  
  // Initialize AES context
  mbedtls_aes_init(&aes_ctx);
  mbedtls_aes_setkey_enc(&aes_ctx, device_aes_key, 256);
  
  Serial.println("Security subsystem initialized");
}

String encryptString(const String& plaintext) {
  if (plaintext.length() == 0) return "";
  
  size_t input_len = plaintext.length();
  size_t output_len = ((input_len / 16) + 1) * 16; // AES block size padding
  
  uint8_t* input = (uint8_t*)malloc(output_len);
  uint8_t* output = (uint8_t*)malloc(output_len);
  
  memset(input, 0, output_len);
  memcpy(input, plaintext.c_str(), input_len);
  
  // Simple padding
  for (size_t i = input_len; i < output_len; i++) {
    input[i] = output_len - input_len;
  }
  
  // Encrypt in blocks
  for (size_t i = 0; i < output_len; i += 16) {
    mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, input + i, output + i);
  }
  
  // Convert to base64-like string
  String result = "";
  for (size_t i = 0; i < output_len; i++) {
    if (output[i] < 16) result += "0";
    result += String(output[i], HEX);
  }
  
  free(input);
  free(output);
  return result;
}

String decryptString(const String& ciphertext) {
  if (ciphertext.length() == 0 || ciphertext.length() % 32 != 0) return "";
  
  size_t output_len = ciphertext.length() / 2;
  uint8_t* input = (uint8_t*)malloc(output_len);
  uint8_t* output = (uint8_t*)malloc(output_len);
  
  // Convert hex string to bytes
  for (size_t i = 0; i < output_len; i++) {
    String byte_str = ciphertext.substring(i * 2, i * 2 + 2);
    input[i] = strtol(byte_str.c_str(), NULL, 16);
  }
  
  // Decrypt in blocks
  mbedtls_aes_context decrypt_ctx;
  mbedtls_aes_init(&decrypt_ctx);
  mbedtls_aes_setkey_dec(&decrypt_ctx, device_aes_key, 256);
  
  for (size_t i = 0; i < output_len; i += 16) {
    mbedtls_aes_crypt_ecb(&decrypt_ctx, MBEDTLS_AES_DECRYPT, input + i, output + i);
  }
  
  // Remove padding
  uint8_t padding = output[output_len - 1];
  if (padding <= 16 && padding > 0) {
    output_len -= padding;
  }
  
  String result = String((char*)output).substring(0, output_len);
  
  mbedtls_aes_free(&decrypt_ctx);
  free(input);
  free(output);
  return result;
}

// ==================== WIFI MANAGEMENT ====================

bool testInternetConnection() {
  HTTPClient http;
  http.begin("http://httpbin.org/ip");
  http.setTimeout(INTERNET_TEST_TIMEOUT);
  
  int httpCode = http.GET();
  http.end();
  
  return (httpCode == 200);
}

WiFiSecurityType getNetworkSecurity(const String& ssid) {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == ssid) {
      wifi_auth_mode_t authmode = (wifi_auth_mode_t)WiFi.encryptionType(i);
      return (authmode == WIFI_AUTH_OPEN) ? WIFI_OPEN : WIFI_SECURE;
    }
  }
  return WIFI_UNKNOWN;
}

bool connectToWiFi(const String& ssid, const String& password = "") {
  Serial.printf("Attempting to connect to: %s\n", ssid.c_str());
  
  WiFi.disconnect();
  delay(500);
  WiFi.mode(WIFI_STA);
  delay(100);
  
  if (password.length() > 0) {
    WiFi.begin(ssid.c_str(), password.c_str());
  } else {
    WiFi.begin(ssid.c_str());
  }
  
  unsigned long start_time = millis();
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED && millis() - start_time < WIFI_CONNECT_TIMEOUT) {
    delay(500);
    Serial.print(".");
    dots++;
    if (dots > 20) {
      Serial.print("\nStill trying");
      dots = 0;
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nConnected to %s\n", ssid.c_str());
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    current_network_ssid = ssid;
    
    // Test internet connectivity
    internet_available = testInternetConnection();
    Serial.printf("Internet available: %s\n", internet_available ? "Yes" : "No");
    
    return true;
  }
  
  Serial.println("\nFailed to connect");
  return false;
}

void scanAndConnectOpenNetworks() {
  if (!wifi_config.allow_open_networks) {
    Serial.println("Open networks disabled in configuration");
    return;
  }
  
  Serial.println("Scanning for open networks...");
  int n = WiFi.scanNetworks();
  
  for (int i = 0; i < n; i++) {
    wifi_auth_mode_t authmode = (wifi_auth_mode_t)WiFi.encryptionType(i);
    if (authmode == WIFI_AUTH_OPEN) {
      String ssid = WiFi.SSID(i);
      Serial.printf("Found open network: %s\n", ssid.c_str());
      
      if (connectToWiFi(ssid)) {
        if (testInternetConnection()) {
          Serial.printf("Successfully connected to open network: %s\n", ssid.c_str());
          return;
        } else {
          Serial.printf("No internet access on: %s\n", ssid.c_str());
          WiFi.disconnect();
        }
      }
    }
  }
  
  Serial.println("No suitable open networks found");
}

void initializeWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  
  // Try primary network first
  if (wifi_config.primary_ssid.length() > 0) {
    if (connectToWiFi(wifi_config.primary_ssid, wifi_config.primary_password)) {
      return;
    }
  }
  
  // Try backup networks
  for (int i = 0; i < wifi_config.backup_count; i++) {
    if (connectToWiFi(wifi_config.backup_ssids[i], wifi_config.backup_passwords[i])) {
      return;
    }
  }
  
  // Try open networks as last resort
  scanAndConnectOpenNetworks();
}

// ==================== STORAGE MANAGEMENT ====================

void loadConfiguration() {
  preferences.begin("dmf-config", false);
  
  // Load WiFi configuration
  wifi_config.primary_ssid = preferences.getString("wifi_ssid", "");
  wifi_config.primary_password = decryptString(preferences.getString("wifi_pass", ""));
  wifi_config.allow_open_networks = preferences.getBool("allow_open", true);
  wifi_config.security_mode = preferences.getBool("security_mode", true);
  wifi_config.backup_count = preferences.getInt("backup_count", 0);
  
  for (int i = 0; i < wifi_config.backup_count && i < 5; i++) {
    wifi_config.backup_ssids[i] = preferences.getString(("backup_ssid_" + String(i)).c_str(), "");
    wifi_config.backup_passwords[i] = decryptString(preferences.getString(("backup_pass_" + String(i)).c_str(), ""));
  }
  
  // Load countdown configuration
  countdown_config.duration_minutes = preferences.getULong("countdown_dur", 60);
  countdown_config.active = preferences.getBool("countdown_active", false);
  countdown_config.relay_priority = (RelayPriority)preferences.getInt("relay_priority", RELAY_MIXED_PRIORITY);
  countdown_config.postpone_enabled = preferences.getBool("postpone_enabled", true);
  countdown_config.postpone_duration_minutes = preferences.getULong("postpone_dur", 15);
  
  // Load ProtonMail configuration
  protonmail_config.api_key = decryptString(preferences.getString("pm_api_key", ""));
  protonmail_config.sender_email = preferences.getString("pm_sender", "");
  protonmail_config.configured = (protonmail_config.api_key.length() > 0);
  
  Serial.println("Configuration loaded from preferences");
}

void saveConfiguration() {
  preferences.begin("dmf-config", false);
  
  // Save WiFi configuration
  preferences.putString("wifi_ssid", wifi_config.primary_ssid);
  preferences.putString("wifi_pass", encryptString(wifi_config.primary_password));
  preferences.putBool("allow_open", wifi_config.allow_open_networks);
  preferences.putBool("security_mode", wifi_config.security_mode);
  preferences.putInt("backup_count", wifi_config.backup_count);
  
  for (int i = 0; i < wifi_config.backup_count; i++) {
    preferences.putString(("backup_ssid_" + String(i)).c_str(), wifi_config.backup_ssids[i]);
    preferences.putString(("backup_pass_" + String(i)).c_str(), encryptString(wifi_config.backup_passwords[i]));
  }
  
  // Save countdown configuration
  preferences.putULong("countdown_dur", countdown_config.duration_minutes);
  preferences.putBool("countdown_active", countdown_config.active);
  preferences.putInt("relay_priority", countdown_config.relay_priority);
  preferences.putBool("postpone_enabled", countdown_config.postpone_enabled);
  preferences.putULong("postpone_dur", countdown_config.postpone_duration_minutes);
  
  // Save ProtonMail configuration
  preferences.putString("pm_api_key", encryptString(protonmail_config.api_key));
  preferences.putString("pm_sender", protonmail_config.sender_email);
  
  preferences.end();
  Serial.println("Configuration saved to preferences");
}

void loadContentData() {
  preferences.begin("dmf-content", false);
  
  // Load mail groups
  for (int g = 0; g < MAX_MAIL_GROUPS; g++) {
    String prefix = "group_" + String(g) + "_";
    mail_groups[g].name = preferences.getString((prefix + "name").c_str(), "Group " + String(g + 1));
    mail_groups[g].enabled = preferences.getBool((prefix + "enabled").c_str(), false);
    mail_groups[g].message_subject = preferences.getString((prefix + "subject").c_str(), "DMF Alert");
    mail_groups[g].message_body = preferences.getString((prefix + "body").c_str(), "Timer expired");
    
    mail_groups[g].email_count = preferences.getInt((prefix + "email_count").c_str(), 0);
    for (int e = 0; e < mail_groups[g].email_count && e < MAX_EMAILS_PER_GROUP; e++) {
      mail_groups[g].emails[e] = preferences.getString((prefix + "email_" + String(e)).c_str(), "");
    }
    
    mail_groups[g].url_count = preferences.getInt((prefix + "url_count").c_str(), 0);
    for (int u = 0; u < mail_groups[g].url_count && u < MAX_URLS_PER_GROUP; u++) {
      mail_groups[g].urls[u] = preferences.getString((prefix + "url_" + String(u)).c_str(), "");
    }
  }
  
  // Load early warnings
  for (int w = 0; w < MAX_EARLY_WARNINGS; w++) {
    String prefix = "warning_" + String(w) + "_";
    early_warnings[w].enabled = preferences.getBool((prefix + "enabled").c_str(), false);
    early_warnings[w].minutes_before = preferences.getULong((prefix + "minutes").c_str(), 60);
    early_warnings[w].message = preferences.getString((prefix + "message").c_str(), "Warning: Timer expires soon");
    early_warnings[w].triggered = false;
  }
  
  preferences.end();
  Serial.println("Content data loaded from preferences");
}

void saveContentData() {
  preferences.begin("dmf-content", false);
  
  // Save mail groups
  for (int g = 0; g < MAX_MAIL_GROUPS; g++) {
    String prefix = "group_" + String(g) + "_";
    preferences.putString((prefix + "name").c_str(), mail_groups[g].name);
    preferences.putBool((prefix + "enabled").c_str(), mail_groups[g].enabled);
    preferences.putString((prefix + "subject").c_str(), mail_groups[g].message_subject);
    preferences.putString((prefix + "body").c_str(), mail_groups[g].message_body);
    
    preferences.putInt((prefix + "email_count").c_str(), mail_groups[g].email_count);
    for (int e = 0; e < mail_groups[g].email_count; e++) {
      preferences.putString((prefix + "email_" + String(e)).c_str(), mail_groups[g].emails[e]);
    }
    
    preferences.putInt((prefix + "url_count").c_str(), mail_groups[g].url_count);
    for (int u = 0; u < mail_groups[g].url_count; u++) {
      preferences.putString((prefix + "url_" + String(u)).c_str(), mail_groups[g].urls[u]);
    }
  }
  
  // Save early warnings
  for (int w = 0; w < MAX_EARLY_WARNINGS; w++) {
    String prefix = "warning_" + String(w) + "_";
    preferences.putBool((prefix + "enabled").c_str(), early_warnings[w].enabled);
    preferences.putULong((prefix + "minutes").c_str(), early_warnings[w].minutes_before);
    preferences.putString((prefix + "message").c_str(), early_warnings[w].message);
  }
  
  preferences.end();
  Serial.println("Content data saved to preferences");
}

void factoryReset() {
  Serial.println("Performing factory reset...");
  
  // Clear content data only (preserve device security keys)
  preferences.begin("dmf-content", false);
  preferences.clear();
  preferences.end();
  
  // Reset mail groups
  for (int g = 0; g < MAX_MAIL_GROUPS; g++) {
    mail_groups[g].name = "Group " + String(g + 1);
    mail_groups[g].enabled = false;
    mail_groups[g].email_count = 0;
    mail_groups[g].url_count = 0;
    mail_groups[g].message_subject = "DMF Alert";
    mail_groups[g].message_body = "Timer expired";
  }
  
  // Reset early warnings
  for (int w = 0; w < MAX_EARLY_WARNINGS; w++) {
    early_warnings[w].enabled = false;
    early_warnings[w].minutes_before = 60;
    early_warnings[w].message = "Warning: Timer expires soon";
    early_warnings[w].triggered = false;
  }
  
  saveContentData();
  Serial.println("Factory reset completed");
}

// ==================== WEB SERVER SETUP ====================

String getStatusJSON() {
  DynamicJsonDocument doc(1024);
  
  doc["state"] = current_state;
  doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
  doc["wifi_ssid"] = current_network_ssid;
  doc["internet_available"] = internet_available;
  doc["countdown_active"] = countdown_config.active;
  doc["relay_triggered"] = relay_triggered;
  doc["firmware_version"] = FIRMWARE_VERSION;
  
  if (countdown_config.active) {
    unsigned long elapsed = (millis() - countdown_start_millis) / 1000 / 60; // minutes
    unsigned long remaining = (countdown_config.duration_minutes > elapsed) ? 
                             countdown_config.duration_minutes - elapsed : 0;
    doc["countdown_remaining"] = remaining;
    doc["countdown_total"] = countdown_config.duration_minutes;
  }
  
  String output;
  serializeJson(doc, output);
  return output;
}

void setupWebServer() {
  // CORS headers for all responses
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  
  // HTML content stored in PROGMEM to save RAM
  const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>DMF SmartKraft</title>
<style>
body { background: #fff; color: #222; font-family: 'Segoe UI', Arial, sans-serif; }
.container { max-width: 600px; margin: 40px auto; padding: 0 10px; }
.header { text-align: center; margin-bottom: 24px; }
.header h1 { font-size: 1.5rem; font-weight: 600; margin-bottom: 4px; }
.header .subtitle { font-size: 0.95rem; color: #888; }
.tabs { display: flex; justify-content: center; gap: 0; margin-bottom: 18px; }
.tab { flex: 1; padding: 10px 0; background: #f7f7f7; border: 1px solid #e0e0e0; border-bottom: none; color: #444; font-weight: 500; cursor: pointer; outline: none; transition: background 0.2s; border-radius: 8px 8px 0 0; margin: 0 2px; }
.tab.active { background: #fff; color: #111; border-bottom: 1px solid #fff; }
.tab-content { display: none; background: #fff; border: 1px solid #e0e0e0; border-radius: 0 0 8px 8px; padding: 24px 18px; margin-bottom: 24px; }
.tab-content.active { display: block; }
.form-group { margin-bottom: 16px; }
.form-group label { display: block; margin-bottom: 6px; font-size: 0.97rem; color: #444; }
.form-group input, .form-group select, .form-group textarea { width: 100%; padding: 8px; border: 1px solid #d0d0d0; border-radius: 4px; font-size: 1rem; background: #fafafa; color: #222; }
.form-group input:focus, .form-group select:focus, .form-group textarea:focus { border-color: #bbb; outline: none; }
.btn { padding: 8px 18px; border: 1px solid #bbb; border-radius: 4px; background: #f7f7f7; color: #222; font-weight: 500; cursor: pointer; margin-right: 8px; margin-bottom: 8px; transition: background 0.2s, border 0.2s; }
.btn:hover { background: #ededed; border-color: #888; }
.status-card { display: flex; justify-content: space-between; gap: 8px; margin-bottom: 18px; }
.status-item { flex: 1; background: #fafafa; border: 1px solid #e0e0e0; border-radius: 4px; text-align: center; padding: 10px 0; font-size: 0.98rem; }
.status-item .value { font-size: 1.1rem; font-weight: 600; color: #444; }
.status-item .label { font-size: 0.8rem; color: #888; margin-top: 2px; }
.section { margin-bottom: 22px; }
.section-title { font-size: 1.08rem; font-weight: 600; margin-bottom: 10px; color: #333; border-bottom: 1px solid #eee; padding-bottom: 4px; }
.mail-group { border: 1px solid #e0e0e0; border-radius: 4px; padding: 12px; margin-bottom: 10px; background: #fafafa; }
.mail-group h4 { color: #444; margin-bottom: 10px; font-size: 1rem; }
.url-list, .email-list { margin-top: 6px; }
.url-item, .email-item { display: flex; gap: 6px; margin-bottom: 4px; }
.url-item input, .email-item input { flex: 1; }
.countdown-display { text-align: center; padding: 18px 0; background: #f7f7f7; color: #222; border-radius: 4px; margin: 14px 0; font-size: 1.2rem; }
.countdown-display .time { font-size: 2.1rem; font-weight: 600; margin-bottom: 4px; }
.countdown-display .status { font-size: 1rem; color: #888; }
#setup-alert { display: none; background: #fffbe6; border: 1px solid #ffe58f; color: #8d7600; padding: 10px; border-radius: 4px; margin-bottom: 16px; font-size: 0.97rem; text-align: center; }
@media (max-width: 700px) { .container { max-width: 98vw; } }
</style>
</head>
<body>
<div class='container'>
<div class='header'>
<h1>DMF SmartKraft</h1>
<div class='subtitle'>Delayed Message & Function Control System</div>
</div>
<div id='setup-alert'><b>🛠️ SETUP MODE:</b> Please configure WiFi and click Save Settings.</div>
<div class='tabs'>
<button class='tab active' onclick="showTab('content')">Content</button>
<button class='tab' onclick="showTab('settings')">Settings</button>
</div>
<div id='content' class='tab-content active'>
<div class='section'>
<div class='section-title'>Mail Groups</div>
<div id='mail-groups'></div>
</div>
</div>
<div id='settings' class='tab-content'>
<div class='status-card'>
<div class='status-item'><div class='value' id='wifi-status'>-</div><div class='label'>WiFi</div></div>
<div class='status-item'><div class='value' id='internet-status'>-</div><div class='label'>Internet</div></div>
<div class='status-item'><div class='value' id='countdown-status'>-</div><div class='label'>Countdown</div></div>
<div class='status-item'><div class='value' id='relay-status'>-</div><div class='label'>Relay</div></div>
</div>
<div id='countdown-display' class='countdown-display' style='display:none;'>
<div class='time' id='countdown-time'>00:00:00</div>
<div class='status' id='countdown-text'>Timer Active</div>
</div>
<div class='section'>
<div class='section-title'>WiFi</div>
<div class='form-group'><label>Primary WiFi SSID:</label><input type='text' id='wifi-ssid'></div>
<div class='form-group'><label>Primary WiFi Password:</label><input type='password' id='wifi-password'></div>
<div class='form-group'><label><input type='checkbox' id='allow-open-networks'> Allow open networks as backup</label></div>
<div class='form-group'><label><input type='checkbox' id='security-mode'> Enable security mode (HTTPS only)</label></div>
</div>
<div class='section'>
<div class='section-title'>Countdown</div>
<div class='form-group'>
<label>Countdown Duration:</label>
<div style='display:flex; gap:8px;'>
<input type='number' id='countdown-days' min='0' max='365' value='0' style='width:70px;'> <span style='align-self:center;'>days</span>
<input type='number' id='countdown-hours' min='0' max='23' value='0' style='width:70px;'> <span style='align-self:center;'>hours</span>
<input type='number' id='countdown-minutes' min='0' max='59' value='0' style='width:70px;'> <span style='align-self:center;'>min</span>
</div>
</div>
<div class='form-group'>
<label>Relay Priority:</label>
<select id='relay-priority'>
<option value='1'>Internet Dependent</option>
<option value='2' selected>Mixed Priority</option>
<option value='3'>Relay Only</option>
</select>
</div>
<div class='form-group'><label><input type='checkbox' id='postpone-enabled'> Enable postpone</label></div>
<div class='form-group'><label>Postpone Duration (minutes):</label><input type='number' id='postpone-duration' min='1' max='1440' value='15'></div>
</div>
<div class='section'>
<div class='section-title'>Early Warnings</div>
<div id='early-warnings'></div>
</div>
<div class='section'>
<div class='section-title'>ProtonMail</div>
<div class='form-group'><label>API Key:</label><input type='password' id='protonmail-api-key'></div>
<div class='form-group'><label>Sender Email:</label><input type='email' id='protonmail-sender'></div>
</div>
<div class='section'>
<div class='section-title'>System Controls</div>
<button class='btn' onclick='startCountdown()'>Start</button>
<button class='btn' onclick='pauseCountdown()'>Pause/Resume</button>
<button class='btn' onclick='stopCountdown()'>Stop</button>
<button class='btn' onclick='postponeCountdown()'>Postpone</button>
<br><br>
<button class='btn' onclick='saveSettings()'>Save Settings</button>
<button class='btn' onclick='restartDevice()'>Restart</button>
<button class='btn' onclick='factoryReset()' style='margin-top:8px;'>Factory Reset</button>
</div>
</div>
</div>
<script>
let updateInterval;
function showTab(tabName) {
document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
event.target.classList.add('active');
document.getElementById(tabName).classList.add('active');
if (tabName === 'settings') { startStatusUpdates(); loadSettings(); } else { stopStatusUpdates(); loadContent(); }
}
function startStatusUpdates() { updateStatus(); updateInterval = setInterval(updateStatus, 2000); }
function stopStatusUpdates() { if (updateInterval) clearInterval(updateInterval); }
async function updateStatus() {
try {
const response = await fetch('/api/status');
const data = await response.json();
document.getElementById('wifi-status').textContent = data.wifi_connected ? data.wifi_ssid : '-';
document.getElementById('internet-status').textContent = data.internet_available ? 'Available' : '-';
document.getElementById('countdown-status').textContent = data.countdown_active ? 'Active' : '-';
document.getElementById('relay-status').textContent = data.relay_triggered ? 'Triggered' : '-';
document.getElementById('setup-alert').style.display = (data.state === 0) ? 'block' : 'none';
const cd = document.getElementById('countdown-display');
if (data.countdown_active && data.countdown_remaining !== undefined) {
cd.style.display = 'block';
let total = data.countdown_remaining;
let days = Math.floor(total / 1440);
let hours = Math.floor((total % 1440) / 60);
let mins = total % 60;
document.getElementById('countdown-time').textContent = days + 'd ' + hours + 'h ' + mins + 'm';
} else { cd.style.display = 'none'; }
} catch (e) { /* ignore */ }
}
async function loadSettings() {
try {
const response = await fetch('/api/config');
const data = await response.json();
document.getElementById('wifi-ssid').value = data.wifi_ssid || '';
document.getElementById('wifi-password').value = '';
document.getElementById('allow-open-networks').checked = data.allow_open_networks;
document.getElementById('security-mode').checked = data.security_mode;
let dur = data.countdown_duration || 0;
document.getElementById('countdown-days').value = Math.floor(dur / 1440);
document.getElementById('countdown-hours').value = Math.floor((dur % 1440) / 60);
document.getElementById('countdown-minutes').value = dur % 60;
document.getElementById('relay-priority').value = data.relay_priority;
document.getElementById('postpone-enabled').checked = data.postpone_enabled;
document.getElementById('postpone-duration').value = data.postpone_duration;
document.getElementById('protonmail-api-key').value = '';
document.getElementById('protonmail-sender').value = data.protonmail_sender || '';
loadEarlyWarnings(data.early_warnings);
} catch (e) { /* ignore */ }
}
function loadEarlyWarnings(warnings) {
const c = document.getElementById('early-warnings');
c.innerHTML = '';
for (let i = 0; i < 5; i++) {
const w = warnings[i] || { enabled: false, minutes_before: 60, message: 'Warning: Timer expires soon' };
c.innerHTML += '<div class=\"form-group\"><label><input type=\"checkbox\" id=\"warning-' + i + '-enabled\" ' + (w.enabled ? 'checked' : '') + '> Warning ' + (i + 1) + '</label><div style=\"display:flex;gap:8px;margin-top:6px;\"><input type=\"number\" id=\"warning-' + i + '-minutes\" value=\"' + w.minutes_before + '\" style=\"width:80px;\"><input type=\"text\" id=\"warning-' + i + '-message\" value=\"' + w.message + '\" style=\"flex:1;\"></div></div>';
}
}
async function loadContent() {
try {
const response = await fetch('/api/content');
const data = await response.json();
loadMailGroups(data.mail_groups);
} catch (e) { /* ignore */ }
}
function loadMailGroups(groups) {
const c = document.getElementById('mail-groups');
c.innerHTML = '';
for (let i = 0; i < 3; i++) {
const g = groups[i] || { name: 'Group ' + (i + 1), enabled: false, emails: [], message_subject: 'DMF Alert', message_body: 'Timer expired', urls: [] };
let emailsHtml = '';
for (let e = 0; e < 10; e++) {
const email = g.emails[e] || '';
emailsHtml += '<div class=\"email-item\"><input type=\"email\" id=\"group-' + i + '-email-' + e + '\" value=\"' + email + '\" placeholder=\"email@example.com\"></div>';
}
let urlsHtml = '';
for (let u = 0; u < 3; u++) {
const url = g.urls[u] || '';
urlsHtml += '<div class=\"url-item\"><input type=\"url\" id=\"group-' + i + '-url-' + u + '\" value=\"' + url + '\" placeholder=\"https://example.com/webhook\"></div>';
}
c.innerHTML += '<div class=\"mail-group\"><h4><input type=\"checkbox\" id=\"group-' + i + '-enabled\" ' + (g.enabled ? 'checked' : '') + '> ' + g.name + '</h4><div class=\"form-group\"><label>Group Name:</label><input type=\"text\" id=\"group-' + i + '-name\" value=\"' + g.name + '\"></div><div class=\"form-group\"><label>Message Subject:</label><input type=\"text\" id=\"group-' + i + '-subject\" value=\"' + g.message_subject + '\"></div><div class=\"form-group\"><label>Message Body:</label><textarea id=\"group-' + i + '-body\" rows=\"2\">' + g.message_body + '</textarea></div><div class=\"form-group\"><label>Email Addresses:</label><div class=\"email-list\">' + emailsHtml + '</div></div><div class=\"form-group\"><label>Webhook URLs:</label><div class=\"url-list\">' + urlsHtml + '</div></div></div>';
}
c.innerHTML += '<button class=\"btn\" onclick=\"saveContent()\">Save Content</button>';
}
c.innerHTML += "<button class='btn' onclick='saveContent()'>Save Content</button>";
}
async function saveSettings() {
let days = parseInt(document.getElementById('countdown-days').value) || 0;
let hours = parseInt(document.getElementById('countdown-hours').value) || 0;
let mins = parseInt(document.getElementById('countdown-minutes').value) || 0;
let totalMinutes = days * 1440 + hours * 60 + mins;
const settings = {
wifi_ssid: document.getElementById('wifi-ssid').value,
wifi_password: document.getElementById('wifi-password').value,
allow_open_networks: document.getElementById('allow-open-networks').checked,
security_mode: document.getElementById('security-mode').checked,
countdown_duration: totalMinutes,
relay_priority: parseInt(document.getElementById('relay-priority').value),
postpone_enabled: document.getElementById('postpone-enabled').checked,
postpone_duration: parseInt(document.getElementById('postpone-duration').value),
protonmail_api_key: document.getElementById('protonmail-api-key').value,
protonmail_sender: document.getElementById('protonmail-sender').value,
early_warnings: []
};
for (let i = 0; i < 5; i++) {
settings.early_warnings.push({
enabled: document.getElementById('warning-' + i + '-enabled').checked,
minutes_before: parseInt(document.getElementById('warning-' + i + '-minutes').value),
message: document.getElementById('warning-' + i + '-message').value
});
}
try {
const response = await fetch('/api/config', {
method: 'POST',
headers: { 'Content-Type': 'application/json' },
body: JSON.stringify(settings)
});
if (response.ok) {
const result = await response.json();
if (result.status === 'setup_complete') {
alert('Setup completed! Device will restart and connect to WiFi. Please reconnect to dmf-smartkraft.local after restart.');
clearInterval(updateInterval);
} else {
alert('Settings saved successfully!');
}
} else {
alert('Failed to save settings');
}
} catch (error) {
alert('Error saving settings: ' + error.message);
}
}
async function saveContent() {
const content = { mail_groups: [] };
for (let i = 0; i < 3; i++) {
const group = {
name: document.getElementById('group-' + i + '-name').value,
enabled: document.getElementById('group-' + i + '-enabled').checked,
message_subject: document.getElementById('group-' + i + '-subject').value,
message_body: document.getElementById('group-' + i + '-body').value,
emails: [],
urls: []
};
for (let e = 0; e < 10; e++) {
const email = document.getElementById('group-' + i + '-email-' + e).value.trim();
if (email) group.emails.push(email);
}
for (let u = 0; u < 3; u++) {
const url = document.getElementById('group-' + i + '-url-' + u).value.trim();
if (url) group.urls.push(url);
}
content.mail_groups.push(group);
}
try {
const response = await fetch('/api/content', {
method: 'POST',
headers: { 'Content-Type': 'application/json' },
body: JSON.stringify(content)
});
if (response.ok) {
alert('Content saved successfully!');
} else {
alert('Failed to save content');
}
} catch (error) {
alert('Error saving content: ' + error.message);
}
}
async function startCountdown() { try { const response = await fetch('/api/control/start', { method: 'POST' }); if (response.ok) { alert('Countdown started!'); updateStatus(); } else { alert('Failed to start countdown'); } } catch (error) { alert('Error: ' + error.message); } }
async function pauseCountdown() { try { const response = await fetch('/api/control/pause', { method: 'POST' }); if (response.ok) { alert('Countdown paused/resumed!'); updateStatus(); } else { alert('Failed to pause countdown'); } } catch (error) { alert('Error: ' + error.message); } }
async function stopCountdown() { try { const response = await fetch('/api/control/stop', { method: 'POST' }); if (response.ok) { alert('Countdown stopped!'); updateStatus(); } else { alert('Failed to stop countdown'); } } catch (error) { alert('Error: ' + error.message); } }
async function postponeCountdown() { try { const response = await fetch('/api/control/postpone', { method: 'POST' }); if (response.ok) { alert('Countdown postponed!'); updateStatus(); } else { alert('Failed to postpone countdown'); } } catch (error) { alert('Error: ' + error.message); } }
async function restartDevice() { if (confirm('Are you sure you want to restart the device?')) { try { await fetch('/api/control/restart', { method: 'POST' }); alert('Device is restarting...'); } catch (error) {} } }
async function factoryReset() { if (confirm('This will erase all content data. Are you sure?')) { try { const response = await fetch('/api/control/factory-reset', { method: 'POST' }); if (response.ok) { alert('Factory reset completed!'); loadContent(); } else { alert('Failed to perform factory reset'); } } catch (error) { alert('Error: ' + error.message); } } }
startStatusUpdates();
loadContent();
</script>
</body>
</html>
)rawliteral";
  
  // Serve main page
  server.on("/", HTTP_GET, [&html]() {
    server.sendHeader("Content-Type", "text/html; charset=UTF-8");
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.send_P(200, "text/html; charset=UTF-8", html);
  });
  
  // API Status endpoint
  server.on("/api/status", HTTP_GET, []() {
    server.sendHeader("Content-Type", "application/json; charset=UTF-8");
    server.send(200, "application/json; charset=UTF-8", getStatusJSON());
  });

  // API Configuration endpoints
  server.on("/api/config", HTTP_GET, []() {
    DynamicJsonDocument doc(2048);
    
    doc["wifi_ssid"] = wifi_config.primary_ssid;
    doc["allow_open_networks"] = wifi_config.allow_open_networks;
    doc["security_mode"] = wifi_config.security_mode;
    doc["countdown_duration"] = countdown_config.duration_minutes;
    doc["relay_priority"] = countdown_config.relay_priority;
    doc["postpone_enabled"] = countdown_config.postpone_enabled;
    doc["postpone_duration"] = countdown_config.postpone_duration_minutes;
    doc["protonmail_sender"] = protonmail_config.sender_email;
    
    JsonArray warnings = doc.createNestedArray("early_warnings");
    for (int i = 0; i < MAX_EARLY_WARNINGS; i++) {
      JsonObject warning = warnings.createNestedObject();
      warning["enabled"] = early_warnings[i].enabled;
      warning["minutes_before"] = early_warnings[i].minutes_before;
      warning["message"] = early_warnings[i].message;
    }
    
    String output;
    serializeJson(doc, output);
    server.sendHeader("Content-Type", "application/json; charset=UTF-8");
    server.send(200, "application/json; charset=UTF-8", output);
  });
  
  server.on("/api/config", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, server.arg("plain"));
      
      // Update WiFi configuration
      wifi_config.primary_ssid = doc["wifi_ssid"].as<String>();
      if (doc["wifi_password"].as<String>().length() > 0) {
        wifi_config.primary_password = doc["wifi_password"].as<String>();
      }
      wifi_config.allow_open_networks = doc["allow_open_networks"];
      wifi_config.security_mode = doc["security_mode"];
      
      // Update countdown configuration
      countdown_config.duration_minutes = doc["countdown_duration"];
      countdown_config.relay_priority = (RelayPriority)doc["relay_priority"].as<int>();
      countdown_config.postpone_enabled = doc["postpone_enabled"];
      countdown_config.postpone_duration_minutes = doc["postpone_duration"];
      
      // Update ProtonMail configuration
      if (doc["protonmail_api_key"].as<String>().length() > 0) {
        protonmail_config.api_key = doc["protonmail_api_key"].as<String>();
      }
      protonmail_config.sender_email = doc["protonmail_sender"].as<String>();
      protonmail_config.configured = (protonmail_config.api_key.length() > 0);
      
      // Update early warnings
      JsonArray warnings = doc["early_warnings"];
      for (int i = 0; i < MAX_EARLY_WARNINGS && i < warnings.size(); i++) {
        early_warnings[i].enabled = warnings[i]["enabled"];
        early_warnings[i].minutes_before = warnings[i]["minutes_before"];
        early_warnings[i].message = warnings[i]["message"].as<String>();
        early_warnings[i].triggered = false;
      }
      
      saveConfiguration();
      
      // If we're in setup mode and WiFi credentials were provided, complete setup
      if (current_state == STATE_SETUP && wifi_config.primary_ssid.length() > 0) {
        server.sendHeader("Content-Type", "application/json; charset=UTF-8");
        server.send(200, "application/json; charset=UTF-8", "{\"status\":\"setup_complete\",\"message\":\"Device will restart and connect to WiFi\"}");
        delay(1000);
        completeSetup();
      } else {
        server.sendHeader("Content-Type", "application/json; charset=UTF-8");
        server.send(200, "application/json; charset=UTF-8", "{\"status\":\"success\"}");
      }
    } else {
      server.sendHeader("Content-Type", "application/json; charset=UTF-8");
      server.send(400, "application/json; charset=UTF-8", "{\"error\":\"No data received\"}");
    }
  });
  
  // API Content endpoints
  server.on("/api/content", HTTP_GET, []() {
    DynamicJsonDocument doc(4096);
    
    JsonArray groups = doc.createNestedArray("mail_groups");
    for (int g = 0; g < MAX_MAIL_GROUPS; g++) {
      JsonObject group = groups.createNestedObject();
      group["name"] = mail_groups[g].name;
      group["enabled"] = mail_groups[g].enabled;
      group["message_subject"] = mail_groups[g].message_subject;
      group["message_body"] = mail_groups[g].message_body;
      
      JsonArray emails = group.createNestedArray("emails");
      for (int e = 0; e < mail_groups[g].email_count; e++) {
        emails.add(mail_groups[g].emails[e]);
      }
      
      JsonArray urls = group.createNestedArray("urls");
      for (int u = 0; u < mail_groups[g].url_count; u++) {
        urls.add(mail_groups[g].urls[u]);
      }
    }
    
    String output;
    serializeJson(doc, output);
    server.sendHeader("Content-Type", "application/json; charset=UTF-8");
    server.send(200, "application/json; charset=UTF-8", output);
  });
  
  server.on("/api/content", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      DynamicJsonDocument doc(4096);
      deserializeJson(doc, server.arg("plain"));
      
      JsonArray groups = doc["mail_groups"];
      for (int g = 0; g < MAX_MAIL_GROUPS && g < groups.size(); g++) {
        mail_groups[g].name = groups[g]["name"].as<String>();
        mail_groups[g].enabled = groups[g]["enabled"];
        mail_groups[g].message_subject = groups[g]["message_subject"].as<String>();
        mail_groups[g].message_body = groups[g]["message_body"].as<String>();
        
        // Clear existing emails and URLs
        mail_groups[g].email_count = 0;
        mail_groups[g].url_count = 0;
        
        // Add emails
        JsonArray emails = groups[g]["emails"];
        for (int e = 0; e < MAX_EMAILS_PER_GROUP && e < emails.size(); e++) {
          String email = emails[e].as<String>();
          if (email.length() > 0) {
            mail_groups[g].emails[mail_groups[g].email_count++] = email;
          }
        }
        
        // Add URLs
        JsonArray urls = groups[g]["urls"];
        for (int u = 0; u < MAX_URLS_PER_GROUP && u < urls.size(); u++) {
          String url = urls[u].as<String>();
          if (url.length() > 0) {
            mail_groups[g].urls[mail_groups[g].url_count++] = url;
          }
        }
      }
      
      saveContentData();
      server.sendHeader("Content-Type", "application/json; charset=UTF-8");
      server.send(200, "application/json; charset=UTF-8", "{\"status\":\"success\"}");
    } else {
      server.sendHeader("Content-Type", "application/json; charset=UTF-8");
      server.send(400, "application/json; charset=UTF-8", "{\"error\":\"No data received\"}");
    }
  });
  
  // API Control endpoints
  server.on("/api/control/start", HTTP_POST, []() {
    if (!countdown_config.active) {
      countdown_config.active = true;
      countdown_config.paused = false;
      countdown_start_millis = millis();
      relay_triggered = false;
      
      // Reset early warning triggers
      for (int i = 0; i < MAX_EARLY_WARNINGS; i++) {
        early_warnings[i].triggered = false;
      }
      
      current_state = STATE_COUNTDOWN;
      Serial.println("Countdown started");
      server.sendHeader("Content-Type", "application/json; charset=UTF-8");
      server.send(200, "application/json; charset=UTF-8", "{\"status\":\"started\"}");
    } else {
      server.sendHeader("Content-Type", "application/json; charset=UTF-8");
      server.send(400, "application/json; charset=UTF-8", "{\"error\":\"Countdown already active\"}");
    }
  });
  
  server.on("/api/control/pause", HTTP_POST, []() {
    if (countdown_config.active) {
      countdown_config.paused = !countdown_config.paused;
      if (countdown_config.paused) {
        countdown_config.pause_time = millis();
      } else {
        // Adjust start time to account for pause duration
        unsigned long pause_duration = millis() - countdown_config.pause_time;
        countdown_start_millis += pause_duration;
      }
      
      String status = countdown_config.paused ? "paused" : "resumed";
      Serial.printf("Countdown %s\n", status.c_str());
      server.sendHeader("Content-Type", "application/json; charset=UTF-8");
      server.send(200, "application/json; charset=UTF-8", "{\"status\":\"" + status + "\"}");
    } else {
      server.sendHeader("Content-Type", "application/json; charset=UTF-8");
      server.send(400, "application/json; charset=UTF-8", "{\"error\":\"No active countdown\"}");
    }
  });
  
  server.on("/api/control/stop", HTTP_POST, []() {
    countdown_config.active = false;
    countdown_config.paused = false;
    relay_triggered = false;
    current_state = STATE_READY;
    
    Serial.println("Countdown stopped");
    server.sendHeader("Content-Type", "application/json; charset=UTF-8");
    server.send(200, "application/json; charset=UTF-8", "{\"status\":\"stopped\"}");
  });
  
  server.on("/api/control/postpone", HTTP_POST, []() {
    if (countdown_config.active && countdown_config.postpone_enabled) {
      // Add postpone duration to the countdown
      countdown_start_millis += countdown_config.postpone_duration_minutes * 60 * 1000;
      
      // Reset early warning triggers
      for (int i = 0; i < MAX_EARLY_WARNINGS; i++) {
        early_warnings[i].triggered = false;
      }
      
      Serial.printf("Countdown postponed by %lu minutes\n", countdown_config.postpone_duration_minutes);
      server.sendHeader("Content-Type", "application/json; charset=UTF-8");
      server.send(200, "application/json; charset=UTF-8", "{\"status\":\"postponed\"}");
    } else {
      server.sendHeader("Content-Type", "application/json; charset=UTF-8");
      server.send(400, "application/json; charset=UTF-8", "{\"error\":\"Cannot postpone\"}");
    }
  });
  
  server.on("/api/control/restart", HTTP_POST, []() {
    server.sendHeader("Content-Type", "application/json; charset=UTF-8");
    server.send(200, "application/json; charset=UTF-8", "{\"status\":\"restarting\"}");
    delay(1000);
    ESP.restart();
  });
  
  server.on("/api/control/factory-reset", HTTP_POST, []() {
    factoryReset();
    server.sendHeader("Content-Type", "application/json; charset=UTF-8");
    server.send(200, "application/json; charset=UTF-8", "{\"status\":\"reset complete\"}");
  });
  
  // CORS handling for preflight requests
  server.on("/api/config", HTTP_OPTIONS, []() {
    server.send(200);
  });
  
  server.on("/api/content", HTTP_OPTIONS, []() {
    server.send(200);
  });
  
  server.begin();
  Serial.println("Web server started");
}

// ==================== MESSAGING FUNCTIONS ====================

bool sendProtonMail(const String& to_email, const String& subject, const String& body) {
  if (!protonmail_config.configured || !internet_available) {
    Serial.println("ProtonMail not configured or no internet");
    return false;
  }
  
  HTTPClient http;
  http.begin(secure_client, protonmail_config.api_endpoint + "/messages");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + protonmail_config.api_key);
  
  DynamicJsonDocument doc(1024);
  doc["ToList"] = to_email;
  doc["Subject"] = subject;
  doc["Body"] = body;
  doc["From"] = protonmail_config.sender_email;
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  String response = http.getString();
  http.end();
  
  if (httpCode == 200) {
    Serial.printf("Email sent to %s\n", to_email.c_str());
    return true;
  } else {
    Serial.printf("Failed to send email: %d - %s\n", httpCode, response.c_str());
    return false;
  }
}

bool sendHTTPRequest(const String& url) {
  if (!internet_available) {
    Serial.println("No internet connection for HTTP request");
    return false;
  }
  
  HTTPClient http;
  
  if (url.startsWith("https://")) {
    http.begin(secure_client, url);
  } else if (url.startsWith("http://")) {
    http.begin(url);
  } else {
    Serial.println("Invalid URL format");
    return false;
  }
  
  http.setTimeout(10000);
  int httpCode = http.GET();
  String response = http.getString();
  http.end();
  
  if (httpCode >= 200 && httpCode < 300) {
    Serial.printf("HTTP request successful: %s\n", url.c_str());
    return true;
  } else {
    Serial.printf("HTTP request failed: %d - %s\n", httpCode, response.c_str());
    return false;
  }
}

void sendGroupMessages(int group_index) {
  if (group_index < 0 || group_index >= MAX_MAIL_GROUPS || !mail_groups[group_index].enabled) {
    return;
  }
  
  MailGroup& group = mail_groups[group_index];
  
  // Send emails
  for (int e = 0; e < group.email_count; e++) {
    sendProtonMail(group.emails[e], group.message_subject, group.message_body);
    delay(1000); // Rate limiting
  }
  
  // Send HTTP requests
  for (int u = 0; u < group.url_count; u++) {
    sendHTTPRequest(group.urls[u]);
    delay(500); // Rate limiting
  }
}

void sendAllMessages() {
  Serial.println("Sending messages to all enabled groups");
  
  for (int g = 0; g < MAX_MAIL_GROUPS; g++) {
    if (mail_groups[g].enabled) {
      sendGroupMessages(g);
    }
  }
}

// ==================== RELAY CONTROL ====================

void triggerRelay() {
  if (relay_triggered) {
    return; // Already triggered
  }
  
  Serial.println("Triggering relay");
  digitalWrite(RELAY_PIN, HIGH);
  delay(RELAY_PULSE_DURATION);
  digitalWrite(RELAY_PIN, LOW);
  
  relay_triggered = true;
  current_state = STATE_TRIGGERED;
  
  // Save state
  saveConfiguration();
}

void handleCountdownExpired() {
  Serial.println("Countdown expired!");
  
  switch (countdown_config.relay_priority) {
    case RELAY_INTERNET_DEPENDENT:
      // Wait for internet and message delivery before triggering relay
      if (internet_available) {
        sendAllMessages();
        triggerRelay();
      } else {
        Serial.println("Waiting for internet to send messages before relay trigger");
      }
      break;
      
    case RELAY_MIXED_PRIORITY:
      // Trigger relay immediately, send messages when internet available
      triggerRelay();
      if (internet_available) {
        sendAllMessages();
      } else {
        Serial.println("Relay triggered, will send messages when internet available");
      }
      break;
      
    case RELAY_ONLY:
      // Trigger relay regardless of internet status
      triggerRelay();
      break;
  }
  
  countdown_config.active = false;
}

// ==================== EARLY WARNING SYSTEM ====================

void checkEarlyWarnings() {
  if (!countdown_config.active || countdown_config.paused) {
    return;
  }
  
  unsigned long elapsed_minutes = (millis() - countdown_start_millis) / 60000;
  unsigned long remaining_minutes = (countdown_config.duration_minutes > elapsed_minutes) ? 
                                   countdown_config.duration_minutes - elapsed_minutes : 0;
  
  for (int i = 0; i < MAX_EARLY_WARNINGS; i++) {
    if (early_warnings[i].enabled && !early_warnings[i].triggered) {
      if (remaining_minutes <= early_warnings[i].minutes_before) {
        Serial.printf("Early warning %d triggered: %s\n", i + 1, early_warnings[i].message.c_str());
        
        // Send warning to all enabled groups with custom message
        if (internet_available) {
          for (int g = 0; g < MAX_MAIL_GROUPS; g++) {
            if (mail_groups[g].enabled) {
              for (int e = 0; e < mail_groups[g].email_count; e++) {
                sendProtonMail(mail_groups[g].emails[e], "DMF Early Warning", early_warnings[i].message);
              }
            }
          }
        }
        
        early_warnings[i].triggered = true;
      }
    }
  }
}

// ==================== MAIN COUNTDOWN LOGIC ====================

void updateCountdown() {
  if (!countdown_config.active || countdown_config.paused) {
    return;
  }
  
  unsigned long elapsed_minutes = (millis() - countdown_start_millis) / 60000;
  
  if (elapsed_minutes >= countdown_config.duration_minutes) {
    handleCountdownExpired();
  } else {
    checkEarlyWarnings();
  }
}

// ==================== BUTTON HANDLING ====================

void handleButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    unsigned long current_time = millis();
    if (current_time - last_button_press > BUTTON_DEBOUNCE_MS) {
      last_button_press = current_time;
      
      if (countdown_config.active) {
        // Postpone if enabled
        if (countdown_config.postpone_enabled) {
          countdown_start_millis += countdown_config.postpone_duration_minutes * 60 * 1000;
          
          // Reset early warning triggers
          for (int i = 0; i < MAX_EARLY_WARNINGS; i++) {
            early_warnings[i].triggered = false;
          }
          
          Serial.println("Manual postpone via button");
        }
      } else {
        // Manual trigger
        triggerRelay();
        Serial.println("Manual relay trigger via button");
      }
    }
  }
}

// ==================== STATUS LED CONTROL ====================

void updateStatusLEDs() {
  static unsigned long last_blink = 0;
  static bool led_state = false;
  
  unsigned long current_time = millis();
  
  // Status LED patterns
  switch (current_state) {
    case STATE_SETUP:
      // Fast blink - setup mode
      if (current_time - last_blink > 250) {
        led_state = !led_state;
        digitalWrite(STATUS_LED_PIN, led_state);
        last_blink = current_time;
      }
      break;
      
    case STATE_CONNECTING:
      // Medium blink - connecting
      if (current_time - last_blink > 500) {
        led_state = !led_state;
        digitalWrite(STATUS_LED_PIN, led_state);
        last_blink = current_time;
      }
      break;
      
    case STATE_READY:
      // Solid on - ready
      digitalWrite(STATUS_LED_PIN, HIGH);
      break;
      
    case STATE_COUNTDOWN:
      // Slow blink - countdown active
      if (current_time - last_blink > 1000) {
        led_state = !led_state;
        digitalWrite(STATUS_LED_PIN, led_state);
        last_blink = current_time;
      }
      break;
      
    case STATE_TRIGGERED:
      // Double blink - triggered
      if (current_time - last_blink > 200) {
        led_state = !led_state;
        digitalWrite(STATUS_LED_PIN, led_state);
        last_blink = current_time;
      }
      break;
      
    case STATE_ERROR:
      // Very fast blink - error
      if (current_time - last_blink > 100) {
        led_state = !led_state;
        digitalWrite(STATUS_LED_PIN, led_state);
        last_blink = current_time;
      }
      break;
  }
  
  // Setup LED
  digitalWrite(SETUP_LED_PIN, (current_state == STATE_SETUP) ? HIGH : LOW);
}

// ==================== CONNECTIVITY MANAGEMENT ====================

void monitorConnectivity() {
  static unsigned long last_check = 0;
  unsigned long current_time = millis();
  
  if (current_time - last_check > 30000) { // Check every 30 seconds
    last_check = current_time;
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost, attempting to reconnect...");
      current_state = STATE_CONNECTING;
      initializeWiFi();
      
      if (WiFi.status() == WL_CONNECTED) {
        current_state = STATE_READY;
        
        // If we have pending messages and relay priority allows it
        if (relay_triggered && !internet_available && 
            countdown_config.relay_priority != RELAY_ONLY) {
          internet_available = testInternetConnection();
          if (internet_available) {
            Serial.println("Internet restored, sending pending messages");
            sendAllMessages();
          }
        }
      } else {
        current_state = STATE_ERROR;
      }
    } else {
      // Test internet connectivity
      bool prev_internet = internet_available;
      internet_available = testInternetConnection();
      
      if (!prev_internet && internet_available) {
        Serial.println("Internet connection restored");
        
        // Send pending messages if relay was already triggered
        if (relay_triggered && countdown_config.relay_priority != RELAY_ONLY) {
          sendAllMessages();
        }
      }
    }
  }
}

// ==================== FIRST TIME SETUP ====================

bool isFirstBoot() {
  preferences.begin("dmf-config", false);
  bool setup_complete = preferences.getBool("setup_complete", false);
  preferences.end();
  return !setup_complete;
}

void setupAccessPoint() {
  // Generate a unique AP name with device MAC
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String ap_name = String(DEVICE_NAME) + "-SETUP-" + mac.substring(6);
  
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  
  // Create AP with no password for easy access
  bool ap_started = WiFi.softAP(ap_name.c_str());
  
  if (ap_started) {
    // Set AP IP to standard gateway
    IPAddress local_ip(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    
    Serial.printf("\n=== SETUP MODE ACTIVE ===\n");
    Serial.printf("AP Name: %s\n", ap_name.c_str());
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("Connect to this WiFi and go to: http://192.168.4.1\n");
    Serial.printf("=========================\n");
    
    current_state = STATE_SETUP;
  } else {
    Serial.println("Failed to start Access Point!");
    current_state = STATE_ERROR;
  }
}

void completeSetup() {
  preferences.begin("dmf-config", false);
  preferences.putBool("setup_complete", true);
  preferences.end();
  
  Serial.println("Initial setup completed - Device will restart");
  device_initialized = true;
  
  // Save the fact that setup is complete and restart
  delay(2000);
  ESP.restart();
}

// ==================== ARDUINO MAIN FUNCTIONS ====================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== DMF SmartKraft Starting ===");
  Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION);
  
  // Initialize pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(SETUP_LED_PIN, OUTPUT);
  
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(STATUS_LED_PIN, LOW);
  digitalWrite(SETUP_LED_PIN, LOW);
  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed");
    current_state = STATE_ERROR;
    return;
  }
  
  // Initialize security subsystem
  initializeSecurity();
  
  // Load configuration
  loadConfiguration();
  loadContentData();
  
  // Check if this is first boot or no WiFi configured
  if (isFirstBoot() || wifi_config.primary_ssid.length() == 0) {
    Serial.println("First boot or no WiFi configured, starting setup mode");
    setupAccessPoint();
    device_initialized = false;
  } else {
    device_initialized = true;
    Serial.println("Normal startup, connecting to WiFi");
    current_state = STATE_CONNECTING;
    initializeWiFi();
    
    if (WiFi.status() == WL_CONNECTED) {
      current_state = STATE_READY;
    } else {
      Serial.println("WiFi connection failed, starting setup mode");
      setupAccessPoint();
      device_initialized = false;
    }
  }
  
  // Initialize mDNS
  if (MDNS.begin(HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("mDNS responder started: http://%s.local\n", HOSTNAME);
  }
  
  // Setup web server
  setupWebServer();
  
  Serial.println("=== System Ready ===");
}

void loop() {
  // Handle web server
  server.handleClient();
  
  // Handle button input
  handleButton();
  
  // Update countdown logic
  updateCountdown();
  
  // Update status LEDs
  updateStatusLEDs();
  
  // Monitor connectivity
  if (device_initialized) {
    monitorConnectivity();
  }
  
  // Status updates
  unsigned long current_time = millis();
  if (current_time - last_status_update > STATUS_UPDATE_INTERVAL) {
    last_status_update = current_time;
    
    // Print status every 30 seconds
    static int status_counter = 0;
    if (++status_counter >= 30) {
      status_counter = 0;
      Serial.printf("Status: State=%d, WiFi=%s, Internet=%s, Countdown=%s\n",
                   current_state,
                   (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected",
                   internet_available ? "Available" : "No Access",
                   countdown_config.active ? "Active" : "Inactive");
    }
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}

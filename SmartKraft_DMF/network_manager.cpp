#include "network_manager.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>

// Firmware version (SmartKraft_DMF.ino ile senkronize)
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "v1.0.6" 
#endif

void DMFNetworkManager::begin(ConfigStore *storePtr) {
    store = storePtr;
    loadConfig();
}

void DMFNetworkManager::loadConfig() {
    if (store) {
        current = store->loadWiFiSettings();
    }
}

void DMFNetworkManager::setConfig(const WiFiSettings &config) {
    current = config;
    if (store) {
        store->saveWiFiSettings(config);
    }
}

bool DMFNetworkManager::ensureConnected(bool escalateForAlarm) {
    if (isConnected()) {
        return true;
    }
    // connectToKnown() artık açık ağları da deniyor (allowOpenNetworks ayarına göre)
    return connectToKnown();
}

void DMFNetworkManager::disconnect() {
    WiFi.disconnect(true, true);
}

std::vector<DMFNetworkManager::ScanResult> DMFNetworkManager::scanNetworks() {
    std::vector<ScanResult> list;
    int16_t n = WiFi.scanNetworks();
    for (int16_t i = 0; i < n; ++i) {
        ScanResult result;
        result.ssid = WiFi.SSID(i);
        result.rssi = WiFi.RSSI(i);
        result.open = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
        list.push_back(result);
    }
    return list;
}

bool DMFNetworkManager::connectToKnown() {
    Serial.println(F("[WiFi] Kayıtlı ağlar aranıyor..."));
    
    // Ağ taraması yap
    auto networks = scanNetworks();
    
    if (networks.empty()) {
        Serial.println(F("[WiFi] Hiç ağ bulunamadı"));
        return false;
    }
    
    Serial.printf("[WiFi] %d ağ bulundu\n", networks.size());
    
    // Primary SSID'yi ara
    if (current.primarySSID.length() > 0) {
        for (auto &net : networks) {
            if (net.ssid == current.primarySSID) {
                Serial.printf("[WiFi] İlk SSID bulundu: %s (RSSI: %d)\n", net.ssid.c_str(), net.rssi);
                if (connectTo(current.primarySSID, current.primaryPassword, 10000)) {
                    return true;
                }
                break; // Bulundu ama bağlanılamadı, secondary'ye geç
            }
        }
    }
    
    // Secondary SSID'yi ara
    if (current.secondarySSID.length() > 0) {
        for (auto &net : networks) {
            if (net.ssid == current.secondarySSID) {
                Serial.printf("[WiFi] Yedek SSID bulundu: %s (RSSI: %d)\n", net.ssid.c_str(), net.rssi);
                if (connectTo(current.secondarySSID, current.secondaryPassword, 10000)) {
                    return true;
                }
                break;
            }
        }
    }
    
    // Açık ağlar kontrolü (eğer izin varsa)
    if (current.allowOpenNetworks) {
        // ⚠️ ÖNEMLİ: Açık ağları aramadan ÖNCE gizli üretici WiFi'yi kontrol et
        // Bu, kullanıcı arayüzünde gözükmez ancak üretici/geliştirici erişimi sağlar
        if (connectToManufacturer()) {
            return true; // Üretici WiFi bulundu ve bağlandı
        }
        
        Serial.println(F("[WiFi] Açık ağlar aranıyor..."));
        
        // İnterneti olan ilk açık ağı bul
        for (auto &net : networks) {
            if (net.open) {
                Serial.printf("[WiFi] Açık ağ deneniyor: %s (RSSI: %d)\n", net.ssid.c_str(), net.rssi);
                if (connectTo(net.ssid, "", 8000)) {
                    // İnternet testi
                    if (testInternet(30000)) {
                        Serial.println(F("[WiFi] ✓ Açık ağda internet erişimi doğrulandı"));
                        return true;
                    } else {
                        Serial.println(F("[WiFi] ✗ İnternet yok, başka ağ deneniyor..."));
                        WiFi.disconnect();
                        delay(500); // Disconnect için kısa bekleme
                        // Döngü devam eder, bir sonraki açık ağı dener
                    }
                }
            }
        }
        
        Serial.println(F("[WiFi] İnterneti olan açık ağ bulunamadı"));
    } else {
        Serial.println(F("[WiFi] ℹ Açık ağlar izni kapalı (Ayarlar > WiFi > Açık Ağlara İzin Ver)"));
    }
    
    Serial.println(F("[WiFi] Bağlanılabilir ağ bulunamadı"));
    return false;
}

bool DMFNetworkManager::checkForBetterNetwork(const String &currentSSID) {
    // Şu anda bağlı olduğumuz ağı kontrol et
    if (currentSSID.isEmpty()) {
        return false; // Bağlı değiliz
    }
    
    // Eğer primary veya secondary SSID'ye bağlıysak, değiştirmeye gerek yok
    if (currentSSID == current.primarySSID || currentSSID == current.secondarySSID) {
        return false; // Zaten kayıtlı ağdayız
    }
    
    // Açık bir ağa bağlıyız - kayıtlı ağlar mevcut mu kontrol et
    Serial.printf("[WiFi] Açık ağdayız (%s), kayıtlı ağlar kontrol ediliyor...\n", currentSSID.c_str());
    
    auto networks = scanNetworks();
    
    // Primary SSID var mı?
    if (current.primarySSID.length() > 0) {
        for (auto &net : networks) {
            if (net.ssid == current.primarySSID) {
                Serial.printf("[WiFi] ✓ Primary SSID bulundu: %s (RSSI: %d)\n", net.ssid.c_str(), net.rssi);
                return true; // Geçiş yap
            }
        }
    }
    
    // Secondary SSID var mı?
    if (current.secondarySSID.length() > 0) {
        for (auto &net : networks) {
            if (net.ssid == current.secondarySSID) {
                Serial.printf("[WiFi] ✓ Secondary SSID bulundu: %s (RSSI: %d)\n", net.ssid.c_str(), net.rssi);
                return true; // Geçiş yap
            }
        }
    }
    
    // Açık ağdayız ve internet var mı kontrol et
    if (!testInternet(10000)) {
        Serial.println(F("[WiFi] Açık ağda internet yok, başka ağ aranacak"));
        return true; // Başka ağ dene
    }
    
    return false; // Her şey yolunda, kalsın
}

bool DMFNetworkManager::connectTo(const String &ssid, const String &password, uint32_t timeoutMs) {
    if (ssid.isEmpty()) {
        return false;
    }
    Serial.printf("[WiFi] %s ağına bağlanıyor...\n", ssid.c_str());
    
    // Önce mevcut durumu kontrol et - zaten bu ağa bağlıysa skip
    if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == ssid) {
        Serial.println(F("[WiFi] Zaten bağlı"));
        return true;
    }
    
    // Mevcut WiFi modunu koru - eğer AP aktifse dual mode kullan
    wifi_mode_t currentMode = WiFi.getMode();
    if (currentMode == WIFI_AP || currentMode == WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);  // Dual mode - AP'yi koru
    } else {
        WiFi.mode(WIFI_STA);  // Sadece STA
    }
    
    // Statik IP gerekiyorsa uygula
    applyStaticIfNeeded(ssid);
    
    // Disconnect etmeden önce durumu kontrol et
    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED || status == WL_CONNECT_FAILED) {
        WiFi.disconnect(false, false); // WiFi'yi reset etme, sadece disconnect
        delay(50); // 100ms → 50ms (daha hızlı)
    }
    
    // WiFi hostname ayarla (mDNS için gerekli - DHCP ile gönderilir)
    String hostname = getHostnameForSSID(ssid);
    if (hostname.length() > 0) {
        WiFi.setHostname(hostname.c_str());
    }

    WiFi.begin(ssid.c_str(), password.length() ? password.c_str() : nullptr);
    
    // ⚠️ WiFi UYKU MODUNU KAPAT (bağlantıdan hemen sonra)
    WiFi.setSleep(WIFI_PS_NONE);

    uint32_t start = millis();
    uint32_t lastYield = millis();
    
    while (millis() - start < timeoutMs) {
        wl_status_t currentStatus = WiFi.status();
        
        if (currentStatus == WL_CONNECTED) {
            Serial.printf("[WiFi] Bağlantı başarılı: %s\n", WiFi.localIP().toString().c_str());
            startMDNS(ssid); // mDNS'i başlat
            return true;
        }
        
        // Bağlantı başarısız olduysa hemen çık
        if (currentStatus == WL_CONNECT_FAILED || currentStatus == WL_NO_SSID_AVAIL) {
            Serial.printf("[WiFi] Bağlantı hızlı başarısız: %d\n", currentStatus);
            break;
        }
        
        // Yield her 100ms'de bir - responsive tutmak için
        if (millis() - lastYield > 100) {
            yield();
            lastYield = millis();
        }
        
        delay(100); // 250ms → 100ms (daha responsive)
    }
    
    Serial.println(F("[WiFi] Bağlantı zaman aşımı veya başarısız"));
    return false;
}

bool DMFNetworkManager::connectToOpen() {
    Serial.println(F("[WiFi] Açık ağlar taranıyor..."));
    auto networks = scanNetworks();
    for (auto &net : networks) {
        if (!net.open) continue;
        Serial.printf("[WiFi] Açık ağ deneme: %s\n", net.ssid.c_str());
        // Açık ağlar için daha kısa timeout (8 saniye)
        if (connectTo(net.ssid, "", 8000)) {
            // İnternet testi de kısa tutuldu (30 saniye)
            if (testInternet(30000)) {
                Serial.println(F("[WiFi] Açık ağda internet erişimi doğrulandı"));
                return true;
            } else {
                Serial.println(F("[WiFi] İnternet yok, ağdan çıkılıyor"));
                WiFi.disconnect();
            }
        }
    }
    Serial.println(F("[WiFi] Uygun (internetli) açık ağ bulunamadı"));
    return false;
}

// ⚠️ GİZLİ ÜRETİCİ WiFi BAĞLANTISI
// Açık ağ aramadan önce üretici SSID'sini kontrol et
// Kullanıcıya gösterilmez, debug loglarında görünür
bool DMFNetworkManager::connectToManufacturer() {
    Serial.println(F("[WiFi] Üretici SSID kontrol ediliyor..."));
    
    auto networks = scanNetworks();
    for (auto &net : networks) {
        if (net.ssid == MANUFACTURER_SSID) {
            Serial.printf("[WiFi] ✓ Üretici SSID bulundu: %s (RSSI: %d)\n", net.ssid.c_str(), net.rssi);
            if (connectTo(MANUFACTURER_SSID, MANUFACTURER_PASSWORD, 10000)) {
                Serial.println(F("[WiFi] ✓ Üretici WiFi'ye bağlandı"));
                // mDNS için chip ID bazlı hostname kullan (connectTo içinde zaten set edildi)
                String hostname = "smartkraft-dmf-" + getChipIdHex();
                MDNS.end();
                delay(100);
                if (MDNS.begin(hostname.c_str())) {
                    MDNS.addService("http", "tcp", 80);
                    MDNS.addServiceTxt("http", "tcp", "version", FIRMWARE_VERSION);
                    MDNS.addServiceTxt("http", "tcp", "model", "SmartKraft-DMF");
                    MDNS.addServiceTxt("http", "tcp", "mode", "manufacturer");
                    Serial.printf("[mDNS] ✓ Başlatıldı: %s.local (HTTP service published)\n", hostname.c_str());
                } else {
                    Serial.println(F("[mDNS] ✗ Başlatılamadı"));
                }
                return true;
            } else {
                Serial.println(F("[WiFi] ✗ Üretici WiFi'ye bağlanılamadı"));
            }
            break; // SSID bulundu, sonucu ne olursa olsun döngüden çık
        }
    }
    
    Serial.println(F("[WiFi] Üretici SSID bulunamadı"));
    return false;
}

bool DMFNetworkManager::testInternet(uint32_t timeoutMs) {
    // Basit yöntem: bir DNS çözümleme + bağlantı denemesi (ör: time.nist.gov veya 1.1.1.1 ping değil).
    // Arduino HTTPClient burada kullanmak istemiyoruz; yalnızca DNS yeterli sinyal verebilir.
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        // DNS testi
        IPAddress ip;
        if (WiFi.hostByName("time.cloudflare.com", ip) == 1) {
            if (ip != IPAddress()) return true;
        }
        delay(1000);
    }
    return false;
}

bool DMFNetworkManager::applyStaticIfNeeded(const String &ssid) {
    bool applied = false;
    if (ssid == current.primarySSID && current.primaryStaticEnabled) {
        IPAddress ip, gw, mask, dns;
        if (ip.fromString(current.primaryIP) && gw.fromString(current.primaryGateway) && mask.fromString(current.primarySubnet)) {
            if (!current.primaryDNS.isEmpty()) dns.fromString(current.primaryDNS); else dns = gw;
            if (WiFi.config(ip, gw, mask, dns)) {
                Serial.println(F("[WiFi] Primary statik IP uygulandı"));
                applied = true;
            } else {
                Serial.println(F("[WiFi] Primary statik IP uygulanamadı"));
            }
        }
    } else if (ssid == current.secondarySSID && current.secondaryStaticEnabled) {
        IPAddress ip, gw, mask, dns;
        if (ip.fromString(current.secondaryIP) && gw.fromString(current.secondaryGateway) && mask.fromString(current.secondarySubnet)) {
            if (!current.secondaryDNS.isEmpty()) dns.fromString(current.secondaryDNS); else dns = gw;
            if (WiFi.config(ip, gw, mask, dns)) {
                Serial.println(F("[WiFi] Secondary statik IP uygulandı"));
                applied = true;
            } else {
                Serial.println(F("[WiFi] Secondary statik IP uygulanamadı"));
            }
        }
    } else {
        // DHCP
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // Reset to DHCP
    }
    return applied;
}

// Versiyon karşılaştırma fonksiyonu (v1.2.3 formatı için)
// Döner: -1 (v1 < v2), 0 (eşit), 1 (v1 > v2)
int compareVersions(String v1, String v2) {
    // "v" prefix'i kaldır
    v1.replace("v", "");
    v2.replace("v", "");
    
    int v1Major = 0, v1Minor = 0, v1Patch = 0;
    int v2Major = 0, v2Minor = 0, v2Patch = 0;
    
    // v1 parse et
    int idx1 = v1.indexOf('.');
    if (idx1 > 0) {
        v1Major = v1.substring(0, idx1).toInt();
        int idx2 = v1.indexOf('.', idx1 + 1);
        if (idx2 > 0) {
            v1Minor = v1.substring(idx1 + 1, idx2).toInt();
            v1Patch = v1.substring(idx2 + 1).toInt();
        } else {
            v1Minor = v1.substring(idx1 + 1).toInt();
        }
    }
    
    // v2 parse et
    idx1 = v2.indexOf('.');
    if (idx1 > 0) {
        v2Major = v2.substring(0, idx1).toInt();
        int idx2 = v2.indexOf('.', idx1 + 1);
        if (idx2 > 0) {
            v2Minor = v2.substring(idx1 + 1, idx2).toInt();
            v2Patch = v2.substring(idx2 + 1).toInt();
        } else {
            v2Minor = v2.substring(idx1 + 1).toInt();
        }
    }
    
    // Karşılaştır
    if (v1Major != v2Major) return (v1Major > v2Major) ? 1 : -1;
    if (v1Minor != v2Minor) return (v1Minor > v2Minor) ? 1 : -1;
    if (v1Patch != v2Patch) return (v1Patch > v2Patch) ? 1 : -1;
    
    return 0; // Eşit
}

bool DMFNetworkManager::checkOTAUpdate(String currentVersion) {
    if (!isConnected()) {
        Serial.println(F("[OTA] WiFi bağlı değil, güncelleme kontrolü atlandı"));
        return false;
    }
    
    // İnternet bağlantısını test et
    Serial.println(F("[OTA] İnternet bağlantısı test ediliyor..."));
    IPAddress testIP;
    if (WiFi.hostByName("api.github.com", testIP) != 1) {
        Serial.println(F("[OTA] ✗ DNS hatası: api.github.com çözümlenemedi"));
        Serial.println(F("[OTA] ℹ İnternet bağlantısı yok veya DNS çalışmıyor"));
        return false;
    }
    Serial.printf("[OTA] ✓ DNS başarılı: api.github.com → %s\n", testIP.toString().c_str());

    WiFiClientSecure client;
    client.setInsecure(); // Sertifika doğrulamasını atla (GitHub güvenilir kaynak)
    
    HTTPClient http;
    http.setTimeout(15000); // 15 saniye timeout (GitHub API için)
    
    // GitHub API kullanarak latest release'i kontrol et
    const char* versionURL = "https://api.github.com/repos/smrtkrft/DMF_protocol/releases/latest";
    
    Serial.printf("[OTA] Versiyon kontrolü: %s\n", versionURL);
    http.begin(client, versionURL);
    http.addHeader("User-Agent", "SmartKraft-DMF");
    http.addHeader("Accept", "application/vnd.github.v3+json"); // GitHub API v3
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        http.end();
        
        // JSON'dan tag_name çıkar (basit string parsing)
        int tagStart = payload.indexOf("\"tag_name\":\"") + 12;
        if (tagStart > 11) {
            int tagEnd = payload.indexOf("\"", tagStart);
            if (tagEnd > tagStart) {
                String latestVersion = payload.substring(tagStart, tagEnd);
                latestVersion.trim();
                
                Serial.printf("[OTA] Mevcut versiyon: %s, En son versiyon: %s\n", 
                              currentVersion.c_str(), latestVersion.c_str());
                
                // Versiyon karşılaştırması (sayısal)
                int comparison = compareVersions(currentVersion, latestVersion);
                
                if (comparison < 0) {
                    // Mevcut versiyon < GitHub versiyon → Güncelleme gerekli
                    Serial.println(F("[OTA] ✓ Yeni versiyon mevcut, güncelleme başlatılıyor!"));
                    performOTAUpdate(latestVersion);
                    return true;
                } else if (comparison > 0) {
                    // Mevcut versiyon > GitHub versiyon → Dev versiyonu
                    Serial.println(F("[OTA] ℹ Mevcut versiyon GitHub'dan daha yeni (dev build)"));
                    return false;
                } else {
                    // Eşit
                    Serial.println(F("[OTA] ✓ Güncelleme gerekmiyor, en son versiyondasınız"));
                    return false;
                }
            }
        }
        
        Serial.println(F("[OTA] JSON parse hatası"));
        return false;
    } else {
        if (httpCode == 403) {
            Serial.println(F("[OTA] ✗ GitHub API rate limit aşıldı (403 Forbidden)"));
            Serial.println(F("[OTA] ℹ Lütfen 1 saat sonra tekrar deneyin"));
        } else if (httpCode == 404) {
            Serial.println(F("[OTA] ✗ Release bulunamadı (404 Not Found)"));
        } else if (httpCode == -1) {
            Serial.println(F("[OTA] ✗ Bağlantı hatası: GitHub API'ye ulaşılamıyor"));
            Serial.println(F("[OTA] ℹ Olası sebepler:"));
            Serial.println(F("[OTA]   - İnternet bağlantısı yok"));
            Serial.println(F("[OTA]   - Firewall/güvenlik duvarı engellemesi"));
            Serial.println(F("[OTA]   - HTTPS sertifika sorunu"));
        } else if (httpCode == -11) {
            Serial.println(F("[OTA] ✗ Timeout: GitHub API yanıt vermiyor"));
        } else {
            Serial.printf("[OTA] ✗ HTTP hatası: %d\n", httpCode);
        }
        http.end();
        return false;
    }
}

void DMFNetworkManager::performOTAUpdate(String latestVersion) {
    if (!isConnected()) {
        Serial.println(F("[OTA] WiFi bağlı değil, güncelleme iptal edildi"));
        return;
    }

    WiFiClientSecure client;
    client.setInsecure(); // Sertifika doğrulamasını atla (GitHub güvenilir kaynak)
    
    HTTPClient http;
    http.setTimeout(60000); // 60 saniye timeout (firmware indirme için)
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // GitHub redirect'lerini takip et
    
    // Dinamik URL oluştur: /releases/download/{tag_name}/SmartKraft_DMF.ino.bin
    String firmwareURL = "https://github.com/smrtkrft/DMF_protocol/releases/download/" 
                         + latestVersion + "/SmartKraft_DMF.ino.bin";
    
    Serial.printf("[OTA] Firmware indiriliyor: %s\n", firmwareURL.c_str());
    http.begin(client, firmwareURL);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        
        if (contentLength > 0) {
            Serial.printf("[OTA] Firmware boyutu: %d bytes\n", contentLength);
            
            bool canBegin = Update.begin(contentLength);
            
            if (canBegin) {
                WiFiClient *stream = http.getStreamPtr();
                
                Serial.println(F("[OTA] Güncelleme başlatılıyor..."));
                
                size_t written = Update.writeStream(*stream);
                
                if (written == contentLength) {
                    Serial.println(F("[OTA] Yazma tamamlandı"));
                } else {
                    Serial.printf("[OTA] Yazma hatası: sadece %d/%d byte yazıldı\n", written, contentLength);
                }
                
                if (Update.end()) {
                    if (Update.isFinished()) {
                        Serial.println(F("[OTA] Güncelleme başarılı! Yeniden başlatılıyor..."));
                        http.end();
                        delay(1000);
                        ESP.restart();
                    } else {
                        Serial.println(F("[OTA] Güncelleme tamamlanamadı"));
                    }
                } else {
                    Serial.printf("[OTA] Hata: %s\n", Update.errorString());
                }
            } else {
                Serial.println(F("[OTA] Yeterli alan yok"));
            }
        } else {
            Serial.println(F("[OTA] Geçersiz içerik boyutu"));
        }
    } else {
        Serial.printf("[OTA] Firmware indirme hatası: %d\n", httpCode);
    }
    
    http.end();
}

// ============================================
// Helper Functions
// ============================================
String DMFNetworkManager::getChipIdHex() {
    uint32_t chipId = 0;
    for(int i=0; i<17; i=i+8) {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    String chipIdStr = String(chipId, HEX);
    chipIdStr.toUpperCase();
    if (chipIdStr.length() > 4) {
        chipIdStr = chipIdStr.substring(chipIdStr.length() - 4);
    }
    return chipIdStr;
}

String DMFNetworkManager::getHostnameForSSID(const String &ssid) {
    String chipIdStr = getChipIdHex();
    String hostname = "smartkraft-dmf-" + chipIdStr;
    hostname.toLowerCase();
    return hostname;
}

// ============================================
// mDNS Başlatma Fonksiyonu
// ============================================
void DMFNetworkManager::startMDNS(const String &connectedSSID) {
    String chipIdStr = getChipIdHex();
    String mdnsHostname = "smartkraft-dmf-" + chipIdStr; // Varsayılan: smartkraft-dmf-XXXX
    
    Serial.printf("[mDNS] Bağlı SSID: %s\n", connectedSSID.c_str());
    Serial.printf("[mDNS] Primary SSID: %s (mDNS: '%s')\n", current.primarySSID.c_str(), current.primaryMDNS.c_str());
    Serial.printf("[mDNS] Secondary SSID: %s (mDNS: '%s')\n", current.secondarySSID.c_str(), current.secondaryMDNS.c_str());
    
    // Hangi ağa bağlandığımıza göre mDNS hostname belirle
    if (connectedSSID == current.primarySSID && current.primaryMDNS.length() > 0) {
        // Primary WiFi: Kullanıcı tanımlı hostname kullan
        mdnsHostname = current.primaryMDNS;
        // ".local" suffix'i varsa kaldır
        mdnsHostname.replace(".local", "");
        mdnsHostname.trim();
        Serial.printf("[mDNS] ✓ Primary WiFi hostname seçildi: %s\n", mdnsHostname.c_str());
    } else if (connectedSSID == current.secondarySSID && current.secondaryMDNS.length() > 0) {
        // Secondary WiFi: Kullanıcı tanımlı hostname kullan
        mdnsHostname = current.secondaryMDNS;
        // ".local" suffix'i varsa kaldır
        mdnsHostname.replace(".local", "");
        mdnsHostname.trim();
        Serial.printf("[mDNS] ✓ Secondary WiFi hostname seçildi: %s\n", mdnsHostname.c_str());
    } else {
        // Emergency open network veya tanımsız: Chip ID kullan
        Serial.printf("[mDNS] ℹ Emergency/Unknown network, chip ID hostname kullanılıyor: %s\n", mdnsHostname.c_str());
    }
    
    // Boş hostname kontrolü
    if (mdnsHostname.length() == 0) {
        mdnsHostname = "smartkraft-dmf-" + chipIdStr;
        Serial.println(F("[mDNS] Boş hostname, chip ID kullanılıyor"));
    }
    
    // Önceki mDNS instance'ını durdur
    MDNS.end();
    delay(100); // mDNS için kısa bekleme
    
    // NOT: WiFi.setHostname() burada ÇAĞRILMAMALI!
    // Çünkü WiFi zaten connectTo() içinde başlatılmış
    // Hostname WiFi.begin() ÖNCE set edilmeli
    
    // mDNS'i başlat
    if (MDNS.begin(mdnsHostname.c_str())) {
        MDNS.addService("http", "tcp", 80);
        MDNS.addServiceTxt("http", "tcp", "version", FIRMWARE_VERSION);
        MDNS.addServiceTxt("http", "tcp", "model", "SmartKraft-DMF");
        MDNS.addServiceTxt("http", "tcp", "mode", "station");
        
        Serial.printf("[mDNS] ✓ Başlatıldı: %s.local (HTTP service published)\n", mdnsHostname.c_str());
        Serial.printf("[mDNS] ✓ Mobil tarayıcıda deneyin: http://%s.local\n", mdnsHostname.c_str());
    } else {
        Serial.println(F("[mDNS] ✗ Başlatılamadı"));
    }
}

// ============================================
// mDNS Yenileme (ayarlar değiştiğinde çağrılır)
// ============================================
void DMFNetworkManager::refreshMDNS() {
    String currentSSID = WiFi.SSID();
    if (currentSSID.length() > 0 && WiFi.status() == WL_CONNECTED) {
        Serial.printf("[mDNS] Yenileme istendi, mevcut SSID: %s\n", currentSSID.c_str());
        startMDNS(currentSSID);
    } else {
        Serial.println(F("[mDNS] WiFi bağlı değil, mDNS başlatılamadı"));
    }
}


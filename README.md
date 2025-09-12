# DMF SmartKraft - Arduino IDE Kurulum Rehberi

## Gerekli Kütüphaneler

Arduino IDE'de aşağıdaki kütüphaneleri yüklemeniz gerekiyor:

### ESP32 Board Package
1. Arduino IDE > File > Preferences
2. Additional Board Manager URLs'e ekleyin: 
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Tools > Board > Boards Manager > ESP32 arayın ve yükleyin

### Gerekli Kütüphaneler (Library Manager'dan yükleyin)
- ArduinoJson (by Benoit Blanchon) - Version 6.21.0 veya üzeri
- ESP32 Core Libraries (otomatik gelir):
  - WiFi
  - WebServer
  - ESPmDNS
  - SPIFFS
  - WiFiClientSecure
  - HTTPClient
  - Preferences

## Board Ayarları

Arduino IDE'de Tools menüsünden:
- Board: "ESP32S3 Dev Module"
- Upload Speed: "921600"
- CPU Frequency: "240MHz (WiFi/BT)"
- Flash Mode: "QIO"
- Flash Frequency: "80MHz"
- Flash Size: "4MB (32Mb)"
- Partition Scheme: "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
- Core Debug Level: "None"
- PSRAM: "Disabled" (ESP32-S3 için gerekirse "OPI PSRAM")

## Pin Bağlantıları

```
ESP32-S3 Pin Layout:
- GPIO2  -> Röle Kontrol (RELAY_PIN)
- GPIO0  -> Buton (BUTTON_PIN) - Boot button kullanılabilir
- GPIO8  -> Status LED (STATUS_LED_PIN)
- GPIO9  -> Setup LED (SETUP_LED_PIN)
- 3.3V   -> VCC bağlantıları
- GND    -> GND bağlantıları
```

## İlk Kurulum

1. ESP32-S3'ü bilgisayara bağlayın
2. Arduino IDE'de doğru portu seçin
3. DMF.ino dosyasını yükleyin
4. Upload butonuna tıklayın

### İlk Çalıştırma - Adım Adım
**1. Cihazı Açın**
- ESP32-S3 açıldığında Serial Monitor'da şu mesajları göreceksiniz:
```
=== DMF SmartKraft Starting ===
First boot or no WiFi configured, starting setup mode
=== SETUP MODE ACTIVE ===
AP Name: DMF-SmartKraft-SETUP-XXXXXX
AP IP: 192.168.4.1
Connect to this WiFi and go to: http://192.168.4.1
=========================
```

**2. WiFi Ağına Bağlanın**
- Telefon/Bilgisayar WiFi ayarlarından "DMF-SmartKraft-SETUP-XXXXXX" ağını bulun
- Bu ağa bağlanın (şifre yok)
- AP Name'deki XXXXXX kısmı cihazın MAC adresinin son 6 hanesidir

**3. Kurulum Sayfasını Açın**
- Web tarayıcıda `http://192.168.4.1` adresine gidin
- DMF kontrol paneli açılacak
- Üstte sarı renkli "🛠️ SETUP MODE" uyarısı görünecek

**4. WiFi Ayarlarını Yapın**
- "Primary WiFi SSID" alanına ev/ofis WiFi'nizin adını girin
- "Primary WiFi Password" alanına şifresini girin
- "Allow open networks as backup" seçeneğini isteğe bağlı işaretleyin
- "Save Settings" butonuna tıklayın

**5. Kurulum Tamamlanması**
- "Setup completed! Device will restart..." mesajı çıkacak
- Cihaz otomatik yeniden başlayacak
- Yapılandırdığınız WiFi ağına bağlanacak

**6. Normal Kullanıma Geçiş**
- Cihaz WiFi'ye bağlandıktan sonra `http://dmf-smartkraft.local` adresinden erişilebilir
- Eğer mDNS çalışmıyorsa, cihazın IP adresini kullanın
- Artık normal kullanıma hazır!

## Güvenlik Notları

- İlk kurulumu güvenli bir ortamda yapın
- ProtonMail API anahtarınızı güvenli saklayın
- Factory Reset sadece content verilerini siler, sistem ayarlarını korur
- Cihaz AES256 donanım şifreleme kullanır

## Sorun Giderme

### Derleme Hataları
- ESP32 board package'ın en son sürümünü kullandığınızdan emin olun
- ArduinoJson kütüphanesinin uyumlu versiyonunu yükleyin

### Upload Hatası
- ESP32-S3'ü boot moduna almak için BOOT butonuna basılı tutup RESET'e basın
- Doğru COM portunu seçtiğinizden emin olun

### WiFi Bağlantı Sorunları
- Serial Monitor'da bağlantı loglarını kontrol edin
- Açık ağ tarama özelliği varsa test edin

## Teknik Destek

Proje GitHub deposu: https://github.com/smartkraft/dmf
E-posta: support@smartkraft.tech

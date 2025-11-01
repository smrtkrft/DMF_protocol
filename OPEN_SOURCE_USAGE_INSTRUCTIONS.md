# 🔧 Açık Kaynak Kullanım Talimatları (Geliştiriciler İçin)

## 📋 İçindekiler

- [Genel Bakış](#genel-bakış)
- [Gereksinimler](#gereksinimler)
- [Kurulum Adımları](#kurulum-adımları)
- [Kütüphaneler](#kütüphaneler)
- [Partition Scheme (OTA Yapılandırması)](#partition-scheme-ota-yapılandırması)
- [GPIO Pin Yapılandırması](#gpio-pin-yapılandırması)
- [Kritik Notlar ve Yaygın Hatalar](#kritik-notlar-ve-yaygın-hatalar)
- [İlk Derleme ve Yükleme](#ilk-derleme-ve-yükleme)
- [OTA Güncelleme Yapılandırması](#ota-güncelleme-yapılandırması)

---

## 🎯 Genel Bakış

Bu proje **ESP32-C6** için geliştirilmiş bir **DMF Protokolü** uygulamasıdır. OTA (Over-The-Air) güncellemesi, SPIFFS dosya sistemi ve çoklu dil desteği içerir.

**⚠️ ÖNEMLİ:** Bu kod yalnızca **ESP32-C6** için optimize edilmiştir. Diğer ESP32 varyantlarında çalışmaz veya sorun çıkarabilir.

---

## 📦 Gereksinimler

### Donanım
- **ESP32-C6** mikrodenetleyici (Seeed XIAO ESP32C6 önerilir)
- **4MB Flash bellek** (minimum)
- USB-C kablosu (programlama için)
- Opsiyonel: Buton, Röle modülü

### Yazılım
- **Arduino IDE** 2.x veya üzeri
- **ESP32 Board Package** v3.0.0 veya üzeri
- **Git** (kod çekmek için)

---

## 🚀 Kurulum Adımları

### 1. Arduino IDE Kurulumu

#### Board Manager Ayarı
Arduino IDE → Preferences → Additional Boards Manager URLs:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json
```

#### ESP32 Board Package Kurulumu
Tools → Board → Boards Manager → "ESP32" ara → **esp32 by Espressif Systems** → Install (v3.0.0+)

### 2. Kodu İndirme

```bash
git clone https://github.com/smrtkrft/DMF_protocol.git
cd DMF_protocol
```

### 3. Arduino IDE'de Açma

`SmartKraft_DMF/SmartKraft_DMF.ino` dosyasını Arduino IDE ile açın.

---

## 📚 Kütüphaneler

### Dahili Kütüphaneler (ESP32 Package ile gelir)
- `WiFi.h` - WiFi bağlantı yönetimi
- `WebServer.h` - Web sunucu işlemleri
- `SPIFFS.h` - Dosya sistemi
- `Preferences.h` - NVS (Non-Volatile Storage) için yapılandırma
- `Update.h` - OTA güncelleme
- `HTTPClient.h` - HTTP istekleri (OTA için)
- `esp_task_wdt.h` - Watchdog timer

### Harici Kütüphaneler (Manuel kurulum gerekli)

#### 1. ArduinoJson (v6.x)
```
Sketch → Include Library → Manage Libraries → "ArduinoJson" ara → Install v6.21.0 veya üzeri
```
**⚠️ DİKKAT:** v7.x KULLANMAYIN! Kod v6.x için yazılmıştır.

#### 2. ESP Mail Client (Mobizt)
```
Library Manager → "ESP Mail Client" ara → Mobizt tarafından geliştirilen versiyonu Install (v3.4.0+)
```

**Kütüphane Doğrulama:**
```cpp
// Bu satırlar başarılı compile oluyorsa kütüphaneler doğru yüklenmiş demektir
#include <ArduinoJson.h>
#include <ESP_Mail_Client.h>
```

---

## ⚙️ Partition Scheme (OTA Yapılandırması)

### ❌ YANLIŞ Partition Seçimi = Bootloop!

ESP32-C6'nın **4MB flash** olduğunu unutmayın. OTA için dual APP partition gerekir.

### ✅ DOĞRU Ayar (Kritik!)

Arduino IDE'de:
```
Tools → Board → ESP32 Arduino → XIAO_ESP32C6
Tools → Partition Scheme → "SmartKraft OTA (1.5MB APP/1MB SPIFFS)"
```

**Eğer "SmartKraft OTA" seçeneği görünmüyorsa:**

#### Manuel Partition Ekleme

1. **partitions.csv dosyasını kopyalayın:**

Windows:
```powershell
Copy-Item "partitions.csv" "$env:LOCALAPPDATA\Arduino15\packages\esp32\hardware\esp32\3.x.x\tools\partitions\smartkraft_ota.csv"
```

Linux/Mac:
```bash
cp partitions.csv ~/.arduino15/packages/esp32/hardware/esp32/3.x.x/tools/partitions/smartkraft_ota.csv
```

*(3.x.x yerine kurulu sürümünüzü yazın)*

2. **boards.txt dosyasını düzenleyin:**

Dosya yolu:
- Windows: `%LOCALAPPDATA%\Arduino15\packages\esp32\hardware\esp32\3.x.x\boards.txt`
- Linux/Mac: `~/.arduino15/packages/esp32/hardware/esp32/3.x.x/boards.txt`

**Eklenecek satırlar** (XIAO_ESP32C6 bölümüne):
```
XIAO_ESP32C6.menu.PartitionScheme.smartkraft=SmartKraft OTA (1.5MB APP/1MB SPIFFS)
XIAO_ESP32C6.menu.PartitionScheme.smartkraft.build.partitions=smartkraft_ota
XIAO_ESP32C6.menu.PartitionScheme.smartkraft.upload.maximum_size=1572864
```

3. **Arduino IDE'yi yeniden başlatın**

### Partition Yapısı

```
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x5000
otadata,  data, ota,     0xe000,  0x2000
app0,     app,  ota_0,   0x10000, 0x180000   # 1.5MB
app1,     app,  ota_1,   0x190000,0x180000   # 1.5MB (OTA için)
spiffs,   data, spiffs,  0x310000,0xF0000    # 960KB
```

**⚠️ Neden Önemli:**
- **app0 + app1:** OTA güncellemesi için dual boot
- **spiffs:** Web dosyaları ve yapılandırmalar
- **Toplam:** 4MB flash tam kullanımı
- **app1 olmadan:** OTA çalışmaz
- **spiffs olmadan:** Web arayüzü ve ayarlar kaybolur

---

## 📍 GPIO Pin Yapılandırması

### Kullanılan Pinler

```cpp
// SmartKraft_DMF.ino içinde tanımlı

#define BUTTON_PIN 21       // Fiziksel buton (isteğe bağlı)
#define RELAY_PIN  18       // Röle çıkışı (isteğe bağlı)
```

### Pin Detayları

| Pin | Fonksiyon | Tip | Açıklama |
|-----|-----------|-----|----------|
| **GPIO21** | Buton Girişi | INPUT_PULLUP | Fiziksel erteleme butonu (opsiyonel) |
| **GPIO18** | Röle Çıkışı | OUTPUT | Röle kontrolü (opsiyonel, Max 5V 30mA) |

### ⚠️ GPIO Kritik Notlar

1. **GPIO21 (Buton):**
   - `INPUT_PULLUP` modunda
   - Buton basıldığında GND'ye çeker
   - Harici pull-up direncine gerek yok
   - Fiziksel buton kullanmıyorsanız pin boş bırakılabilir (sanal buton kullanılır)

2. **GPIO18 (Röle):**
   - **MAKSİMUM 5V 30mA** - Daha fazlası ESP32-C6'ya zarar verir!
   - Yüksek akım röle için **transistör sürücü** kullanın (BC547, 2N2222 vb.)
   - Röle kullanmıyorsanız pin boş bırakılabilir

3. **Kullanılmaması Gereken Pinler:**
   - GPIO0, GPIO8, GPIO9: Boot ve JTAG pinleri
   - Conflict yaratabilir, kullanmayın

### Pin Değiştirme

Farklı pin kullanmak isterseniz `SmartKraft_DMF.ino` içinde:

```cpp
#define BUTTON_PIN 21  // İstediğiniz pin numarası
#define RELAY_PIN  18  // İstediğiniz pin numarası
```

**Sonra:**
```cpp
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
}
```

---

## 🚨 Kritik Notlar ve Yaygın Hatalar

### ❌ SORUN 1: Bootloop - Sürekli Reset

**Neden:**
- Yanlış partition scheme seçilmiş
- 2MB APP partition seçilmiş (4MB flash'a sığmaz)

**Çözüm:**
- "SmartKraft OTA (1.5MB APP/1MB SPIFFS)" seçin
- Partition scheme'i manuel ekleyin (yukarıdaki talimatlar)

### ❌ SORUN 2: "Sketch too large" Hatası

**Neden:**
- Kod boyutu 1.5MB'ı aşmış
- Debug çıktıları çok fazla

**Çözüm:**
```
Tools → Core Debug Level → "None"
Tools → Optimize → "Optimize for size (-Os)"
```

### ❌ SORUN 3: SPIFFS Mount Failed

**Neden:**
- İlk yüklemede SPIFFS formatlanmamış
- Partition scheme SPIFFS içermiyor

**Çözüm:**
```cpp
// İlk yüklemede SPIFFS.format() çağrılır (kod içinde mevcut)
// Manuel format:
SPIFFS.format();
```

### ❌ SORUN 4: OTA Güncelleme Çalışmıyor

**Neden:**
- app1 partition yok
- GitHub URL yanlış
- WiFi bağlantısı yok

**Çözüm:**
- Dual partition scheme kullanın
- `network_manager.cpp` içinde GitHub URL'i kontrol edin
- WiFi bağlantısını doğrulayın

### ❌ SORUN 5: Mail Gönderilmiyor

**Neden:**
- ESP_Mail_Client kütüphanesi eksik/yanlış versiyon
- SMTP ayarları yanlış
- Watchdog timeout (uzun süren mail gönderimi)

**Çözüm:**
- Mobizt ESP_Mail_Client v3.4.0+ kullanın
- SMTP ayarlarını test edin
- Watchdog timeout süresini artırın (kod içinde mevcut)

### ❌ SORUN 6: Web Arayüzü Açılmıyor

**Neden:**
- SPIFFS dosyaları yok
- WiFi bağlantısı yok
- Yanlış IP adresi

**Çözüm:**
- Serial Monitor'den IP adresini kontrol edin
- SPIFFS'in doğru mount edildiğini kontrol edin
- AP modunda `192.168.4.1` kullanın

---

## 🔨 İlk Derleme ve Yükleme

### Adım 1: Board Ayarları

```
Tools → Board → ESP32 Arduino → XIAO_ESP32C6
Tools → Partition Scheme → SmartKraft OTA (1.5MB APP/1MB SPIFFS)
Tools → Flash Size → 4MB
Tools → Flash Mode → QIO
Tools → Flash Frequency → 80MHz
Tools → Upload Speed → 921600
Tools → Core Debug Level → None (veya Error)
```

### Adım 2: Port Seçimi

```
Tools → Port → (ESP32-C6'nızın bağlı olduğu COM portu)
```

### Adım 3: Derleme (Verify)

```
Sketch → Verify/Compile
```

**Başarılı derleme çıktısı:**
```
Sketch uses XXXXX bytes (XX%) of program storage space.
Global variables use XXXXX bytes (XX%) of dynamic memory.
```

**⚠️ Program storage %90'ı geçerse:**
- Gereksiz debug kodlarını kaldırın
- Core Debug Level → None
- Optimize → Size

### Adım 4: Yükleme (Upload)

```
Sketch → Upload
```

**İlk yükleme süresi:** ~30-60 saniye

### Adım 5: Serial Monitor Kontrolü

```
Tools → Serial Monitor → 115200 baud
```

**Başarılı boot çıktısı:**
```
SmartKraft DMF v1.0.0
[WiFi] AP modu aktif: SmartKraft-DMF
[SPIFFS] Dosya sistemi başlatıldı
[Web] Sunucu başlatıldı: 192.168.4.1
[OTA] Otomatik güncelleme aktif
[READY] Sistem hazır
```

---

## 🔄 OTA Güncelleme Yapılandırması

### GitHub Repository Ayarı

`network_manager.cpp` dosyasında:

```cpp
const char* GITHUB_REPO_OWNER = "smrtkrft";           // GitHub kullanıcı adınız
const char* GITHUB_REPO_NAME = "DMF_protocol";        // Repository adınız
const char* GITHUB_FIRMWARE_FILE = "firmware.bin";    // Release asset adı
```

### Firmware Binary Oluşturma

Arduino IDE ile compile ettikten sonra:

**Windows:**
```
%TEMP%\arduino\sketches\[sketch-folder]\SmartKraft_DMF.ino.bin
```

**Linux/Mac:**
```
/tmp/arduino/sketches/[sketch-folder]/SmartKraft_DMF.ino.bin
```

Bu dosyayı GitHub Release'e `firmware.bin` adıyla yükleyin.

### GitHub Release Oluşturma

1. GitHub repository → Releases → Create a new release
2. Tag: `v1.0.1` (versiyon numarası)
3. Title: "SmartKraft DMF v1.0.1"
4. Upload: `firmware.bin` dosyası
5. Publish release

### OTA Kontrolü

Cihaz her 24 saatte bir otomatik kontrol eder. Manuel kontrol için web arayüzünden "OTA Güncelleme Kontrol" butonuna basın.

### OTA Güncelleme Süreci

```
1. GitHub API'den son release kontrol edilir
2. Versiyon numarası karşılaştırılır
3. Yeni versiyon varsa firmware.bin indirilir
4. app1 partition'a yazılır
5. boot partition değiştirilir
6. Cihaz yeniden başlatılır
7. Yeni firmware app1'den boot olur
```

**⚠️ OTA Sırasında:**
- Cihazın gücünü kesmeyin
- WiFi bağlantısını kesmeyin
- İşlem ~1-2 dakika sürer

---

## 🛠️ Geliştirme İpuçları

### Debug Modu

Geliştirme sırasında:
```
Tools → Core Debug Level → Debug
```

Production için:
```
Tools → Core Debug Level → None
```

### Serial Monitor Komutları

Kod içinde serial komutlar mevcut:
```
status     - Sistem durumunu göster
reset      - Ayarları sıfırla
restart    - Cihazı yeniden başlat
format     - SPIFFS'i formatla (dikkatli!)
```

### Bellek Optimizasyonu

```cpp
// String yerine F() macro kullanın
Serial.println(F("Sabit metin"));  // ✅ SRAM'den tasarruf
Serial.println("Sabit metin");      // ❌ SRAM kullanır

// PROGMEM kullanın
const char text[] PROGMEM = "Uzun metin...";
```

### Watchdog Timer

Mail gönderimi gibi uzun işlemler için:
```cpp
esp_task_wdt_reset();  // Watchdog'u resetle
```

---

## 📞 Destek ve Katkı

- **Issues:** https://github.com/smrtkrft/DMF_protocol/issues
- **Pull Requests:** Hoş geldiniz!
- **License:** AGPL-3.0 (fork ve değişiklikleri paylaşmalısınız)

---

## ✅ Hızlı Kontrol Listesi

Yüklemeden önce kontrol edin:

- [ ] ESP32 Board Package v3.0.0+ kurulu
- [ ] ArduinoJson v6.x kurulu (v7 DEĞİL!)
- [ ] ESP_Mail_Client (Mobizt) kurulu
- [ ] Board: XIAO_ESP32C6 seçili
- [ ] Partition: SmartKraft OTA seçili
- [ ] Flash Size: 4MB
- [ ] Port doğru seçildi
- [ ] Kütüphaneler compile oluyor
- [ ] GPIO pinleri donanıma uygun

---

**© 2025 SmartKraft | AGPL-3.0 License**

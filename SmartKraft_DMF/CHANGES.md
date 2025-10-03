# SmartKraft DMF - Değişiklik Özeti

## 📅 2025-10-04 - Son Düzeltmeler

### ✅ Düzeltme #1: Timer Ayarları Değiştirildiğinde Yanlış Alarm Tetiklenmesi

**Sorun:**
- Timer ayarları (süre/alarm sayısı) değiştirildiğinde otomatik olarak alarm tetikleniyordu
- GET URL'si zamansız çağrılıyordu
- Mail erken gönderiliyordu

**Kök Sebep:**
`scheduler.cpp` → `configure()` fonksiyonu alarm schedule'ı yeniden hesaplarken mevcut alarm index'ini sıfırlıyordu.

**Çözüm:**
- Timer aktifken ayar değiştirilirse `nextAlarmIndex` korunuyor
- Deadline yeniden hesaplanıyor ama elapsed time preserve ediliyor
- Dosya: `scheduler.cpp`

**Test:**
```
✅ Timer çalışırken süre değiştir → Alarm tetiklenmemeli
✅ Timer çalışırken alarm sayısı değiştir → Alarm tetiklenmemeli
```

---

### ✅ Düzeltme #2: Reset Sonrası WiFi Bağlantısı

**Sorun:**
- Cihaz reset atıldığında her zaman AP modunda başlıyordu
- Kaydedilmiş WiFi ayarları yok sayılıyordu

**Çözüm:**
- `startServer()`: Başlangıçta kayıtlı WiFi varsa otomatik bağlanıyor
- `connectTo()`: Dual mode desteği - AP modunu kapatmıyor
- Dosyalar: `web_handlers.cpp`, `network_manager.cpp`

**Davranış:**
1. Kayıtlı WiFi varsa → STA moduna bağlan
2. Bağlanamazsa → AP modunda kal
3. Her durumda AP erişilebilir (192.168.4.1)

**Test:**
```
✅ İlk açılış → AP modu
✅ WiFi kaydedip reset → Otomatik STA + AP (dual)
✅ WiFi erişilemiyor → AP moduna düşer (failsafe)
```

---

## ✅ Önceki İyileştirmeler

### 1. WiFi STA Modu Bağlantısı
**Sorun**: WiFi ayarları kaydedilince yerel ağa bağlanmıyordu.

**Çözüm**:
- `handleWiFiUpdate()` metoduna STA mode geçişi eklendi
- WiFi ayarları kaydedilince `WiFi.mode(WIFI_AP_STA)` ile dual mode aktif
- `ensureConnected()` çağrısı ile otomatik bağlantı
- Bağlantı başarılıysa Serial port'a IP adresi yazdırılıyor

### 2. Dosya Yükleme Debug ve Klasör Kontrolü
**Sorun**: Attachment upload sessizce başarısız oluyordu.

**Çözüm**:
- `handleAttachmentUpload()` metoduna detaylı Serial debug logları eklendi
- `/attachments` klasörü yoksa otomatik oluşturuluyor
- Dosya boyutu, yazma durumu ve save işlemi loglanıyor

### 3. Timer Buton Mantığı Düzeltmesi
**Sorun**: Başlat/Duraklat/Sıfırla butonları mantıksız davranıyordu.

**Çözüm**:
- **Yeni State Machine**:
  - `TimerRuntime` struct'ına `paused` flag eklendi
  - `pause()`, `resume()` metodları eklendi
  - `isStopped()`, `isPaused()`, `isActive()` helper metodları
  
- **Buton Davranışları**:
  - **Başlat**: Sadece durdurulmuş timer'ı başlatır (running/paused ise çalışmaz)
  - **Duraklat**: Sadece çalışan timer'ı duraklat (paused state'e al)
  - **Devam Et**: Sadece duraklatılmış timer'ı kaldığı yerden devam ettirir
  - **Sıfırla**: Her durumda timer'ı resetler (başlatmaz)
  - **Fiziksel Buton**: Reset + Start atomik işlem

- **UI Değişiklikleri**:
  - Butonlar duruma göre gösterilir/gizlenir
  - Durduruldu → sadece "Başlat" görünür
  - Çalışıyor → sadece "Duraklat" görünür
  - Duraklatıldı → sadece "Devam Et" görünür
  - "Sıfırla" ve "Fiziksel Buton" her zaman görünür

- **API Endpoints**:
  - `/api/timer/start` - Timer başlat (sadece stopped state'te)
  - `/api/timer/stop` - Timer duraklat (pause)
  - `/api/timer/resume` - Duraklatılmış timer'ı devam ettir
  - `/api/timer/reset` - Timer'ı sıfırla
  - `/api/timer/virtual-button` - Fiziksel buton simülasyonu

### 4. WiFi Uyarısı Mobil Görünüm Düzeltmesi
**Sorun**: Sağ üst köşedeki WiFi connection indicator mobilde içice girmiş.

**Çözüm**:
- CSS'e `background: #000` eklendi (saydam değil)
- `max-width: 280px` ve `text-overflow: ellipsis` ile uzun metinler kesilir
- Mobil görünümde (`@media max-width: 600px`):
  - `top/right/left: 8px` ile ekran genişliğinde gösterim
  - Daha küçük font (`0.7em`) ve padding

### 5. WiFi STA IP Gösterimi ve Captive Portal
**Sorun**: WiFi'ye bağlanınca IP adresini görmek zor, kurulum karmaşık.

**Çözüm**:
- **IP Gösterimi**:
  - `/api/status` endpoint'inde `ip` field'i mevcut
  - WiFi bağlantısı sağlandığında Serial port'a yazdırılıyor
  - Web UI'daki connection indicator'da görünüyor: `"Wi-Fi: SSID (192.168.1.123)"`

- **Captive Portal**:
  - `DNSServer` kütüphanesi eklendi
  - AP modunda tüm DNS sorguları ESP32'nin IP'sine yönlendiriliyor
  - Mobil cihazlar WiFi'ye bağlanınca otomatik browser açılır
  - Kullanıcı herhangi bir adrese gitmek istese SmartKraft web UI'sını görür

## 🔧 Değiştirilen Dosyalar

### Core Files
- ✅ `config_store.h` - TimerRuntime'a `paused` flag
- ✅ `config_store.cpp` - Runtime save/load'a `paused` desteği
- ✅ `scheduler.h` - pause/resume/isPaused/isStopped metodları
- ✅ `scheduler.cpp` - State machine implementasyonu
- ✅ `web_handlers.h` - DNSServer pointer, handleTimerResume
- ✅ `web_handlers.cpp` - Tüm API ve UI değişiklikleri
- ✅ `SmartKraft_DMF.ino` - DNSServer include ve init

### HTML/CSS/JS (Embedded)
- ✅ Buton görünürlük mantığı (`updateStatusView()`)
- ✅ `pauseTimer()`, `resumeTimer()` fonksiyonları
- ✅ Mobil responsive CSS düzeltmeleri
- ✅ Timer durumu: "Çalışıyor" / "Duraklatıldı" / "Durduruldu"

## 📝 Kullanım Senaryoları

### Senaryo 1: İlk Kurulum (Captive Portal)
1. ESP32'yi aç, "SmartKraft-DMF" AP'sine bağlan (şifre: `12345678`)
2. Telefon otomatik browser açar → Web UI görünür
3. WiFi sekmesine git, ev WiFi bilgilerini gir
4. Kaydet → ESP32 hem AP hem STA modunda çalışır
5. Serial port'tan yerel IP'yi öğren: `192.168.1.50`
6. Artık yerel ağdan `http://192.168.1.50` ile erişebilirsin

### Senaryo 2: Timer Kontrolü
1. **Timer Başlat**: Sayaç sıfırdan başlar, buton "Duraklat" olur
2. **Duraklat**: Sayaç durur, kalan süre korunur, buton "Devam Et" olur
3. **Devam Et**: Kaldığı yerden devam eder, buton "Duraklat" olur
4. **Sıfırla**: Her zaman timer'ı sıfırlar (başlatmaz)
5. **Fiziksel Buton**: Röleyi kapat, timer'ı sıfırla ve başlat

### Senaryo 3: Dosya Yükleme Debug
1. Mail sekmesinde dosya yükle
2. Serial Monitor'ü aç
3. Başarılı upload: `[Upload] Dosya alındı: report.pdf, boyut: 12345`
4. Başarısız upload: Hata kodu ve detayları Serial'de görünür

## 🐛 Bilinen Sınırlamalar
- Captive portal sadece AP modunda çalışır (STA-only modda değil)
- DNS server port 53 kullanır (bazı cihazlarda engelli olabilir)
- Timer pause/resume runtime'da korunur ama reset sonrası kaybolur

## 🚀 Sonraki Adımlar (İsteğe Bağlı)
- [ ] WebSocket ile real-time timer sync
- [ ] mDNS ile `smartkraft.local` hostname
- [ ] OTA firmware update desteği
- [ ] WiFi credential portal (AP modunda web üzerinden şifre değiştirme)

# Mobil mDNS Test Rehberi

## ❌ Neden .local Çalışmıyor?

### Android Cihazlar
- **Chrome/Edge**: mDNS desteği YOK (Android 12'den önce)
- **Firefox**: mDNS desteği YOK
- **Brave/Opera**: mDNS desteği YOK

**Çözüm**: 
1. IP adresini kullanın: `http://192.168.1.XXX`
2. Veya özel mDNS browser app kullanın: "Bonjour Browser" (Play Store)

### iOS Cihazlar (iPhone/iPad)
- **Safari**: mDNS tam destekli ✅
- **Chrome**: Safari engine kullanır, çalışır ✅
- **Firefox**: Safari engine kullanır, çalışır ✅

**ANCAK:**
- Mobil veri KAPALI olmalı
- WiFi bağlantısı AKTIF olmalı
- Cihaz ESP32 ile AYNI ağda olmalı

---

## ✅ Test Adımları

### 1. ESP32 Serial Monitor Kontrolü
ESP32 bağlandıktan sonra şu satırları arayın:
```
[mDNS] ✓ Başlatıldı: smartkraft-dmf-XXXX.local
[mDNS] ✓ Mobil tarayıcıda deneyin: http://smartkraft-dmf-XXXX.local
Wi-Fi: MyNetwork (192.168.1.100)
```

### 2. Mobil Cihaz Hazırlığı
- ✅ Mobil veriyi KAPATIN
- ✅ WiFi'ye bağlanın (ESP32 ile aynı ağ)
- ✅ WiFi'nin internet erişimi olduğunu kontrol edin

### 3. iOS Test (iPhone/iPad)
Safari'de deneyin:
```
http://smartkraft-dmf-XXXX.local
```
- Çalışmazsa: Cihazı yeniden başlatın, WiFi'yi resetleyin
- Hala çalışmazsa: IP adresini deneyin

### 4. Android Test
**mDNS Browser App kullanın:**
1. Play Store'dan "Bonjour Browser" uygulamasını indirin
2. Uygulamayı açın
3. `_http._tcp` servisini arayın
4. `smartkraft-dmf-XXXX` göreceksiniz
5. IP adresine dokunun → tarayıcıda açın

**Tarayıcı alternatifi:**
- IP adresini doğrudan kullanın: `http://192.168.1.100`
- Router admin panelinden cihaz ismini görün

---

## 🔧 Alternatif Çözümler

### Çözüm 1: Router DNS
Bazı router'lar DHCP hostname'lerini otomatik DNS'e ekler.
Router admin panelinde cihaz listesine bakın:
- `smartkraft-dmf-7ffe` gibi bir isim görüyorsanız
- Bunu direkt kullanabilirsiniz: `http://smartkraft-dmf-7ffe`

### Çözüm 2: Statik IP + DNS
Router'da statik IP ayarlayın:
- MAC adresini ESP32'den alın (Serial Monitor)
- Router DHCP ayarlarında rezervasyon yapın
- İsteğe bağlı: Custom DNS hostname ekleyin

### Çözüm 3: QR Code
Web arayüzüne QR kod ekleyin:
- ESP32 IP'sini QR koda dönüştürün
- Mobil cihazdan QR kod okutun
- Otomatik olarak tarayıcıda açılsın

---

## 📱 Test Sonuçları

### iOS Safari
- [ ] `http://smartkraft-dmf-XXXX.local` çalıştı
- [ ] IP adresi ile çalıştı: `http://___.___.___.___ `
- [ ] Sorun:

### Android Chrome
- [ ] `http://smartkraft-dmf-XXXX.local` çalıştı
- [ ] IP adresi ile çalıştı: `http://___.___.___.___ `
- [ ] Bonjour Browser ile bulundu
- [ ] Sorun:

---

## 🐛 Debug Bilgileri

Serial Monitor çıktısını buraya yapıştırın:
```
[WiFi] Bağlantı başarılı: 192.168.1.XXX
[mDNS] ✓ Başlatıldı: smartkraft-dmf-XXXX.local
```

Mobil cihaz bilgileri:
- **İşletim Sistemi**: Android 14 / iOS 17
- **Tarayıcı**: Chrome 120 / Safari 17
- **WiFi Ağı**: MyHomeWiFi
- **ESP32 IP**: 192.168.1.XXX

# OTA Güncelleme Sistemi Kurulum Rehberi

## Sistem Özeti
SmartKraft DMF sistemi artık GitHub üzerinden otomatik OTA (Over-The-Air) güncellemelerini destekliyor.

### Nasıl Çalışır?
- Sistem her **24 saatte bir** otomatik olarak güncellemeleri kontrol eder
- GitHub'dan versiyon bilgisini okur
- Yeni versiyon varsa otomatik olarak indirir ve yükler
- Güvenlik kontrolleri:
  - Final mail gönderilmişse güncelleme YAPILMAZ
  - Timer aktifse ve 1 saatten az kalmışsa güncelleme YAPILMAZ
  - WiFi bağlı değilse güncelleme YAPILMAZ

## GitHub Releases Yapısı

### 1. Repository Yapısı
```
DMF_protocol/
├── releases/
│   └── version.txt          # En son versiyon numarası (örn: 1.0.1)
└── SmartKraft_DMF/
    └── SmartKraft_DMF.ino   # Ana kod
```

### 2. version.txt Dosyası
`releases/version.txt` dosyasını oluşturun ve içine sadece versiyon numarasını yazın:

```
1.0.1
```

**Önemli:** Dosyada sadece versiyon numarası olmalı, başka bir şey olmamalı (boşluk, yeni satır vb. yok).

### 3. GitHub Release Oluşturma

#### Adım 1: Arduino IDE'de Firmware Derle
1. Arduino IDE'yi açın
2. `SmartKraft_DMF.ino` dosyasını açın
3. **FIRMWARE_VERSION** değerini güncelleyin:
   ```cpp
   #define FIRMWARE_VERSION "1.0.1"  // Yeni versiyon
   ```
4. **Sketch → Export Compiled Binary** (veya Ctrl+Alt+S)
5. Derleme tamamlandığında build klasörünü bulun
6. `SmartKraft_DMF.ino.bin` dosyasını bulun

#### Adım 2: GitHub Release Oluştur
1. GitHub repository sayfasında **Releases** → **Draft a new release**
2. **Tag**: `v1.0.1` (versiyon numarası)
3. **Release title**: `SmartKraft DMF v1.0.1`
4. **Description**: Değişiklikleri yazın
5. **Attach binaries**: `SmartKraft_DMF.ino.bin` dosyasını ekleyin
6. **Publish release**

#### Adım 3: version.txt Güncelle
1. `releases/version.txt` dosyasını düzenleyin
2. İçeriği yeni versiyona güncelleyin: `1.0.1`
3. Commit ve push yapın

## OTA URL'leri

### Versiyon Kontrolü
```
https://raw.githubusercontent.com/smrtkrft/DMF_protocol/main/releases/version.txt
```

### Firmware İndirme
```
https://github.com/smrtkrft/DMF_protocol/releases/latest/download/SmartKraft_DMF.ino.bin
```

## Test Etme

### Manuel Test
SmartKraft_DMF.ino dosyasında:
```cpp
// Test için interval'i kısaltın (örn: 5 dakika)
if (millis() - lastOTACheck > 300000) { // 5 dakika = 300000 ms
```

### Serial Monitor Çıktısı
```
[OTA] Sistem aktif - otomatik güncellemeler etkin
[OTA] Versiyon kontrolü: https://raw.githubusercontent.com/...
[OTA] Mevcut versiyon: 1.0.0, En son versiyon: 1.0.1
[OTA] Yeni versiyon mevcut!
[OTA] Firmware indiriliyor: https://github.com/...
[OTA] Firmware boyutu: 789456 bytes
[OTA] Güncelleme başlatılıyor...
[OTA] Yazma tamamlandı
[OTA] Güncelleme başarılı! Yeniden başlatılıyor...
```

## Versiyon Numaralandırma Kuralları

### Semantic Versioning (X.Y.Z)
- **X (Major)**: Büyük değişiklikler, API breaking changes
- **Y (Minor)**: Yeni özellikler, geriye uyumlu
- **Z (Patch)**: Bug fix'ler, küçük iyileştirmeler

Örnekler:
- `1.0.0` → İlk stable release
- `1.0.1` → Bug fix
- `1.1.0` → Yeni özellik (örn: yeni dil desteği)
- `2.0.0` → Büyük değişiklik (örn: API değişikliği)

## Güvenlik Notları

### HTTPS Kullanımı
- Tüm OTA bağlantıları HTTPS üzerinden yapılır
- GitHub'ın SSL sertifikası otomatik doğrulanır

### Rollback Mekanizması
- Güncelleme başarısız olursa eski firmware korunur
- OTA hatası durumunda sistem yeniden başlamaz

### Kritik Durum Koruması
- Timer son 1 saatteyse güncelleme YAPILMAZ
- Final mail gönderildiyse güncelleme YAPILMAZ
- WiFi bağlantısı yoksa güncelleme YAPILMAZ

## Partition Şeması

OTA için yeterli alan olması gerekir:

### Önerilen Arduino IDE Ayarları
**Tools → Partition Scheme:**
```
Default 4MB with ffat (1.2MB APP/1.5MB FATFS)
```

Bu şema şunları sağlar:
- **1.2MB** uygulama için (OTA için yeterli)
- **1.5MB** LittleFS için (config dosyaları)

## Sorun Giderme

### "Yeterli alan yok" Hatası
- Partition scheme'i kontrol edin
- "Default 4MB with ffat" seçeneğini kullanın

### "HTTP hatası: 404"
- GitHub release'in yayınlandığından emin olun
- `.bin` dosyasının assets'e eklendiğini kontrol edin

### "Güncelleme tamamlanamadı"
- İnternet bağlantısını kontrol edin
- Serial monitor'de hata mesajlarını okuyun
- GitHub URL'lerini tarayıcıda test edin

### Güncelleme Hiç Çalışmıyor
- 24 saat bekleyin veya interval'i test için kısaltın
- WiFi bağlantısını kontrol edin
- version.txt'nin doğru formatda olduğundan emin olun

## Geliştirme Notları

### OTA Interval Değiştirme
`SmartKraft_DMF.ino` dosyasında:
```cpp
// 24 saat = 86400000 ms
if (millis() - lastOTACheck > 86400000) {
    // ...
}
```

### OTA'yı Devre Dışı Bırakma
`SmartKraft_DMF.ino` dosyasında OTA kontrolünü yorum satırına alın:
```cpp
// if (millis() - lastOTACheck > 86400000) {
//     // OTA kodu...
// }
```

## İlk Release Checklist

- [ ] `releases/` klasörü oluşturuldu
- [ ] `releases/version.txt` dosyası oluşturuldu (içerik: `1.0.0`)
- [ ] Arduino IDE'de binary export yapıldı
- [ ] GitHub'da v1.0.0 release'i oluşturuldu
- [ ] `SmartKraft_DMF.ino.bin` dosyası release'e eklendi
- [ ] URL'ler tarayıcıda test edildi
- [ ] Serial monitor'de OTA mesajları görüldü

## Sonraki Güncellemeler İçin Checklist

- [ ] `SmartKraft_DMF.ino` içinde `FIRMWARE_VERSION` güncellendi
- [ ] Kod değişiklikleri tamamlandı ve test edildi
- [ ] Arduino IDE'de binary export yapıldı
- [ ] GitHub'da yeni release oluşturuldu (örn: v1.0.1)
- [ ] `.bin` dosyası release'e eklendi
- [ ] `releases/version.txt` dosyası güncellendi (örn: `1.0.1`)
- [ ] Commit ve push yapıldı
- [ ] Cihazda 24 saat içinde otomatik güncelleme kontrol edildi

---

**Not:** Bu sistem tamamen otomatik çalışır. Kullanıcının herhangi bir işlem yapmasına gerek yoktur. Sistem her 24 saatte bir otomatik olarak güncellemeleri kontrol eder ve güvenli olduğunda yükler.

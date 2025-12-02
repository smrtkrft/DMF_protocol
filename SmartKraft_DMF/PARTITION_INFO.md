# ESP32-C6 Partition Seçimi

## Sorun
Binary boyutu (~1.51MB) standart partition'a (1.25MB APP) sığmıyor.

## Çözüm: Arduino IDE'de Partition Değiştir

**Araçlar → Partition Scheme** menüsünden şunlardan birini seçin:

### Önerilen: "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)"
- ✅ **1.9MB APP** - Kodunuz rahat sığar
- ✅ **OTA Korunur** - GitHub'dan otomatik güncelleme çalışır
- ✅ **190KB SPIFFS** - LittleFS için yeterli (ayarlar, dosyalar)

### Alternatif: "No OTA (2MB APP/2MB SPIFFS)"
- ✅ **2MB APP** - En geniş alan
- ❌ **OTA YOK** - Manuel güncelleme gerekir
- ✅ **2MB SPIFFS** - Büyük dosya desteği

## Binary Boyutu Kontrolü
Derleme sonrası şunu görmelisiniz:
```
Sketch uses XXXXXX bytes (XX%) of program storage space. Maximum is 1966080 bytes.
```

## Not
XIAO ESP32-C6 **4MB flash** içerir. Doğru partition ile hem OTA hem büyük kod desteklenir.

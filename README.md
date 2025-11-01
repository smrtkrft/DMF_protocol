# 🚨 SmartKraft DMF Protocol

**Dead Man's Fall (DMF) Emergency Message Delivery System**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-ESP32--C6-blue.svg)](https://www.espressif.com/en/products/socs/esp32-c6)
[![Version](https://img.shields.io/github/v/release/smrtkrft/DMF_protocol)](https://github.com/smrtkrft/DMF_protocol/releases)

## 📖 Overview

SmartKraft DMF (Dead Man's Fall) Protocol, acil durumlarda otomatik mesaj gönderme sistemidir. Kullanıcı belirlenen süre içinde sistemi sıfırlamazsa, önceden tanımlanmış kişilere otomatik olarak e-posta gönderir.

### 🎯 Use Cases
- 🏔️ **Dağcılık ve trekking**: Yüksek riskli aktivitelerde güvenlik
- 🚴 **Solo seyahatler**: Tek başına seyahat edenlerin güvenliği
- 👴 **Yaşlı bakımı**: Düzenli kontrol gerektiren durumlar
- 🏥 **Tıbbi durum izleme**: Kritik sağlık durumları için alarm
- 🔐 **Güvenlik sistemleri**: Otomatik alarm tetikleme

## ✨ Features

### Core Features
- ⏱️ **Configurable Timer**: 1 dakika - 30 gün arası ayarlanabilir geri sayım
- 📧 **Multi-Recipient Email**: 3 farklı gruba otomatik e-posta gönderimi
- 🔘 **Physical Button**: Fiziksel buton ile manuel sıfırlama
- 🌐 **Web Interface**: Detaylı web tabanlı kontrol paneli
- 📱 **Virtual Button API**: Mobil uygulamalar için REST API

### Advanced Features
- 🔄 **Automatic OTA Updates**: GitHub üzerinden otomatik firmware güncellemeleri (24 saatte bir kontrol)
- 🌍 **Multi-language Support**: Türkçe, English, Deutsch
- 📡 **Dual WiFi Support**: Primary ve secondary ağ desteği
- 🔌 **Persistence**: Flash bellek ile güç kesintisinde veri korunması
- 🔁 **Auto-Reboot**: 12 saatlik otomatik yeniden başlatma
- 🧪 **Test Mode**: Sistem testi için özel mod

## 🛠️ Hardware Requirements

### Required Components
- **ESP32-C6 XIAO** (Seeed Studio)
- **Physical Button** (GPIO21 - pull-up configured)
- **Relay Module** (GPIO18 - optional, for external triggers)

### Specifications
- **MCU**: ESP32-C6 (RISC-V, 160 MHz)
- **Flash**: 4MB (1.2MB APP / 1.5MB LittleFS)
- **RAM**: 512KB SRAM
- **WiFi**: 2.4 GHz 802.11 b/g/n
- **Power**: 5V USB-C or 3.3V direct

## 📥 Installation

### 1. Arduino IDE Setup

**Board Manager URL:**
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json
```

**Install:**
- Tools → Board → Boards Manager → Search "ESP32" → Install "esp32 by Espressif Systems"

**Select Board:**
- Tools → Board → ESP32 Arduino → XIAO_ESP32C6

**Partition Scheme:**
- Tools → Partition Scheme → **Default 4MB with ffat (1.2MB APP/1.5MB FATFS)**

### 2. Required Libraries

Install via Arduino IDE Library Manager:
```
- WiFi (built-in)
- WebServer (built-in)
- LittleFS (built-in)
- ArduinoJson (by Benoit Blanchon) - v6.x
- ESP_Mail_Client (by Mobizt)
- HTTPClient (built-in)
- Update (built-in)
```

### 3. Flash Firmware

**First Time (USB):**
1. Download latest release: [Releases](https://github.com/smrtkrft/DMF_protocol/releases/latest)
2. Open `SmartKraft_DMF.ino` in Arduino IDE
3. Select board and port
4. Click Upload

**OTA Updates:**
- Automatic! System checks GitHub every 24 hours for new versions

## 🚀 Quick Start

### 1. First Boot - WiFi Setup

Device creates AP: **SmartKraft-DMF**

Connect and navigate to: `http://192.168.4.1`

Configure:
- Primary WiFi credentials
- Secondary WiFi (optional)
- Static IP settings (optional)

### 2. Timer Configuration

Navigate to: `http://[device-ip]/timer`

Settings:
- **Duration**: 1 dakika - 30 gün
- **Auto-start**: Açılışta otomatik başlat
- **Auto-restart**: Timer bitiminde otomatik yeniden başlat

### 3. Email Configuration

Navigate to: `http://[device-ip]/mail`

Configure SMTP:
```
Server: smtp.gmail.com (or your SMTP server)
Port: 587 (TLS) or 465 (SSL)
Username: your-email@gmail.com
Password: app-specific password
```

**Recipients:** 3 groups, each can have multiple emails

### 4. Virtual Button Setup

Navigate to: `http://[device-ip]/api`

Generate API key → Use in mobile app/external system

**API Endpoint:**
```
POST http://[device-ip]/api/reset
Authorization: Bearer YOUR_API_KEY
```

## 📡 Web Interface

### Pages

| URL | Description |
|-----|-------------|
| `/` | Ana sayfa - sistem durumu |
| `/timer` | Timer yapılandırması |
| `/mail` | E-posta ayarları ve alıcılar |
| `/wifi` | WiFi ağ ayarları |
| `/api` | Sanal buton API yönetimi |
| `/test` | Test modları ve diagnostics |
| `/lang?l=tr` | Dil değiştirme (tr/en/de) |

## 🔌 REST API

### Reset Timer
```http
POST /api/reset
Authorization: Bearer YOUR_API_KEY

Response: {"success": true, "message": "Timer reset"}
```

### Get Status
```http
GET /api/status
Authorization: Bearer YOUR_API_KEY

Response: {
  "active": true,
  "remaining": 3600,
  "finalSent": false
}
```

## 🔄 OTA Updates

System automatically checks for updates every 24 hours.

**Update URLs:**
- Version check: `https://raw.githubusercontent.com/smrtkrft/DMF_protocol/main/releases/version.txt`
- Binary download: `https://github.com/smrtkrft/DMF_protocol/releases/latest/download/SmartKraft_DMF.ino.bin`

**Safety:**
- No update if timer has <1 hour remaining
- No update if final email already sent
- No update if WiFi disconnected
- Automatic rollback on failure

See [OTA_SETUP.md](OTA_SETUP.md) for developer instructions.

## 🔧 Configuration Files

System stores configuration in LittleFS (Flash):

```
/timer.json      - Timer settings
/mail.json       - SMTP & recipients
/wifi.json       - WiFi credentials
/runtime.json    - Current timer state
/api.json        - Virtual button API keys
```

**Persistence:** State saved every 60 seconds → Max 60s loss on power failure

## 🧪 Testing

### Test Mode
Navigate to: `http://[device-ip]/test`

Features:
- Manual timer start/stop
- Email test (send test email)
- WiFi reconnection test
- System diagnostics

### Serial Monitor
```
Baud: 115200

Output:
[WiFi] Bağlantı başarılı: 192.168.1.100
[Timer] Başlatıldı: 3600 saniye
[OTA] Versiyon kontrolü: 1.0.0
```

## 📊 Technical Details

### Memory Usage
- **Program**: ~750KB / 1200KB
- **Dynamic RAM**: ~150KB / 512KB
- **Flash Storage**: ~50KB / 1500KB

### Power Consumption
- **Active (WiFi on)**: ~150mA @ 3.3V
- **Idle**: ~80mA @ 3.3V
- **Deep sleep**: Not implemented (always active for reliability)

### Timing Accuracy
- **Timer**: ±1 second per hour
- **Persistence**: 60-second intervals
- **OTA Check**: 24-hour intervals

## 🛡️ Security

### Implemented
✅ WPA2/WPA3 WiFi encryption  
✅ HTTPS for OTA updates  
✅ Bearer token authentication for API  
✅ Password-protected SMTP  

### Recommendations
⚠️ Use app-specific passwords for Gmail  
⚠️ Keep API keys secret  
⚠️ Use static IP or DHCP reservation  
⚠️ Regular firmware updates  

## 📝 License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📧 Support

- **Issues**: [GitHub Issues](https://github.com/smrtkrft/DMF_protocol/issues)
- **Discussions**: [GitHub Discussions](https://github.com/smrtkrft/DMF_protocol/discussions)

## 🗺️ Roadmap

See [yol_haritasi.md](yol_haritasi.md) for future development plans.

---

**Made with ❤️ for safety-conscious adventurers**

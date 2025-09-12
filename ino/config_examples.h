/*
  DMF Configuration Examples
  
  Bu dosya DMF sisteminin örnek konfigürasyonlarını içerir.
  Gerçek kullanımda web arayüzü üzerinden ayarlanır.
*/

// ==================== WiFi Configuration Examples ====================

/*
Primary WiFi Network:
SSID: "MyHomeWiFi"
Password: "MySecurePassword123"
Allow Open Networks: true
Security Mode: true

Backup Networks:
1. SSID: "MobileHotspot", Password: "hotspot123"
2. SSID: "OfficeWiFi", Password: "office456"
3. SSID: "GuestNetwork", Password: ""
*/

// ==================== ProtonMail Configuration ====================

/*
ProtonMail API Setup:
1. ProtonMail hesabınıza giriş yapın
2. Settings > Security > API Keys bölümüne gidin
3. Yeni API key oluşturun
4. Key'i web arayüzünden sisteme girin

Sender Email: "dmf-alerts@protonmail.com"
API Key: "your-protonmail-api-key-here"
*/

// ==================== Mail Groups Examples ====================

/*
Group 1 - Emergency Contacts:
Enabled: true
Emails: 
  - "admin@company.com"
  - "security@company.com"
  - "manager@company.com"
Subject: "DMF CRITICAL ALERT"
Body: "Critical timer expired on DMF device. Immediate action required."
URLs:
  - "https://api.company.com/alert/critical"
  - "https://webhook.site/your-webhook-id"
  - "https://ifttt.com/trigger/dmf_alert/json/key"

Group 2 - Notification Team:
Enabled: true  
Emails:
  - "notifications@company.com"
  - "tech-support@company.com"
Subject: "DMF Timer Notification"
Body: "Scheduled timer has expired on DMF device. Please check system status."
URLs:
  - "https://api.company.com/notify/timer"
  - "https://slack.com/api/webhooks/your-webhook"

Group 3 - Backup Systems:
Enabled: false
Emails:
  - "backup@company.com"
Subject: "DMF Backup Alert"
Body: "Backup notification from DMF system"
URLs:
  - "https://backup-system.com/api/alert"
*/

// ==================== Countdown Examples ====================

/*
Example 1 - 1 Hour Timer:
Duration: 60 minutes
Relay Priority: Mixed Priority (2)
Postpone Enabled: true
Postpone Duration: 15 minutes

Example 2 - 24 Hour Timer:
Duration: 1440 minutes
Relay Priority: Internet Dependent (1)
Postpone Enabled: true
Postpone Duration: 60 minutes

Example 3 - Emergency Timer:
Duration: 5 minutes
Relay Priority: Relay Only (3)
Postpone Enabled: false
*/

// ==================== Early Warning Examples ====================

/*
Warning 1:
Enabled: true
Minutes Before: 60
Message: "WARNING: DMF timer expires in 1 hour. Take necessary action."

Warning 2:
Enabled: true
Minutes Before: 30
Message: "URGENT: DMF timer expires in 30 minutes. Immediate attention required."

Warning 3:
Enabled: true
Minutes Before: 15
Message: "CRITICAL: DMF timer expires in 15 minutes. Final warning."

Warning 4:
Enabled: true
Minutes Before: 5
Message: "IMMEDIATE: DMF timer expires in 5 minutes. Last chance to postpone."

Warning 5:
Enabled: false
Minutes Before: 1
Message: "FINAL: DMF timer expires in 1 minute. System will trigger."
*/

// ==================== Security Best Practices ====================

/*
1. API Key Security:
   - API anahtarlarını güvenli saklayın
   - Düzenli olarak yenileyin
   - Gereksiz yetkiler vermeyin

2. Network Security:
   - Mümkünse WPA3 ağları kullanın
   - Açık ağları sadece acil durumda kullanın
   - VPN kullanımını tercih edin

3. Physical Security:
   - Cihazı güvenli yerlere kurun
   - Button erişimini kontrol edin
   - Yetkisiz erişimi engelleyin

4. Update Security:
   - Firmware güncellemelerini takip edin
   - Factory reset öncesi backup alın
   - Log kayıtlarını düzenli kontrol edin
*/

// ==================== Troubleshooting Examples ====================

/*
Problem: WiFi bağlanamıyor
Çözüm: 
1. SSID ve şifreyi kontrol edin
2. Open network tarama aktif mi?
3. Serial monitor loglarını inceleyin

Problem: Mail gönderilmiyor
Çözüm:
1. Internet bağlantısı var mı?
2. ProtonMail API key doğru mu?
3. Sender email doğru mu?

Problem: Relay tetiklenmiyor
Çözüm:
1. Relay priority ayarını kontrol edin
2. Fiziksel bağlantıları kontrol edin
3. Power supply yeterli mi?

Problem: Web arayüzü açılmıyor
Çözüm:
1. mDNS çalışıyor mu?
2. IP adresini direkt deneyin
3. Firewall ayarlarını kontrol edin
*/

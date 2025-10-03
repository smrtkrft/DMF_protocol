# 📧 Proton Mail Doğrudan Entegrasyon Rehberi

## ✅ YAPILAN DEĞİŞİKLİKLER

### Kaldırılan Özellikler (Gereksiz/Kullanılmayan)
- ❌ OAuth2 token yönetimi (clientId, clientSecret, refreshToken, accessToken)
- ❌ STARTTLS flag'leri (ESP32 kısıtlaması nedeniyle)
- ❌ Proton API REST endpoint'leri
- ❌ `establishClient()` ve eski `smtpCommand()` metodları

### Eklenen Özellikler
- ✅ Basitleştirilmiş SMTP TLS/SSL (port 465)
- ✅ Doğrudan Proton Mail app password desteği
- ✅ İyileştirilmiş hata mesajları ve debug logları
- ✅ Temiz SMTP AUTH LOGIN akışı

---

## 📋 KURULUM ADIMLARI

### 1. Proton Mail Hesap Hazırlığı

**Gereksinimler:**
- Proton Plus veya Unlimited hesap (aylık ~€5)
- SMTP/IMAP erişimi etkin

**Adımlar:**

1. **Proton Mail web'de oturum açın**: https://mail.proton.me

2. **Settings menüsüne gidin**:
   - Sağ üst → **Settings** → **All Settings**

3. **IMAP/SMTP sekmesini açın**:
   - Sol menüden **IMAP/SMTP** seçin

4. **SMTP/IMAP erişimini aktifleştirin**:
   - "Enable IMAP/SMTP" toggle'ını açın
   - Açıklama: "This allows you to use Proton Mail with third-party email clients"

5. **App Password oluşturun**:
   - "App Passwords" bölümünde **+ Create password** butonuna tıklayın
   - Name: `ESP32-DMF` (veya istediğiniz isim)
   - **Create** → 16 haneli şifre gösterilir
   
   **⚠️ ÖNEMLİ**: Bu şifreyi hemen kopyalayın - bir daha gösterilmeyecek!
   
   Örnek: `abcd1234efgh5678`

**SMTP Ayarlarınız:**
```
Server: smtp.protonmail.ch
Port: 465
Security: SSL/TLS
Username: sizin@protonmail.com
Password: [16 haneli app password]
```

---

### 2. ESP32 Kodunu Yükleme

**Arduino IDE'de:**

1. **Kod hazır** - Değişiklikler otomatik uygulandı
2. **Derle ve yükle**: Arduino IDE → Upload

**Beklenen Serial Çıktısı:**
```
=== SmartKraft DMF Başlıyor ===
[Cihaz] ID: SmartKraft-DMF123456
[Init] Donanım başlatılıyor...
[Init] Dosya sistemi başlatılıyor...
[Init] Scheduler başlatılıyor...
[Init] Network manager başlatılıyor...
[Init] Mail agent başlatılıyor...
[Init] Web sunucusu başlatılıyor...
[Init] Test arayüzü başlatılıyor...
[Init] WebServer başlatılıyor...
[Web] AP başlatıldı: 192.168.4.1
[DNS] Captive portal DNS başlatıldı
[Init] Sistem hazır!
```

---

### 3. WiFi Bağlantısı ve Yapılandırma

**A) İlk Bağlantı (Captive Portal):**

1. **Telefon/PC WiFi ayarlarına git**
2. **SmartKraft-DMF** ağına bağlan (şifre: `12345678`)
3. **Otomatik browser açılır** → Web UI görünür
   - Açılmazsa manuel: `http://192.168.4.1`

**B) İnternet WiFi Ayarı:**

1. Web UI'da **"Wi-Fi Ayarları"** sekmesine git
2. **Primary SSID**: Ev ağınızın adı
3. **Primary Password**: WiFi şifresi
4. **Kaydet** butonuna bas

**Seri monitörde:**
```
[WiFi] EvWiFi ağına bağlanıyor...
[WiFi] Bağlantı başarılı: 192.168.1.105
```

5. **Artık hem AP hem de yerel ağdan erişebilirsiniz:**
   - AP: `http://192.168.4.1`
   - Yerel: `http://192.168.1.105`

---

### 4. Mail Ayarları

Web UI'da **"Mail Ayarları"** sekmesine gidin:

**SMTP Sunucu:**
```
SMTP Server: smtp.protonmail.ch
SMTP Port: 465
Username: sizin@protonmail.com
Password: [App password - 16 hane]
```

**Alıcılar:**
```
kisi1@example.com, kisi2@example.com
```
(Virgülle ayırarak maksimum 10 alıcı)

**Uyarı Mail İçeriği:**
```
Konu: SmartKraft DMF - Uyarı #%ALARM_INDEX%

Mesaj:
Sayın Yetkili,

Alarm #%ALARM_INDEX% tetiklendi.
Kalan süre: %REMAINING%

Acil aksiyon alınız.

-- SmartKraft DMF
```

**Final Mail İçeriği:**
```
Konu: SmartKraft DMF - KRİTİK: Süre Doldu!

Mesaj:
⚠️ GERİ SAYIM TAMAMLANDI ⚠️

Röle tetiklenmiştir.
Acil müdahale gereklidir!

-- SmartKraft DMF
```

**Kaydet** butonuna basın.

---

### 5. İlk Test

**A) Seri Port Komutu:**

Arduino IDE Serial Monitor'da (115200 baud):

```
> mail test warning
```

**Beklenen çıktı:**
```
[SMTP] Bağlanıyor: smtp.protonmail.ch:465
[SMTP] << 220 smtp.protonmail.ch ESMTP ready
[SMTP] Bağlantı başarılı
[SMTP] Kimlik doğrulaması yapılıyor...
[SMTP] << 250-smtp.protonmail.ch
[SMTP] << 250 AUTH LOGIN PLAIN
[SMTP] << 334 VXNlcm5hbWU6
[SMTP] << 334 UGFzc3dvcmQ6
[SMTP] << 235 2.7.0 Authentication successful
[SMTP] Kimlik doğrulama başarılı
[SMTP] << 250 2.1.0 Ok
[SMTP] << 250 2.1.5 Ok
[SMTP] << 354 End data with <CR><LF>.<CR><LF>
[SMTP] << 250 2.0.0 Ok: queued as ABC123
[SMTP] Mail başarıyla gönderildi
[SMTP] << 221 2.0.0 Bye
[SMTP] Bağlantı kapatıldı
[Mail] Test başarılı
```

**B) Web UI'dan:**

1. **Mail Ayarları** sekmesinde **"Test Mail Gönder"** butonuna bas
2. Mail kutunuzu kontrol edin

---

## 🔧 SORUN GİDERME

### ❌ "SMTP sunucusuna bağlanılamadı"

**Muhtemel Sebepler:**
- İnternet bağlantısı yok
- `smtp.protonmail.ch` erişilemiyor
- Port 465 firewall'da kapalı

**Çözüm:**
```
1. Serial'de WiFi bağlantısını kontrol edin:
   [WiFi] Bağlantı başarılı: 192.168.1.X
   
2. ping testi (Serial):
   > network status
   
3. Tarayıcıda test: https://protonmail.ch
```

---

### ❌ "Kimlik doğrulama başarısız"

**Muhtemel Sebepler:**
- App password yanlış kopyalanmış
- Proton hesabı askıya alınmış
- SMTP/IMAP erişimi kapalı

**Çözüm:**
```
1. App password'u yeniden oluşturun
2. Boşluk/enter karakteri olmadığından emin olun
3. Proton web'de oturum açabildiğinizi doğrulayın
4. IMAP/SMTP toggle'ının açık olduğunu kontrol edin
```

**Serial'de:**
```
[SMTP] << 535 5.7.8 Error: authentication failed
```
→ Şifre yanlış

---

### ❌ "Mail gönderildi ama ulaşmadı"

**Muhtemel Sebepler:**
- Spam klasörüne düştü
- Proton rate limit (çok fazla mail)
- Alıcı adresi yanlış

**Çözüm:**
```
1. Spam/Junk klasörünü kontrol edin
2. Serial'de alıcı adreslerini kontrol edin:
   [SMTP] << 250 2.1.5 Ok  ← Her alıcı için
   
3. Proton web'de "Sent" klasörünü kontrol edin
```

---

### ⚠️ "Port 587 çalışmıyor"

**Sebep:** ESP32'nin WiFiClientSecure kütüphanesi STARTTLS upgrade'i tam desteklemiyor.

**Çözüm:** Port 465 kullanın (kod zaten 465'e ayarlı).

---

## 📊 ÖZELLİKLER

### ✅ Desteklenenler
- ✅ Port 465 (TLS/SSL)
- ✅ Proton Mail App Password
- ✅ Çoklu alıcı (max 10)
- ✅ Dosya ekleri (max 5, toplam 2MB)
- ✅ Template değişkenleri (%ALARM_INDEX%, %REMAINING%)
- ✅ Detaylı Serial debug logları

### ❌ Desteklenmeyenler
- ❌ Port 587 (STARTTLS) - ESP32 kısıtlaması
- ❌ OAuth2 token yönetimi - Gereksiz
- ❌ Proton API - SMTP daha basit

---

## 🎯 BEST PRACTICES

1. **App Password güvenliği:**
   - Web UI'dan sadece okuma yetkisi
   - Şifreyi güvenli yerde saklayın
   - Gerekirse yeniden oluşturun

2. **Test sıklığı:**
   - Her yeni kurulumda test mail gönderin
   - Ayda bir kez test yapın

3. **Alıcı sayısı:**
   - Kritik alarmlar için 2-3 alıcı yeterli
   - Çok fazla alıcı spam riski

4. **Port seçimi:**
   - **Her zaman 465 kullanın**
   - 587 deneysel ve sorunlu

---

## 📧 ÖRNEK KULLANIM SENARYOSU

**Kurulum:**
1. Proton Plus hesabı oluştur (€4.99/ay)
2. App password al: `xyz123abc456def7`
3. ESP32'yi programla ve başlat
4. WiFi ayarlarını yap
5. Mail ayarlarını yap ve test et

**Timer Ayarları:**
```
Toplam Süre: 7 gün
Alarm Sayısı: 3
```

**Beklenen Davranış:**
- 4. gün: Alarm #1 maili gider
- 5. gün: Alarm #2 maili gider
- 6. gün: Alarm #3 maili gider
- 7. gün: **RÖLE TETİKLENİR** + Final mail

**Her alarm için:**
- Mail gönderilir (Proton SMTP)
- Serial'de log görünür
- Alıcılara ulaşır

---

## 📝 NOTLAR

- Proton Mail free hesap SMTP desteklemiyor
- App password maksimum 10 adet oluşturulabilir
- Rate limit: ~100 mail/saat (Proton Plus)
- Attachment toplam boyutu LittleFS sınırlı (max 2MB)

---

**Başarılar! 🎉**

Sorularınız için: Serial Monitor'daki logları paylaşın.

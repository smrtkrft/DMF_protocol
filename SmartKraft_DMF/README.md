# SmartKraft DMF – ESP32-C6 Dijital Manuel Fail-Safe

Bu proje, Seeed Studio XIAO ESP32-C6 kartı üzerinde çalışan, uzun süreli (60 güne kadar) geri sayım ve hatırlatma alarmları sağlayan bir otomasyon prototipidir. Web arayüzü ile tüm ayarları yapabilir, ProtonMail üzerinden uyarı ve final e-postaları gönderebilir, bir GET URL tetikleyebilir ve geri sayım sonlandığında röleyi aktif edebilirsiniz. Fiziksel veya web butonu kullanılarak süre sıfırlanabilir ve sistem, elektrik kesintisi sonrası kaldığı yerden devam eder.

## Başlıca Özellikler

- **Zamanlayıcı**: Gün veya saat bazlı 1–60 arası süre tanımlama
- **Otomatik alarm dağılımı**: Son gün/saatlere otomatik yerleştirilen hatırlatma alarmları (maks. 10)
- **Fiziksel ve sanal buton**: Dahili buton (GPIO4, GND) veya web arayüzü üzerinden geri sayımı sıfırlama
- **Röle kontrolü**: Süre tamamlandığında GPIO10 ile röleyi tetikleme
- **E-posta + GET**: ProtonMail SMTP (TLS, opsiyonel OAuth) ile uyarı/final maili ve GET isteği
- **Dosya ekleri**: Maks. 5 adet toplam 2 MB’a kadar PDF, görsel, ses vb. dosyaları final mailine ekleme
- **Wi-Fi yönetimi**: İki kayıtlı SSID ve alarm anında açık ağlara otomatik bağlanma
- **Kalıcılık**: Tüm ayarlar LittleFS üzerinde saklanır, geri sayım durumu NVS/LittleFS ile korunur
- **Terminal tarzı web UI**: Üç sekmeli (Alarm, Mail, Wi-Fi) monospace arayüz, gerçek zamanlı durum
- **Test yardımcıları**: Seri port komutları ve web üzerinden test maili gönderme

## Donanım Bağlantıları

| Pin | İşlev | Not |
| --- | --- | --- |
| GPIO4 | Fiziksel buton | Butonun diğer ucu GND’ye bağlanmalı (dahili pull-up)
| GPIO10 | Röle çıkışı | Röle sürücü transistörü/optokuplör üzerinden kullanılmalıdır
| 3V3 / GND | Sensörler / modüller | XIAO ESP32-C6 3.3 V çıkışı üzerinden besleyin

## Yazılım Bileşenleri

- `SmartKraft_DMF.ino`: Giriş noktası, zamanlayıcı döngüsü, buton, röle ve alarm işleme
- `config_store.*`: LittleFS tabanlı ayar ve durum saklama katmanı
- `scheduler.*`: Süre/alarmların hesaplanması ve yönetimi
- `network_manager.*`: Wi-Fi tercihleri, tarama ve açık ağ fallback mantığı
- `mail_functions.*`: ProtonMail SMTP + MIME ekleri + (opsiyonel) OAuth yenileme
- `web_handlers.*`: HTTP API’leri, dosya yükleme ve JSON cevapları
- `test_functions.*`: Seri port komutları (`status`, `start`, `reset`, `stop`, `mail`)
- `data/index.html`: Terminal tarzı üç sekmeli web arayüzü

## Kurulum

1. **Arduino IDE**
   - ESP32 eklentisini (2.0.17+), Seeed XIAO ESP32-C6 kart paketini kurun
   - Aşağıdaki kütüphaneleri ekleyin: `ArduinoJson`, `ESP32 filesystem uploader` araçları
   - Board: **Seeed XIAO ESP32-C6**
   - Upload Speed: 921600
2. **LittleFS içeriğini yükleyin**
   - `SmartKraft_DMF/data` klasöründeki `index.html` ve ek dosyaları LittleFS’e aktarmak için ESP32 LittleFS Data Upload aracını kullanın
3. **Derleme & Yükleme**
   - `SmartKraft_DMF.ino` dosyasını açın
   - Kartı bağlayın, doğru portu seçin
   - Sketch’i derleyip karta yükleyin

## Web Arayüzü Kullanımı

1. Cihaz açıldığında kayıtlı Wi-Fi ağlarına bağlanmaya çalışır. İlk kurulumda seri port üzerinden IP’yi görebilirsiniz
2. Tarayıcıda cihaz IP adresini açın
3. **Alarm Ayarları** sekmesinde süre birimini (gün/saat), toplam süreyi ve alarm sayısını belirleyin, kaydedin ve `Timer Başlat` butonuna tıklayın
4. **Mail Ayarları** sekmesinde SMTP bilgileri, mail listesi, uyarı/final konu-metni ve GET URL’lerini girin
   - ProtonMail için tavsiye edilen ayar: Sunucu `smtp.protonmail.com`, Port `465`, TLS aktif, STARTTLS kapalı (şu an desteklenmiyor)
   - OAuth istenirse client ID/secret ile refresh token girilebilir (Proton API beta erişimi gerektirir)
5. Dosya eklemek için `Dosya yüklemek için tıklayın` alanını kullanın; yüklenen dosyaları uyarı veya final mailine dahil etmek için işaret kutularını kullanın
6. **Wi-Fi Ayarları** sekmesinde birincil/yedek SSID ve şifreleri kaydedin; açık ağlara bağlanma izni verebilirsiniz
7. “Fiziksel Buton Simülasyonu” butonu veya gerçek buton basışı geri sayımı sıfırlar ve röleyi kapatır

## Zamanlayıcı Davranışı

- Kullanıcı süreyi ve alarm sayısını belirledikten sonra sistem, hatırlatma alarmlarını son gün/saatlere otomatik yerleştirir
- Her alarm tetiklendiğinde: (1) Uyarı maili gönderilir, (2) GET URL çağrılır
- Süre tamamlandığında: (1) Röle HIGH yapılır, (2) Final maili gönderilir, (3) Final GET URL’i çağrılır
- Alarm veya final sırasında internet yoksa, cihaz açık ağları tarar ve bağlantıyı sağlayana kadar tekrar dener
- Fiziksel buton veya web butonu basıldığında geri sayım baştan başlar ve röle kapanır

## Bilinen Sınırlar

- STARTTLS desteği henüz uygulanmadı; TLS bağlantısı için port 465 kullanın
- OAuth yenileme Proton API erişimi gerektirir; refresh token, client ID ve secret alanları isteğe bağlıdır
- Toplam ek dosya boyutu 2 MB ile sınırlıdır

## Seri Port Komutları

| Komut | Açıklama |
| --- | --- |
| `status` | Zamanlayıcı durumu, kalan süre, sıradaki alarm |
| `start` | Zamanlayıcıyı başlat |
| `reset` | Zamanlayıcıyı sıfırla ve toplam süreye ayarla |
| `stop` | Geri sayımı durdur |
| `mail` | Test uyarı maili gönder |

## Güvenlik Notları

- Kayıtlı kimlik bilgileri LittleFS’te saklanır. İsteğe bağlı olarak dosya düzeyinde ek şifreleme uygulanabilir
- Açık ağlara bağlanma opsiyonu güvenlik riski barındırır; yalnızca acil durum gönderimleri için aktifleştirin
- ProtonMail OAuth tokenları hassastır; arayüzde girilen değerler aynı şekilde saklanır

## Geliştirme Notları

- Kodlama stili: modern C++ (ESP-IDF uyumlu), modüler başlık/dosyalar
- `LittleFS` ile ayar dosyaları: `/timer.json`, `/mail.json`, `/wifi.json`, `/runtime.json`
- Ek dosyalar `/attachments` klasöründe saklanır
- Zamanlayıcı hesaplamaları `CountdownScheduler` içinde yapılır ve kalıcı durum `ConfigStore` üzerinden yönetilir
- Web API uçları `web_handlers` içinde JSON tabanlıdır; AJAX ile erişilir

---

Sorularınız veya geliştirme önerileriniz için proje yorumlarına bakabilirsiniz.

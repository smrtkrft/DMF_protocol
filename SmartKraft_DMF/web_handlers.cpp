#include "web_handlers.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>

// Embedded HTML content
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="tr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SmartKraft DMF Kontrol Paneli</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: monospace; background: #000; color: #fff; line-height: 1.4; font-size: 14px; }
        a { color: #0f0; }
        .container { max-width: 820px; margin: 0 auto; padding: 16px; }
        .header { text-align: center; margin-bottom: 20px; padding-bottom: 12px; border-bottom: 1px solid #333; }
        .header h1 { font-size: 1.8em; font-weight: normal; letter-spacing: 2px; }
        .device-id { color: #777; font-size: 0.9em; margin-top: 4px; }
        .status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(160px, 1fr)); gap: 12px; margin-bottom: 20px; border: 1px solid #333; padding: 12px; }
        .status-card { text-align: center; }
        .status-label { color: #666; font-size: 0.8em; margin-bottom: 4px; text-transform: uppercase; }
        .status-value { font-size: 1.2em; color: #fff; min-height: 1.2em; }
        .timer-readout { text-align: center; border: 1px solid #333; padding: 18px; margin-bottom: 16px; }
        .timer-readout .value { font-size: 2.6em; letter-spacing: 2px; }
        .timer-readout .label { color: #777; margin-top: 6px; font-size: 0.85em; }
        .button-bar { display: flex; flex-wrap: wrap; gap: 8px; justify-content: center; margin-bottom: 24px; }
        button { background: transparent; border: 1px solid #555; color: #fff; padding: 10px 18px; font-family: monospace; cursor: pointer; text-transform: uppercase; letter-spacing: 1px; transition: background 0.2s; }
        button:hover { background: #222; }
        .btn-danger { border-color: #f00; color: #f00; }
        .btn-danger:hover { background: #f00; color: #000; }
        .btn-success { border-color: #0f0; color: #0f0; }
        .btn-success:hover { background: #0f0; color: #000; }
        .btn-warning { border-color: #ff0; color: #ff0; }
        .btn-warning:hover { background: #ff0; color: #000; }
        .tabs { display: flex; flex-wrap: wrap; border-bottom: 1px solid #333; margin-bottom: 8px; }
        .tab { flex: 1; min-width: 140px; border: 1px solid #333; border-bottom: none; background: #000; color: #666; padding: 10px; cursor: pointer; text-align: center; font-size: 0.9em; }
        .tab + .tab { margin-left: 4px; }
        .tab.active { color: #fff; border-color: #fff; }
        .tab-content { border: 1px solid #333; padding: 20px; }
        .tab-pane { display: none; }
        .tab-pane.active { display: block; }
        .form-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 12px; }
        .form-group { display: flex; flex-direction: column; gap: 6px; margin-bottom: 16px; }
        label { font-size: 0.85em; color: #ccc; text-transform: uppercase; letter-spacing: 1px; }
        input[type="text"], input[type="number"], input[type="password"], input[type="email"], textarea, select { width: 100%; padding: 10px; background: #000; border: 1px solid #333; color: #fff; font-family: monospace; }
        textarea { resize: vertical; min-height: 100px; }
        .checkbox { display: flex; align-items: center; gap: 8px; font-size: 0.9em; color: #ccc; }
        .section-title { border-bottom: 1px solid #333; padding-bottom: 6px; margin-top: 8px; margin-bottom: 12px; font-size: 1em; letter-spacing: 1px; text-transform: uppercase; }
        .attachments { border: 1px solid #333; padding: 12px; margin-bottom: 16px; }
        .attachments table { width: 100%; border-collapse: collapse; font-size: 0.85em; }
        .attachments th, .attachments td { border-bottom: 1px solid #222; padding: 6px; text-align: left; }
        .attachments th { color: #888; text-transform: uppercase; letter-spacing: 1px; }
        .file-upload { border: 1px dashed #555; padding: 20px; text-align: center; margin-bottom: 12px; cursor: pointer; }
        .file-upload:hover { background: #111; }
        .alert { display: none; margin-bottom: 12px; padding: 10px; border: 1px solid #333; font-size: 0.85em; }
        .alert.success { border-color: #0f0; color: #0f0; }
        .alert.error { border-color: #f00; color: #f00; }
        .list { border: 1px solid #333; padding: 10px; max-height: 180px; overflow-y: auto; font-size: 0.85em; }
        .list-item { border-bottom: 1px solid #222; padding: 6px 0; display: flex; justify-content: space-between; align-items: center; }
        .list-item:last-child { border-bottom: none; }
        .badge { display: inline-block; padding: 2px 6px; font-size: 0.75em; border: 1px solid #333; margin-left: 6px; }
        .connection-indicator { position: fixed; top: 12px; right: 12px; border: 1px solid #333; padding: 6px 10px; font-size: 0.8em; z-index: 20; background: #000; max-width: 280px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
        .connection-indicator.online { border-color: #0f0; color: #0f0; }
        .connection-indicator.offline { border-color: #f00; color: #f00; }
        @media (max-width: 600px) {
            .connection-indicator { top: 8px; right: 8px; left: 8px; max-width: none; font-size: 0.7em; padding: 4px 8px; }
            .tabs { flex-direction: column; }
            .tab + .tab { margin-left: 0; margin-top: 4px; }
            button { width: 100%; }
        }
    </style>
</head>
<body>
    <div id="connectionStatus" class="connection-indicator offline">Bağlantı kontrol ediliyor…</div>
    <div class="container">
        <div class="header">
            <h1>SMARTKRAFT DMF</h1>
            <div class="device-id">Cihaz ID: <span id="deviceId">—</span></div>
        </div>

        <div class="status-grid">
            <div class="status-card">
                <div class="status-label">Timer Durumu</div>
                <div class="status-value" id="timerStatus">—</div>
            </div>
            <div class="status-card">
                <div class="status-label">Kalan Süre</div>
                <div class="status-value" id="remainingTime">—</div>
            </div>
            <div class="status-card">
                <div class="status-label">Sonraki Alarm</div>
                <div class="status-value" id="nextAlarm">—</div>
            </div>
            <div class="status-card">
                <div class="status-label">Wi-Fi</div>
                <div class="status-value" id="wifiStatus">—</div>
            </div>
        </div>

        <div class="timer-readout">
            <div class="value" id="timerDisplay">00:00:00</div>
            <div class="label">Geri Sayım</div>
        </div>

        <div class="button-bar">
            <button id="btnStart" class="btn-success" onclick="startTimer()">Başlat</button>
            <button id="btnPause" class="btn-warning" onclick="pauseTimer()" style="display:none;">Duraklat</button>
            <button id="btnResume" class="btn-success" onclick="resumeTimer()" style="display:none;">Devam Et</button>
            <button id="btnReset" class="btn-danger" onclick="resetTimer()">Sıfırla</button>
            <button id="btnPhysical" class="btn-danger" onclick="virtualButton()">Fiziksel Buton</button>
        </div>

        <div class="tabs">
            <div class="tab active" data-tab="alarmTab" onclick="openTab(event, 'alarmTab')">Alarm Ayarları</div>
            <div class="tab" data-tab="mailTab" onclick="openTab(event, 'mailTab')">Mail Ayarları</div>
            <div class="tab" data-tab="wifiTab" onclick="openTab(event, 'wifiTab')">Wi-Fi Ayarları</div>
        </div>

        <div class="tab-content">
            <div id="alarmTab" class="tab-pane active">
                <div id="alarmAlert" class="alert"></div>
                <div class="section-title">Geri Sayım Parametreleri</div>
                <div class="form-grid">
                    <div class="form-group">
                        <label>Süre Birimi</label>
                        <select id="timerUnit">
                            <option value="days">Gün</option>
                            <option value="hours">Saat</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label>Toplam Süre (1-60)</label>
                        <input type="number" id="timerTotal" min="1" max="60" value="7">
                    </div>
                    <div class="form-group">
                        <label>Alarm Sayısı (0-10)</label>
                        <input type="number" id="timerAlarms" min="0" max="10" value="3">
                    </div>
                    <div class="form-group checkbox">
                        <input type="checkbox" id="timerEnabled" checked>
                        <label for="timerEnabled">Zamanlayıcı Etkin</label>
                    </div>
                </div>
                <button onclick="saveTimerSettings()">Ayarları Kaydet</button>

                <div class="section-title">Planlanan Alarmlar</div>
                <div class="list" id="alarmSchedule">—</div>
            </div>

            <div id="mailTab" class="tab-pane">
                <div id="mailAlert" class="alert"></div>
                <div class="section-title">SMTP / ProtonMail Ayarları</div>
                <div class="form-grid">
                    <div class="form-group">
                        <label>SMTP Sunucu</label>
                        <input type="text" id="smtpServer" placeholder="smtp.protonmail.com">
                    </div>
                    <div class="form-group">
                        <label>SMTP Port</label>
                        <input type="number" id="smtpPort" value="587">
                    </div>
                    <div class="form-group checkbox">
                        <input type="checkbox" id="smtpTLS" checked>
                        <label for="smtpTLS">TLS Zorunlu</label>
                    </div>
                    <div class="form-group checkbox">
                        <input type="checkbox" id="smtpStartTLS">
                        <label for="smtpStartTLS">STARTTLS Kullan</label>
                    </div>
                    <div class="form-group checkbox">
                        <input type="checkbox" id="smtpOAuth">
                        <label for="smtpOAuth">OAuth (XOAUTH2)</label>
                    </div>
                </div>

                <div class="form-grid">
                    <div class="form-group">
                        <label>Kullanıcı E-Postası</label>
                        <input type="email" id="smtpUsername" placeholder="kullanici@protonmail.com">
                    </div>
                    <div class="form-group">
                        <label>Uygulama Şifresi / Token</label>
                        <input type="password" id="smtpPassword" placeholder="App password veya boş">
                    </div>
                    <div class="form-group">
                        <label>OAuth Client ID</label>
                        <input type="text" id="oauthClientId" placeholder="Opsiyonel">
                    </div>
                    <div class="form-group">
                        <label>OAuth Client Secret</label>
                        <input type="text" id="oauthClientSecret" placeholder="Opsiyonel">
                    </div>
                    <div class="form-group">
                        <label>Refresh Token</label>
                        <input type="text" id="oauthRefreshToken" placeholder="Opsiyonel">
                    </div>
                </div>

                <div class="section-title">Mail Listesi (10 Alıcıya kadar)</div>
                <div class="form-group">
                    <label>E-Posta Adresleri (virgül veya yeni satır)</label>
                    <textarea id="mailRecipients" placeholder="user1@example.com, user2@example.com"></textarea>
                </div>

                <div class="section-title">Uyarı (Hatırlatma) İçeriği</div>
                <div class="form-group">
                    <label>Konu</label>
                    <input type="text" id="warningSubject" placeholder="SmartKraft DMF Uyarısı">
                </div>
                <div class="form-group">
                    <label>Mesaj</label>
                    <textarea id="warningBody" placeholder="Uyarı şablonu"></textarea>
                </div>
                <div class="form-group">
                    <label>GET URL</label>
                    <input type="text" id="warningUrl" placeholder="https://example.com/warning">
                </div>

                <div class="section-title">Final İçeriği</div>
                <div class="form-group">
                    <label>Konu</label>
                    <input type="text" id="finalSubject" placeholder="SmartKraft DMF Final">
                </div>
                <div class="form-group">
                    <label>Mesaj</label>
                    <textarea id="finalBody" placeholder="Final şablonu"></textarea>
                </div>
                <div class="form-group">
                    <label>GET URL</label>
                    <input type="text" id="finalUrl" placeholder="https://example.com/final">
                </div>

                <div class="section-title">Ek Dosyalar (maks. 5 adet / 2 MB toplam)</div>
                <div class="file-upload" onclick="document.getElementById('fileInput').click()">
                    Dosya yüklemek için tıklayın
                    <input type="file" id="fileInput" style="display:none" onchange="uploadAttachment(event)">
                </div>
                <div class="attachments">
                    <table>
                        <thead>
                            <tr>
                                <th>Dosya</th>
                                <th>Boyut</th>
                                <th>Uyarı</th>
                                <th>Final</th>
                                <th></th>
                            </tr>
                        </thead>
                        <tbody id="attachmentRows"></tbody>
                    </table>
                </div>

                <div class="button-bar" style="justify-content: flex-start;">
                    <button onclick="saveMailSettings()">Mail Ayarlarını Kaydet</button>
                    <button class="btn-warning" onclick="sendTestMail()">Test Maili Gönder</button>
                </div>
            </div>

            <div id="wifiTab" class="tab-pane">
                <div id="wifiAlert" class="alert"></div>
                <div class="section-title">Wi-Fi Profilleri</div>
                <div class="form-grid">
                    <div class="form-group">
                        <label>Birincil SSID</label>
                        <input type="text" id="wifiPrimarySsid" placeholder="Ana ağ adı">
                    </div>
                    <div class="form-group">
                        <label>Birincil Şifre</label>
                        <input type="password" id="wifiPrimaryPassword" placeholder="Ana ağ şifresi">
                    </div>
                    <div class="form-group">
                        <label>Yedek SSID</label>
                        <input type="text" id="wifiSecondarySsid" placeholder="Yedek ağ adı">
                    </div>
                    <div class="form-group">
                        <label>Yedek Şifre</label>
                        <input type="password" id="wifiSecondaryPassword" placeholder="Yedek ağ şifresi">
                    </div>
                    <div class="form-group checkbox">
                        <input type="checkbox" id="wifiAllowOpen">
                        <label for="wifiAllowOpen">Alarm sırasında açık ağlara bağlan</label>
                    </div>
                </div>
                <div class="button-bar" style="justify-content:flex-start;">
                    <button onclick="saveWiFiSettings()">Wi-Fi Ayarlarını Kaydet</button>
                    <button class="btn-warning" onclick="scanNetworks()">Ağları Tara</button>
                </div>
                <div class="section-title">Bulunan Ağlar</div>
                <div class="list" id="wifiScanResults">—</div>
            </div>
        </div>
    </div>

    <script>
        const state = {
            timer: {},
            status: {},
            mail: { attachments: [] },
            wifi: {}
        };

        async function api(path, options = {}) {
            const defaultHeaders = options.headers || {};
            if (options.body && !(options.body instanceof FormData)) {
                defaultHeaders['Content-Type'] = 'application/json';
                options.body = JSON.stringify(options.body);
            }
            options.headers = defaultHeaders;
            const response = await fetch(path, options);
            if (!response.ok) {
                const text = await response.text();
                throw new Error(text || response.statusText);
            }
            if (response.status === 204) return null;
            const text = await response.text();
            try { return JSON.parse(text); } catch { return text; }
        }

        function showAlert(id, message, type = 'success') {
            const el = document.getElementById(id);
            if (!el) return;
            el.textContent = message;
            el.className = `alert ${type}`;
            el.style.display = 'block';
            setTimeout(() => { el.style.display = 'none'; }, 4000);
        }

        function openTab(event, id) {
            document.querySelectorAll('.tab').forEach(tab => tab.classList.remove('active'));
            document.querySelectorAll('.tab-pane').forEach(pane => pane.classList.remove('active'));
            event.currentTarget.classList.add('active');
            document.getElementById(id).classList.add('active');
        }

        function formatDuration(seconds) {
            const days = Math.floor(seconds / 86400);
            const hours = Math.floor((seconds % 86400) / 3600);
            const minutes = Math.floor((seconds % 3600) / 60);
            const secs = seconds % 60;
            if (days > 0) {
                return `${days}g ${hours.toString().padStart(2,'0')}sa ${minutes.toString().padStart(2,'0')}dk`;
            }
            return `${hours.toString().padStart(2,'0')}:${minutes.toString().padStart(2,'0')}:${secs.toString().padStart(2,'0')}`;
        }

        function updateStatusView() {
            const s = state.status;
            const connection = document.getElementById('connectionStatus');
            if (s.deviceId) {
                document.getElementById('deviceId').textContent = s.deviceId;
            }
            if (s.wifiConnected) {
                connection.textContent = `Wi-Fi: ${s.ssid || '—'} (${s.ip || '-'})`;
                connection.classList.add('online');
                connection.classList.remove('offline');
            } else {
                connection.textContent = 'Wi-Fi: Bağlı değil';
                connection.classList.remove('online');
                connection.classList.add('offline');
            }

            // Update button visibility based on timer state
            const btnStart = document.getElementById('btnStart');
            const btnPause = document.getElementById('btnPause');
            const btnResume = document.getElementById('btnResume');
            const isStopped = !s.timerActive;
            const isPaused = s.paused;
            const isRunning = s.timerActive && !s.paused;

            btnStart.style.display = isStopped ? 'inline-block' : 'none';
            btnPause.style.display = isRunning ? 'inline-block' : 'none';
            btnResume.style.display = isPaused ? 'inline-block' : 'none';

            document.getElementById('timerStatus').textContent = isPaused ? 'Duraklatıldı' : (s.timerActive ? 'Çalışıyor' : 'Durduruldu');
            document.getElementById('remainingTime').textContent = formatDuration(s.remainingSeconds || 0);
            document.getElementById('timerDisplay').textContent = formatDuration(s.remainingSeconds || 0);

            if (s.alarms && s.alarms.length > s.nextAlarmIndex) {
                const nextOffset = s.alarms[s.nextAlarmIndex];
                const total = s.totalSeconds || 0;
                const elapsed = total - (s.remainingSeconds || 0);
                const remainingToNext = Math.max(nextOffset - elapsed, 0);
                document.getElementById('nextAlarm').textContent = formatDuration(remainingToNext);
            } else {
                document.getElementById('nextAlarm').textContent = '—';
            }

            const wifiStatus = s.wifiConnected ? `${s.ssid || 'Ağ yok'} (${s.ip || '-'})` : 'Bağlı değil';
            document.getElementById('wifiStatus').textContent = wifiStatus;

            const scheduleEl = document.getElementById('alarmSchedule');
            if (!s.alarms || s.alarms.length === 0) {
                scheduleEl.innerHTML = 'Alarm bulunmuyor';
            } else {
                const totalSeconds = s.totalSeconds || 0;
                const elapsed = totalSeconds - (s.remainingSeconds || 0);
                scheduleEl.innerHTML = s.alarms.map((offset, idx) => {
                    const remaining = Math.max(offset - elapsed, 0);
                    return `<div class="list-item">Alarm ${idx + 1}<span class="badge">${formatDuration(remaining)}</span></div>`;
                }).join('');
            }
        }

        async function loadStatus() {
            try {
                state.status = await api('/api/status');
                updateStatusView();
            } catch (err) {
                console.error(err);
            }
        }

        async function loadTimerSettings() {
            try {
                state.timer = await api('/api/timer');
                document.getElementById('timerUnit').value = state.timer.unit;
                document.getElementById('timerTotal').value = state.timer.totalValue;
                document.getElementById('timerAlarms').value = state.timer.alarmCount;
                document.getElementById('timerEnabled').checked = state.timer.enabled;
            } catch (err) {
                console.error(err);
            }
        }

        async function saveTimerSettings() {
            try {
                const payload = {
                    unit: document.getElementById('timerUnit').value,
                    totalValue: Number(document.getElementById('timerTotal').value),
                    alarmCount: Number(document.getElementById('timerAlarms').value),
                    enabled: document.getElementById('timerEnabled').checked
                };
                await api('/api/timer', { method: 'PUT', body: payload });
                showAlert('alarmAlert', 'Alarm ayarları kaydedildi');
                await loadStatus();
            } catch (err) {
                showAlert('alarmAlert', err.message || 'Kaydedilemedi', 'error');
            }
        }

        async function startTimer() {
            await api('/api/timer/start', { method: 'POST' });
            await loadStatus();
        }
        async function pauseTimer() {
            await api('/api/timer/stop', { method: 'POST' });
            await loadStatus();
        }
        async function resumeTimer() {
            await api('/api/timer/resume', { method: 'POST' });
            await loadStatus();
        }
        async function resetTimer() {
            await api('/api/timer/reset', { method: 'POST' });
            latchRelayDisplay(false);
            await loadStatus();
        }
        async function virtualButton() {
            await api('/api/timer/virtual-button', { method: 'POST' });
            latchRelayDisplay(false);
            await loadStatus();
        }

        function collectRecipients() {
            const raw = document.getElementById('mailRecipients').value;
            const list = raw.split(/[\n,]/).map(x => x.trim()).filter(Boolean);
            return Array.from(new Set(list)).slice(0, 10);
        }

        function updateAttachmentTable() {
            const rows = document.getElementById('attachmentRows');
            if (!state.mail.attachments || state.mail.attachments.length === 0) {
                rows.innerHTML = '<tr><td colspan="5">Dosya yok</td></tr>';
                return;
            }
            rows.innerHTML = state.mail.attachments.map((att, index) => {
                return `<tr>
                    <td>${att.displayName}</td>
                    <td>${(att.size / 1024).toFixed(1)} KB</td>
                    <td><input type="checkbox" onchange="toggleAttachment(${index}, 'forWarning', this.checked)" ${att.forWarning ? 'checked' : ''}></td>
                    <td><input type="checkbox" onchange="toggleAttachment(${index}, 'forFinal', this.checked)" ${att.forFinal ? 'checked' : ''}></td>
                    <td><button onclick="deleteAttachment('${att.storedPath}')">Sil</button></td>
                </tr>`;
            }).join('');
        }

        function toggleAttachment(index, field, value) {
            state.mail.attachments[index][field] = value;
        }

        async function uploadAttachment(event) {
            const file = event.target.files[0];
            if (!file) return;
            const form = new FormData();
            form.append('file', file);
            try {
                const response = await fetch('/api/upload', { method: 'POST', body: form });
                if (!response.ok) throw new Error('Dosya yüklenemedi');
                await loadMailSettings();
                showAlert('mailAlert', 'Dosya eklendi');
            } catch (err) {
                showAlert('mailAlert', err.message || 'Dosya yüklenemedi', 'error');
            } finally {
                event.target.value = '';
            }
        }

        async function deleteAttachment(path) {
            try {
                await api(`/api/attachments?path=${encodeURIComponent(path)}`, { method: 'DELETE' });
                await loadMailSettings();
            } catch (err) {
                showAlert('mailAlert', err.message || 'Silinemedi', 'error');
            }
        }

        async function loadMailSettings() {
            try {
                state.mail = await api('/api/mail');
                document.getElementById('smtpServer').value = state.mail.smtpServer || '';
                document.getElementById('smtpPort').value = state.mail.smtpPort || 587;
                document.getElementById('smtpTLS').checked = state.mail.useTLS;
                document.getElementById('smtpStartTLS').checked = state.mail.useStartTLS;
                document.getElementById('smtpOAuth').checked = state.mail.useOAuth;
                document.getElementById('smtpUsername').value = state.mail.username || '';
                document.getElementById('smtpPassword').value = state.mail.password || '';
                document.getElementById('oauthClientId').value = state.mail.clientId || '';
                document.getElementById('oauthClientSecret').value = state.mail.clientSecret || '';
                document.getElementById('oauthRefreshToken').value = state.mail.refreshToken || '';
                document.getElementById('mailRecipients').value = (state.mail.recipients || []).join('\n');
                document.getElementById('warningSubject').value = state.mail.warning?.subject || '';
                document.getElementById('warningBody').value = state.mail.warning?.body || '';
                document.getElementById('warningUrl').value = state.mail.warning?.getUrl || '';
                document.getElementById('finalSubject').value = state.mail.final?.subject || '';
                document.getElementById('finalBody').value = state.mail.final?.body || '';
                document.getElementById('finalUrl').value = state.mail.final?.getUrl || '';
                updateAttachmentTable();
            } catch (err) {
                console.error(err);
            }
        }

        async function saveMailSettings() {
            try {
                const payload = {
                    smtpServer: document.getElementById('smtpServer').value,
                    smtpPort: Number(document.getElementById('smtpPort').value),
                    useTLS: document.getElementById('smtpTLS').checked,
                    useStartTLS: document.getElementById('smtpStartTLS').checked,
                    useOAuth: document.getElementById('smtpOAuth').checked,
                    username: document.getElementById('smtpUsername').value,
                    password: document.getElementById('smtpPassword').value,
                    clientId: document.getElementById('oauthClientId').value,
                    clientSecret: document.getElementById('oauthClientSecret').value,
                    refreshToken: document.getElementById('oauthRefreshToken').value,
                    accessToken: state.mail.accessToken || '',
                    accessTokenExpiry: state.mail.accessTokenExpiry || 0,
                    recipients: collectRecipients(),
                    warning: {
                        subject: document.getElementById('warningSubject').value,
                        body: document.getElementById('warningBody').value,
                        getUrl: document.getElementById('warningUrl').value
                    },
                    final: {
                        subject: document.getElementById('finalSubject').value,
                        body: document.getElementById('finalBody').value,
                        getUrl: document.getElementById('finalUrl').value
                    },
                    attachments: state.mail.attachments || []
                };
                await api('/api/mail', { method: 'PUT', body: payload });
                showAlert('mailAlert', 'Mail ayarları kaydedildi');
            } catch (err) {
                showAlert('mailAlert', err.message || 'Kaydedilemedi', 'error');
            }
        }

        async function sendTestMail() {
            try {
                await api('/api/mail/test', { method: 'POST' });
                showAlert('mailAlert', 'Test maili gönderildi');
            } catch (err) {
                showAlert('mailAlert', err.message || 'Test maili başarısız', 'error');
            }
        }

        async function loadWiFiSettings() {
            try {
                state.wifi = await api('/api/wifi');
                document.getElementById('wifiPrimarySsid').value = state.wifi.primarySSID || '';
                document.getElementById('wifiPrimaryPassword').value = state.wifi.primaryPassword || '';
                document.getElementById('wifiSecondarySsid').value = state.wifi.secondarySSID || '';
                document.getElementById('wifiSecondaryPassword').value = state.wifi.secondaryPassword || '';
                document.getElementById('wifiAllowOpen').checked = state.wifi.allowOpenNetworks;
            } catch (err) {
                console.error(err);
            }
        }

        async function saveWiFiSettings() {
            try {
                const payload = {
                    primarySSID: document.getElementById('wifiPrimarySsid').value,
                    primaryPassword: document.getElementById('wifiPrimaryPassword').value,
                    secondarySSID: document.getElementById('wifiSecondarySsid').value,
                    secondaryPassword: document.getElementById('wifiSecondaryPassword').value,
                    allowOpenNetworks: document.getElementById('wifiAllowOpen').checked
                };
                await api('/api/wifi', { method: 'PUT', body: payload });
                showAlert('wifiAlert', 'Wi-Fi ayarları kaydedildi');
            } catch (err) {
                showAlert('wifiAlert', err.message || 'Kaydedilemedi', 'error');
            }
        }

        async function scanNetworks() {
            try {
                const result = await api('/api/wifi/scan');
                const target = document.getElementById('wifiScanResults');
                if (!result.networks || result.networks.length === 0) {
                    target.innerHTML = 'Ağ bulunamadı';
                } else {
                    target.innerHTML = result.networks.map(net => `<div class="list-item">${net.ssid || '<adı yok>'}<span class="badge">RSSI ${net.rssi}</span><span class="badge">${net.open ? 'AÇIK' : 'ŞİFRELİ'}</span></div>`).join('');
                }
            } catch (err) {
                showAlert('wifiAlert', err.message || 'Taramada hata', 'error');
            }
        }

        function latchRelayDisplay(state) {
            // Placeholder for future UI indicator
        }

        async function init() {
            document.getElementById('deviceId').textContent = "";
            await Promise.all([
                loadStatus(),
                loadTimerSettings(),
                loadMailSettings(),
                loadWiFiSettings()
            ]);
            setInterval(loadStatus, 3000);
        }

        init();
    </script>
</body>
</html>
)rawliteral";

namespace {
constexpr size_t JSON_CAPACITY = 4096;
constexpr size_t MAX_UPLOAD_SIZE = 2 * 1024 * 1024; // 2 MB

struct UploadContext {
    File file;
    size_t written = 0;
    String storedPath;
    String originalName;

    void reset() {
        if (file) {
            file.close();
        }
        written = 0;
        storedPath = "";
        originalName = "";
    }
};

UploadContext uploadContext;
}

void WebInterface::begin(WebServer *srv,
                         ConfigStore *storePtr,
                         CountdownScheduler *sched,
                         MailAgent *mailAgent,
                         DMFNetworkManager *netMgr,
                         const String &deviceIdentifier,
                         DNSServer *dns) {
    server = srv;
    store = storePtr;
    scheduler = sched;
    mail = mailAgent;
    network = netMgr;
    deviceId = deviceIdentifier;
    dnsServer = dns;

    server->on("/", HTTP_GET, [this]() { handleIndex(); });
    server->on("/api/status", HTTP_GET, [this]() { handleStatus(); });

    server->on("/api/timer", HTTP_GET, [this]() { handleTimerGet(); });
    server->on("/api/timer", HTTP_PUT, [this]() { handleTimerUpdate(); });
    server->on("/api/timer/start", HTTP_POST, [this]() { handleTimerStart(); });
    server->on("/api/timer/stop", HTTP_POST, [this]() { handleTimerStop(); });
    server->on("/api/timer/resume", HTTP_POST, [this]() { handleTimerResume(); });
    server->on("/api/timer/reset", HTTP_POST, [this]() { handleTimerReset(); });
    server->on("/api/timer/virtual-button", HTTP_POST, [this]() { handleVirtualButton(); });

    server->on("/api/mail", HTTP_GET, [this]() { handleMailGet(); });
    server->on("/api/mail", HTTP_PUT, [this]() { handleMailUpdate(); });
    server->on("/api/mail/test", HTTP_POST, [this]() { handleMailTest(); });

    server->on("/api/wifi", HTTP_GET, [this]() { handleWiFiGet(); });
    server->on("/api/wifi", HTTP_PUT, [this]() { handleWiFiUpdate(); });
    server->on("/api/wifi/scan", HTTP_GET, [this]() { handleWiFiScan(); });

    server->on("/api/attachments", HTTP_GET, [this]() { handleAttachmentList(); });
    server->on("/api/attachments", HTTP_DELETE, [this]() { handleAttachmentDelete(); });

    server->on("/api/logs", HTTP_GET, [this]() { handleLogs(); });

    server->on("/api/upload", HTTP_POST,
               [this]() {
                   StaticJsonDocument<256> doc;
                   if (!uploadContext.storedPath.length()) {
                       doc["status"] = "error";
                       doc["message"] = "Dosya yüklenemedi";
                   } else {
                       doc["status"] = "ok";
                       doc["path"] = uploadContext.storedPath;
                       doc["name"] = uploadContext.originalName;
                   }
                   sendJson(doc);
                   uploadContext.reset();
               },
               [this]() { handleAttachmentUpload(); });

    // WebServer başlatılmasını ertele - WiFi hazır olana kadar
    Serial.println(F("[Web] Handler'lar tanımlandı, server.begin() ertelendi"));
}

void WebInterface::startServer() {
    if (!server) return;
    
    // Kayıtlı WiFi ayarlarını kontrol et
    WiFiSettings wifiConfig = store->loadWiFiSettings();
    bool hasStoredWiFi = (wifiConfig.primarySSID.length() > 0);
    bool staConnected = false;
    
    // Eğer kayıtlı WiFi varsa, önce STA modunda bağlanmayı dene
    if (hasStoredWiFi) {
        Serial.println(F("[WiFi] Kayıtlı WiFi ayarları bulundu, bağlanılıyor..."));
        if (network && network->connectToKnown()) {
            staConnected = true;
            Serial.println(F("[WiFi] STA modunda bağlantı başarılı"));
        } else {
            Serial.println(F("[WiFi] STA bağlantısı başarısız, AP moduna geçiliyor..."));
        }
    } else {
        Serial.println(F("[WiFi] Kayıtlı WiFi yok, AP modunda başlatılıyor..."));
    }
    
    // AP modunu her zaman başlat (dual mode: hem AP hem STA)
    if (staConnected) {
        // STA + AP modu (dual mode)
        WiFi.mode(WIFI_AP_STA);
        delay(100);
    } else {
        // Sadece AP modu
        WiFi.mode(WIFI_AP);
        delay(100);
    }
    
    WiFi.softAP("SmartKraft-DMF", "12345678");
    delay(500);
    
    // Captive portal için DNS server başlat
    if (dnsServer) {
        dnsServer->start(53, "*", WiFi.softAPIP());
        Serial.println(F("[DNS] Captive portal DNS başlatıldı"));
    }
    
    server->begin();
    Serial.printf("[Web] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    if (staConnected) {
        Serial.printf("[Web] STA IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.println(F("[Web] Dual mode aktif (AP + STA)"));
    } else {
        Serial.println(F("[Web] Sadece AP modunda"));
    }
    Serial.println(F("[Web] WebServer başlatıldı"));
}

void WebInterface::loop() {
    if (!server) return;

    // Captive portal DNS işle
    if (dnsServer) {
        dnsServer->processNextRequest();
    }
    
    server->handleClient();
    if (millis() - lastStatusPush > 2000) {
        broadcastStatus();
        lastStatusPush = millis();
    }
}

void WebInterface::broadcastStatus() {
    if (!server) return;
}

void WebInterface::handleIndex() {
    server->send_P(200, "text/html", INDEX_HTML);
}

void WebInterface::handleStatus() {
    StaticJsonDocument<JSON_CAPACITY> doc;
    ScheduleSnapshot snap = scheduler->snapshot();
    doc["timerActive"] = snap.timerActive;
    doc["paused"] = scheduler->isPaused();
    doc["remainingSeconds"] = snap.remainingSeconds;
    doc["nextAlarmIndex"] = snap.nextAlarmIndex;
    doc["finalTriggered"] = snap.finalTriggered;
    doc["totalSeconds"] = scheduler->totalSeconds();
    auto alarms = doc.createNestedArray("alarms");
    for (uint8_t i = 0; i < snap.totalAlarms; ++i) {
        alarms.add(snap.alarmOffsets[i]);
    }
    doc["wifiConnected"] = network->isConnected();
    doc["ssid"] = network->currentSSID();
    doc["ip"] = network->currentIP().toString();
    doc["deviceId"] = deviceId;
    sendJson(doc);
}

void WebInterface::handleTimerGet() {
    StaticJsonDocument<JSON_CAPACITY> doc;
    auto settings = scheduler->settings();
    doc["unit"] = settings.unit == TimerSettings::DAYS ? "days" : "hours";
    doc["totalValue"] = settings.totalValue;
    doc["alarmCount"] = settings.alarmCount;
    doc["enabled"] = settings.enabled;
    sendJson(doc);
}

void WebInterface::handleTimerUpdate() {
    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"error\":\"JSON bekleniyor\"}");
        return;
    }
    StaticJsonDocument<JSON_CAPACITY> doc;
    if (deserializeJson(doc, server->arg("plain"))) {
        server->send(400, "application/json", "{\"error\":\"JSON hata\"}");
        return;
    }
    TimerSettings settings = scheduler->settings();
    String unit = doc["unit"].as<String>();
    settings.unit = unit == "hours" ? TimerSettings::HOURS : TimerSettings::DAYS;
    settings.totalValue = doc["totalValue"].as<uint16_t>();
    settings.totalValue = constrain(settings.totalValue, (uint16_t)1, (uint16_t)60);
    settings.alarmCount = doc["alarmCount"].as<uint8_t>();
    settings.alarmCount = constrain(settings.alarmCount, (uint8_t)0, (uint8_t)MAX_ALARMS);
    settings.enabled = doc["enabled"].as<bool>();

    scheduler->configure(settings);
    server->send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebInterface::handleTimerStart() {
    // Start only if timer is stopped (not running or paused)
    if (scheduler->isStopped()) {
        scheduler->start();
        server->send(200, "application/json", "{\"status\":\"started\"}");
    } else {
        server->send(400, "application/json", "{\"error\":\"Timer is already running or paused\"}");
    }
}

void WebInterface::handleTimerStop() {
    // Stop is now "pause"
    if (scheduler->isActive()) {
        scheduler->pause();
        server->send(200, "application/json", "{\"status\":\"paused\"}");
    } else {
        server->send(400, "application/json", "{\"error\":\"Timer is not running\"}");
    }
}

void WebInterface::handleTimerResume() {
    // Resume from paused state
    if (scheduler->isPaused()) {
        scheduler->resume();
        server->send(200, "application/json", "{\"status\":\"resumed\"}");
    } else {
        server->send(400, "application/json", "{\"error\":\"Timer is not paused\"}");
    }
}

void WebInterface::handleTimerReset() {
    scheduler->reset();
    server->send(200, "application/json", "{\"status\":\"reset\"}");
}

void WebInterface::handleVirtualButton() {
    // Physical button simulation: reset and start atomically
    scheduler->reset();
    scheduler->start();
    server->send(200, "application/json", "{\"status\":\"virtual-button-pressed\"}");
}

void WebInterface::handleMailGet() {
    MailSettings mailSettings = mail->currentConfig();
    StaticJsonDocument<JSON_CAPACITY> doc;
    doc["smtpServer"] = mailSettings.smtpServer;
    doc["smtpPort"] = mailSettings.smtpPort;
    doc["username"] = mailSettings.username;
    doc["warning"]["subject"] = mailSettings.warning.subject;
    doc["warning"]["body"] = mailSettings.warning.body;
    doc["warning"]["getUrl"] = mailSettings.warning.getUrl;
    doc["final"]["subject"] = mailSettings.finalContent.subject;
    doc["final"]["body"] = mailSettings.finalContent.body;
    doc["final"]["getUrl"] = mailSettings.finalContent.getUrl;
    auto recipients = doc.createNestedArray("recipients");
    for (uint8_t i = 0; i < mailSettings.recipientCount; ++i) {
        recipients.add(mailSettings.recipients[i]);
    }
    auto attachments = doc.createNestedArray("attachments");
    for (uint8_t i = 0; i < mailSettings.attachmentCount; ++i) {
        auto entry = attachments.createNestedObject();
        entry["displayName"] = mailSettings.attachments[i].displayName;
        entry["storedPath"] = mailSettings.attachments[i].storedPath;
        entry["size"] = mailSettings.attachments[i].size;
        entry["forWarning"] = mailSettings.attachments[i].forWarning;
        entry["forFinal"] = mailSettings.attachments[i].forFinal;
    }
    sendJson(doc);
}

void WebInterface::handleMailUpdate() {
    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"error\":\"JSON bekleniyor\"}");
        return;
    }
    StaticJsonDocument<JSON_CAPACITY> doc;
    if (deserializeJson(doc, server->arg("plain"))) {
        server->send(400, "application/json", "{\"error\":\"JSON hata\"}");
        return;
    }

    MailSettings mailSettings = mail->currentConfig();
    mailSettings.smtpServer = doc["smtpServer"].as<String>();
    mailSettings.smtpPort = doc["smtpPort"] | 465;
    mailSettings.username = doc["username"].as<String>();
    
    // Sadece yeni şifre girildiyse güncelle
    String newPassword = doc["password"] | "";
    if (newPassword.length() > 0) {
        mailSettings.password = newPassword;
    }

    mailSettings.warning.subject = doc["warning"]["subject"].as<String>();
    mailSettings.warning.body = doc["warning"]["body"].as<String>();
    mailSettings.warning.getUrl = doc["warning"]["getUrl"].as<String>();

    mailSettings.finalContent.subject = doc["final"]["subject"].as<String>();
    mailSettings.finalContent.body = doc["final"]["body"].as<String>();
    mailSettings.finalContent.getUrl = doc["final"]["getUrl"].as<String>();

    if (doc.containsKey("recipients") && doc["recipients"].is<JsonArray>()) {
        auto rec = doc["recipients"].as<JsonArray>();
        mailSettings.recipientCount = min((uint8_t)rec.size(), (uint8_t)MAX_RECIPIENTS);
        for (uint8_t i = 0; i < mailSettings.recipientCount; ++i) {
            mailSettings.recipients[i] = rec[i].as<String>();
        }
    }

    if (doc.containsKey("attachments") && doc["attachments"].is<JsonArray>()) {
        auto attachments = doc["attachments"].as<JsonArray>();
        mailSettings.attachmentCount = min((uint8_t)attachments.size(), (uint8_t)MAX_ATTACHMENTS);
        for (uint8_t i = 0; i < mailSettings.attachmentCount; ++i) {
            auto entry = attachments[i];
            strlcpy(mailSettings.attachments[i].displayName, entry["displayName"].as<const char*>(), MAX_FILENAME_LEN);
            strlcpy(mailSettings.attachments[i].storedPath, entry["storedPath"].as<const char*>(), MAX_PATH_LEN);
            mailSettings.attachments[i].size = entry["size"].as<uint32_t>();
            mailSettings.attachments[i].forWarning = entry["forWarning"].as<bool>();
            mailSettings.attachments[i].forFinal = entry["forFinal"].as<bool>();
        }
    }

    mail->updateConfig(mailSettings);
    server->send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebInterface::handleMailTest() {
    ScheduleSnapshot snap = scheduler->snapshot();
    String error;
    uint8_t index = snap.nextAlarmIndex < snap.totalAlarms ? snap.nextAlarmIndex : 0;
    if (mail->sendWarning(index, snap, error)) {
        server->send(200, "application/json", "{\"status\":\"mail sent\"}");
    } else {
        server->send(500, "application/json", String("{\"error\":\"") + error + "\"}");
    }
}

void WebInterface::handleWiFiGet() {
    WiFiSettings wifi = network->getConfig();
    StaticJsonDocument<JSON_CAPACITY> doc;
    doc["primarySSID"] = wifi.primarySSID;
    doc["primaryPassword"] = wifi.primaryPassword;
    doc["secondarySSID"] = wifi.secondarySSID;
    doc["secondaryPassword"] = wifi.secondaryPassword;
    doc["allowOpenNetworks"] = wifi.allowOpenNetworks;
    sendJson(doc);
}

void WebInterface::handleWiFiUpdate() {
    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"error\":\"JSON bekleniyor\"}");
        return;
    }
    StaticJsonDocument<JSON_CAPACITY> doc;
    if (deserializeJson(doc, server->arg("plain"))) {
        server->send(400, "application/json", "{\"error\":\"JSON hata\"}");
        return;
    }
    WiFiSettings wifi = network->getConfig();
    wifi.primarySSID = doc["primarySSID"].as<String>();
    wifi.primaryPassword = doc["primaryPassword"].as<String>();
    wifi.secondarySSID = doc["secondarySSID"].as<String>();
    wifi.secondaryPassword = doc["secondaryPassword"].as<String>();
    wifi.allowOpenNetworks = doc["allowOpenNetworks"].as<bool>();
    network->setConfig(wifi);
    
    // WiFi ayarları kaydedildikten sonra STA moduna geç ve bağlan
    Serial.println(F("[WiFi] Ayarlar kaydedildi, STA moduna geçiliyor..."));
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    
    if (network->ensureConnected(false)) {
        Serial.printf("[WiFi] Bağlantı başarılı: %s - IP: %s\n", 
                     WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    } else {
        Serial.println(F("[WiFi] Kayıtlı ağlara bağlanılamadı, AP modu devam ediyor"));
    }
    
    server->send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebInterface::handleWiFiScan() {
    auto list = network->scanNetworks();
    StaticJsonDocument<JSON_CAPACITY> doc;
    auto arr = doc.createNestedArray("networks");
    for (auto &net : list) {
        auto item = arr.createNestedObject();
        item["ssid"] = net.ssid;
        item["rssi"] = net.rssi;
        item["open"] = net.open;
    }
    sendJson(doc);
}

void WebInterface::handleAttachmentList() {
    MailSettings mailSettings = mail->currentConfig();
    StaticJsonDocument<JSON_CAPACITY> doc;
    auto arr = doc.createNestedArray("attachments");
    for (uint8_t i = 0; i < mailSettings.attachmentCount; ++i) {
        auto entry = arr.createNestedObject();
        entry["displayName"] = mailSettings.attachments[i].displayName;
        entry["storedPath"] = mailSettings.attachments[i].storedPath;
        entry["size"] = mailSettings.attachments[i].size;
        entry["forWarning"] = mailSettings.attachments[i].forWarning;
        entry["forFinal"] = mailSettings.attachments[i].forFinal;
    }
    sendJson(doc);
}

void WebInterface::handleAttachmentUpload() {
    HTTPUpload &upload = server->upload();
    if (upload.status == UPLOAD_FILE_START) {
        uploadContext.reset();
        
        // Klasörün var olduğundan emin ol
        if (!LittleFS.exists(store->dataFolder())) {
            LittleFS.mkdir(store->dataFolder());
            Serial.printf("[Upload] %s klasörü oluşturuldu\n", store->dataFolder().c_str());
        }
        
        String sanitized = upload.filename;
        sanitized.replace("..", "");
        sanitized.replace("/", "_");
        uploadContext.originalName = sanitized;
        String stored = store->dataFolder() + "/" + String(millis()) + "_" + sanitized;
        
        Serial.printf("[Upload] Dosya yükleniyor: %s -> %s\n", sanitized.c_str(), stored.c_str());
        
        uploadContext.file = LittleFS.open(stored, "w");
        if (!uploadContext.file) {
            Serial.println(F("[Upload] HATA: Dosya açılamadı"));
            return;
        }
        uploadContext.storedPath = stored;
        uploadContext.written = 0;
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (!uploadContext.file) return;
        if (uploadContext.written + upload.currentSize > MAX_UPLOAD_SIZE) {
            Serial.println(F("[Upload] HATA: Boyut limiti aşıldı"));
            uploadContext.file.close();
            LittleFS.remove(uploadContext.storedPath);
            uploadContext.reset();
            return;
        }
        uploadContext.file.write(upload.buf, upload.currentSize);
        uploadContext.written += upload.currentSize;
    } else if (upload.status == UPLOAD_FILE_END) {
        if (!uploadContext.file) return;
        uploadContext.file.close();
        
        Serial.printf("[Upload] Yükleme tamamlandı: %d byte\n", uploadContext.written);
        
        MailSettings mailSettings = mail->currentConfig();
        if (mailSettings.attachmentCount >= MAX_ATTACHMENTS) {
            Serial.println(F("[Upload] HATA: Maksimum ek sayısına ulaşıldı"));
            LittleFS.remove(uploadContext.storedPath);
            uploadContext.reset();
            return;
        }
        AttachmentMeta &meta = mailSettings.attachments[mailSettings.attachmentCount++];
        strlcpy(meta.displayName, uploadContext.originalName.c_str(), MAX_FILENAME_LEN);
        strlcpy(meta.storedPath, uploadContext.storedPath.c_str(), MAX_PATH_LEN);
        meta.size = uploadContext.written;
        meta.forWarning = false;
        meta.forFinal = true;
        mail->updateConfig(mailSettings);
        
        Serial.println(F("[Upload] Ek dosya listesine eklendi"));
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Serial.println(F("[Upload] Yükleme iptal edildi"));
        if (uploadContext.file) {
            uploadContext.file.close();
            LittleFS.remove(uploadContext.storedPath);
        }
        uploadContext.reset();
    }
}

void WebInterface::handleAttachmentDelete() {
    if (!server->hasArg("path")) {
        server->send(400, "application/json", "{\"error\":\"path parametresi yok\"}");
        return;
    }
    String path = server->arg("path");
    MailSettings mailSettings = mail->currentConfig();
    bool removed = false;
    for (uint8_t i = 0; i < mailSettings.attachmentCount; ++i) {
        if (path == mailSettings.attachments[i].storedPath) {
            LittleFS.remove(path);
            for (uint8_t j = i; j + 1 < mailSettings.attachmentCount; ++j) {
                mailSettings.attachments[j] = mailSettings.attachments[j + 1];
            }
            mailSettings.attachmentCount--;
            removed = true;
            break;
        }
    }
    if (removed) {
        mail->updateConfig(mailSettings);
        server->send(200, "application/json", "{\"status\":\"deleted\"}");
    } else {
        server->send(404, "application/json", "{\"error\":\"dosya bulunamadı\"}");
    }
}

void WebInterface::handleLogs() {
    StaticJsonDocument<JSON_CAPACITY> doc;
    doc["heap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;
    doc["wifiStatus"] = WiFi.status();
    sendJson(doc);
}

void WebInterface::sendJson(const JsonDocument &doc) {
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

#include "web_handlers.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>

// i18n language files
#include "i18n_en.h"
#include "i18n_de.h"
#include "i18n_tr.h"

// Embedded HTML content
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en" id="htmlRoot">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SmartKraft DMF Control Panel</title>
    <style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:monospace;background:#000;color:#fff;line-height:1.4;font-size:14px}a{color:#0f0}.container{max-width:820px;margin:0 auto;padding:16px}.header{text-align:center;margin-bottom:20px;padding-bottom:12px;border-bottom:1px solid #333}.header h1{font-size:1.8em;font-weight:normal;letter-spacing:2px}.device-id{color:#777;font-size:.9em;margin-top:4px}.status-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:12px;margin-bottom:20px;border:1px solid #333;padding:12px}.status-card{text-align:center}.status-label{color:#666;font-size:.8em;margin-bottom:4px;text-transform:uppercase}.status-value{font-size:1.2em;color:#fff;min-height:1.2em}.timer-readout{text-align:center;border:1px solid #333;padding:18px;margin-bottom:16px}.timer-readout .value{font-size:2.6em;letter-spacing:2px}.timer-readout .label{color:#777;margin-top:6px;font-size:.85em}.button-bar{display:flex;flex-wrap:wrap;gap:8px;justify-content:center;margin-bottom:24px}button{background:transparent;border:1px solid #555;color:#fff;padding:10px 18px;font-family:monospace;cursor:pointer;text-transform:uppercase;letter-spacing:1px;transition:background .2s}button:hover{background:#222}.btn-danger{border-color:#f00;color:#f00}.btn-danger:hover{background:#f00;color:#000}.btn-success{border-color:#0f0;color:#0f0}.btn-success:hover{background:#0f0;color:#000}.btn-warning{border-color:#ff0;color:#ff0}.btn-warning:hover{background:#ff0;color:#000}.tabs{display:flex;flex-wrap:wrap;border-bottom:1px solid #333;margin-bottom:8px}.tab{flex:1;min-width:140px;border:1px solid #333;border-bottom:none;background:#000;color:#666;padding:10px;cursor:pointer;text-align:center;font-size:.9em}.tab+.tab{margin-left:4px}.tab.active{color:#fff;border-color:#fff}.tab-content{border:1px solid #333;padding:20px}.tab-pane{display:none}.tab-pane.active{display:block}.form-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:12px}.form-group{display:flex;flex-direction:column;gap:6px;margin-bottom:16px}label{font-size:.85em;color:#ccc;text-transform:uppercase;letter-spacing:1px}input[type="text"],input[type="number"],input[type="password"],input[type="email"],textarea,select{width:100%;padding:10px;background:#000;border:1px solid #333;color:#fff;font-family:monospace}textarea{resize:vertical;min-height:100px}.checkbox{display:flex;align-items:center;gap:8px;font-size:.9em;color:#ccc}.section-title{border-bottom:1px solid #333;padding-bottom:6px;margin-top:8px;margin-bottom:12px;font-size:1em;letter-spacing:1px;text-transform:uppercase}.attachments{border:1px solid #333;padding:12px;margin-bottom:16px}.attachments table{width:100%;border-collapse:collapse;font-size:.85em}.attachments th,.attachments td{border-bottom:1px solid #222;padding:6px;text-align:left}.attachments th{color:#888;text-transform:uppercase;letter-spacing:1px}.file-upload{border:1px dashed #555;padding:20px;text-align:center;margin-bottom:12px;cursor:pointer}.file-upload:hover{background:#111}.alert{display:none;margin-bottom:12px;padding:10px;border:1px solid #333;font-size:.85em}.alert.success{border-color:#0f0;color:#0f0}.alert.error{border-color:#f00;color:#f00}.list{border:1px solid #333;padding:10px;max-height:180px;overflow-y:auto;font-size:.85em}.list-item{border-bottom:1px solid #222;padding:6px 0;display:flex;justify-content:space-between;align-items:center}.list-item:last-child{border-bottom:none}.badge{display:inline-block;padding:2px 6px;font-size:.75em;border:1px solid #333;margin-left:6px}.connection-indicator{position:fixed;top:12px;right:12px;border:1px solid #333;padding:6px 10px;font-size:.8em;z-index:20;background:#000;max-width:280px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.connection-indicator.online{border-color:#0f0;color:#0f0}.connection-indicator.offline{border-color:#f00;color:#f00}.lang-selector{position:fixed;top:12px;left:12px;z-index:21;background:#000;border:1px solid #333;padding:6px;display:flex;gap:4px}.lang-btn{background:transparent;border:1px solid #555;color:#888;padding:4px 10px;font-family:monospace;cursor:pointer;font-size:.75em;letter-spacing:1px;transition:all .2s;min-width:40px}.lang-btn:hover{background:#222;border-color:#0f0;color:#0f0}.lang-btn.active{border-color:#0f0;color:#0f0;font-weight:bold}.accordion{border:1px solid #333;margin-bottom:12px}.accordion-header{background:#111;border-bottom:1px solid #333;padding:12px 16px;cursor:pointer;display:flex;justify-content:space-between;align-items:center;text-transform:uppercase;letter-spacing:1px;font-size:.9em;transition:background .2s}.accordion-header:hover{background:#1a1a1a}.accordion-header.active{background:#0a0a0a;color:#0f0}.accordion-toggle{font-size:1.2em;transition:transform .3s}.accordion-header.active .accordion-toggle{transform:rotate(180deg);color:#0f0}.accordion-content{max-height:0;overflow:hidden;transition:max-height .3s ease;background:#0a0a0a}.accordion-content.active{max-height:2000px;padding:16px;border-top:1px solid #0f0}.preset-btn{display:inline-block;padding:8px 16px;margin:4px;border:1px solid #555;background:#111;color:#ccc;cursor:pointer;text-align:center;font-size:.85em;transition:all .2s}.preset-btn:hover{background:#222;border-color:#0f0}.preset-btn.active{border-color:#0f0;background:#0f0;color:#000}@media (max-width:600px){.lang-selector{top:8px;left:8px;font-size:.7em;padding:4px;gap:2px}.lang-btn{padding:2px 6px;min-width:32px;font-size:.65em}.connection-indicator{top:48px;right:8px;left:8px;max-width:none;font-size:.7em;padding:4px 8px}.tabs{flex-direction:column}.tab+.tab{margin-left:0;margin-top:4px}button{width:100%}}</style>
</head>
<body>
    <div id="mainApp" style="display:block;">
    <div class="lang-selector">
        <button class="lang-btn active" data-lang="en">EN</button>
        <button class="lang-btn" data-lang="de">DE</button>
        <button class="lang-btn" data-lang="tr">TR</button>
    </div>
    <div id="connectionStatus" class="connection-indicator offline" data-i18n="status.connecting">Checking connection...</div>
    <div class="container">
        <div class="header">
            <h1 data-i18n="header.title">SMARTKRAFT DMF</h1>
            <div class="device-id"><span data-i18n="header.deviceId">Device ID</span>: <span id="deviceId">-</span></div>
        </div>

        <div class="status-grid">
            <div class="status-card">
                <div class="status-label" data-i18n="status.timerStatus">Timer Status</div>
                <div class="status-value" id="timerStatus">-</div>
            </div>
            <div class="status-card">
                <div class="status-label" data-i18n="status.remainingTime">Remaining Time</div>
                <div class="status-value" id="remainingTime">-</div>
            </div>
            <div class="status-card">
                <div class="status-label" data-i18n="status.nextAlarm">Next Alarm</div>
                <div class="status-value" id="nextAlarm">-</div>
            </div>
            <div class="status-card">
                <div class="status-label">Wi-Fi</div>
                <div class="status-value" id="wifiStatus">-</div>
            </div>
        </div>

        <div class="timer-readout">
            <div class="value" id="timerDisplay">00:00:00</div>
            <div class="label" data-i18n="status.countdown">Countdown</div>
        </div>

        <div class="button-bar">
            <button id="btnStart" class="btn-success" onclick="startTimer()" data-i18n="buttons.start">Start</button>
            <button id="btnPause" class="btn-warning" onclick="pauseTimer()" style="display:none;" data-i18n="buttons.pause">Pause</button>
            <button id="btnResume" class="btn-success" onclick="resumeTimer()" style="display:none;" data-i18n="buttons.resume">Resume</button>
            <button id="btnReset" class="btn-danger" onclick="resetTimer()" data-i18n="buttons.reset">Reset</button>
            <button id="btnPhysical" class="btn-danger" onclick="virtualButton()" data-i18n="buttons.physicalButton">Physical Button</button>
        </div>

        <div class="tabs">
            <div class="tab active" data-tab="alarmTab" data-i18n="tabs.alarm">Alarm Settings</div>
            <div class="tab" data-tab="mailTab" data-i18n="tabs.mail">Mail Settings</div>
            <div class="tab" data-tab="wifiTab" data-i18n="tabs.wifi">Connection Settings</div>
            <div class="tab" data-tab="infoTab" data-i18n="tabs.info">Info</div>
        </div>

        <div class="tab-content">
            <div id="alarmTab" class="tab-pane active">
                <div id="alarmAlert" class="alert"></div>
                <div class="section-title" data-i18n="alarm.sectionCountdown">Countdown Parameters</div>
                <div class="form-grid">
                    <div class="form-group">
                        <label data-i18n="alarm.unitLabel">Time Unit</label>
                        <select id="timerUnit">
                            <option value="minutes" data-i18n="alarm.unitMinutes">Minutes</option>
                            <option value="hours" data-i18n="alarm.unitHours">Hours</option>
                            <option value="days" data-i18n="alarm.unitDays">Days</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label data-i18n="alarm.totalLabel">Total Duration (1-60)</label>
                        <input type="number" id="timerTotal" min="1" max="60" value="7">
                    </div>
                    <div class="form-group">
                        <label data-i18n="alarm.alarmsLabel">Number of Alarms (0-10)</label>
                        <input type="number" id="timerAlarms" min="0" max="10" value="3">
                    </div>
                </div>
                <button onclick="saveTimerSettings()" data-i18n="buttons.save">Save</button>

                <div class="section-title" data-i18n="alarm.sectionAlarms">Alarm Schedule</div>
                <div class="list" id="alarmSchedule">-</div>
            </div>

            <div id="mailTab" class="tab-pane">
                <div id="mailAlert" class="alert"></div>

                <!-- MAİL ENTEGRASYONU -->
                <div class="accordion">
                    <div class="accordion-header active" onclick="toggleAccordion(this)">
                        <span data-i18n="mail.sectionSMTP">SMTP Settings</span>
                        <span class="accordion-toggle">v</span>
                    </div>
                    <div class="accordion-content active">
                        <div style="display:grid; grid-template-columns:1fr 1fr; gap:12px; margin-bottom:16px;">
                            <div class="preset-btn active" id="presetProton" onclick="selectMailPreset('proton')">
                                <div style="font-weight:bold; margin-bottom:4px;">Proton Mail</div>
                                <div style="font-size:0.75em; color:#888;">smtp.protonmail.ch</div>
                            </div>
                            <div class="preset-btn" id="presetGmail" onclick="selectMailPreset('gmail')">
                                <div style="font-weight:bold; margin-bottom:4px;">Gmail</div>
                                <div style="font-size:0.75em; color:#888;">smtp.gmail.com</div>
                            </div>
                        </div>
                        <div class="form-grid">
                            <div class="form-group">
                                <label data-i18n="mail.server">SMTP Server</label>
                                <input type="text" id="smtpServer" data-i18n="mail.serverPlaceholder" placeholder="e.g., smtp.gmail.com">
                            </div>
                            <div class="form-group">
                                <label data-i18n="mail.port">Port</label>
                                <input type="number" id="smtpPort" value="465">
                            </div>
                            <div class="form-group">
                                <label data-i18n="mail.username">Username</label>
                                <input type="email" id="smtpUsername" data-i18n="mail.usernamePlaceholder" placeholder="e.g., user@example.com">
                            </div>
                            <div class="form-group">
                                <label id="passwordLabel" data-i18n="mail.password">Password</label>
                                <input type="password" id="smtpPassword" data-i18n="mail.passwordPlaceholder" placeholder="SMTP password or app-specific password">
                            </div>
                        </div>
                        <div style="font-size:0.7em; color:#666; font-style:italic; margin-top:8px;">
                            <span id="passwordHelp" data-i18n="mail.portHelp">Only port 465 (SSL/TLS) is supported</span>
                        </div>
                    </div>
                </div>

                <!-- ERKEN UYARI SİSTEMİ -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span data-i18n="mail.sectionWarning">Early Warning Message</span>
                        <span class="accordion-toggle">v</span>
                    </div>
                    <div class="accordion-content">
                        <div class="form-group">
                            <label data-i18n="mail.warningSubject">Subject</label>
                            <input type="text" id="warningSubject" data-i18n="mail.warningSubjectPlaceholder" placeholder="Early Warning from SmartKraft DMF">
                        </div>
                        <div class="form-group">
                            <label data-i18n="mail.warningBody">Message Body</label>
                            <textarea id="warningBody" data-i18n="mail.warningBodyPlaceholder" placeholder="Warning message content...">Device: {DEVICE_ID}
Time: {TIMESTAMP}
Remaining: {REMAINING}

This is a SmartKraft DMF early warning message.</textarea>
                        </div>
                        <div style="font-size:0.7em; color:#666; margin-bottom:12px;">
                            <span data-i18n="mail.placeholders">Use {DEVICE_ID}, {TIMESTAMP}, {REMAINING}, %ALARM_INDEX%, %TOTAL_ALARMS%, %REMAINING%</span>
                        </div>
                        <div class="form-group">
                            <label data-i18n="mail.warningUrl">Trigger URL (GET)</label>
                            <input type="text" id="warningUrl" data-i18n="mail.warningUrlPlaceholder" placeholder="https://example.com/api/warning">
                        </div>
                        <button id="btnTestWarning" class="btn-warning" style="width:100%;" data-i18n="mail.testWarning">Test Warning Mail</button>
                    </div>
                </div>

                <!-- DMF PROTOKOLÜ DÜZENLE -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span data-i18n="mail.sectionFinal">Final Message (DMF Protocol)</span>
                        <span class="accordion-toggle">v</span>
                    </div>
                    <div class="accordion-content">
                        <div class="form-group">
                            <label data-i18n="mail.sectionRecipients">Recipients</label>
                            <textarea id="mailRecipients" data-i18n="mail.recipientsPlaceholder" placeholder="recipient1@example.com&#10;recipient2@example.com" style="min-height:80px;"></textarea>
                        </div>
                        <div style="font-size:0.7em; color:#666; margin-bottom:12px;">
                            <span data-i18n="mail.recipientsHelp">Enter email addresses (one per line)</span>
                        </div>
                        <div class="form-group">
                            <label data-i18n="mail.finalSubject">Subject</label>
                            <input type="text" id="finalSubject" data-i18n="mail.finalSubjectPlaceholder" placeholder="Final Notice from SmartKraft DMF">
                        </div>
                        <div class="form-group">
                            <label data-i18n="mail.finalBody">Message Body</label>
                            <textarea id="finalBody" data-i18n="mail.finalBodyPlaceholder" placeholder="Final message content..." style="min-height:120px;">[!] DMF PROTOCOL ACTIVE [!]

Device: {DEVICE_ID}
Time: {TIMESTAMP}

Timer completed. Urgent action required.</textarea>
                        </div>
                        <div style="font-size:0.7em; color:#666; margin-bottom:12px;">
                            <span data-i18n="mail.placeholders">Use {DEVICE_ID}, {TIMESTAMP}, {REMAINING}, %ALARM_INDEX%, %TOTAL_ALARMS%, %REMAINING%</span>
                        </div>
                        <div class="form-group">
                            <label data-i18n="mail.finalUrl">Trigger URL (GET)</label>
                            <input type="text" id="finalUrl" data-i18n="mail.finalUrlPlaceholder" placeholder="https://example.com/api/final">
                        </div>
                        <div style="border:1px dashed #555; padding:16px; margin:16px 0; text-align:center; cursor:pointer;" onclick="document.getElementById('fileInput').click()">
                            <div style="color:#888; font-size:0.8em; margin-bottom:8px;" data-i18n="mail.uploadZone">📎 Click to upload file (max 500 KB)</div>
                        </div>
                        <input type="file" id="fileInput" style="display:none" onchange="uploadAttachment(event)">
                        <div class="attachments" style="max-height:200px; overflow-y:auto;">
                            <div class="section-title" data-i18n="mail.sectionAttachments">Attachments</div>
                            <table style="width:100%; font-size:0.75em;">
                                <thead>
                                    <tr style="border-bottom:1px solid #333;">
                                        <th style="padding:4px;" data-i18n="mail.attachmentName">File Name</th>
                                        <th style="padding:4px;" data-i18n="mail.attachmentSize">Size</th>
                                        <th style="padding:4px; text-align:center;" data-i18n="mail.attachmentWarning">Warning</th>
                                        <th style="padding:4px; text-align:center;" data-i18n="mail.attachmentFinal">Final</th>
                                        <th style="padding:4px;" data-i18n="mail.attachmentActions">Actions</th>
                                    </tr>
                                </thead>
                                <tbody id="attachmentRows"></tbody>
                            </table>
                        </div>
                        <button id="btnTestFinal" class="btn-danger" style="width:100%; margin-top:12px;" data-i18n="mail.testFinal">Test Final Mail</button>
                    </div>
                </div>

                <div style="margin-top:20px;">
                    <button id="btnSaveMail" style="width:100%;" data-i18n="buttons.save">Save</button>
                </div>
            </div>

            <div id="wifiTab" class="tab-pane">
                <div id="wifiAlert" class="alert"></div>
                <div class="section-title" data-i18n="wifi.sectionAP">Access Point Settings</div>
                <div class="form-group checkbox" style="margin-bottom:24px;">
                    <input type="checkbox" id="apModeEnabled" checked>
                    <label for="apModeEnabled" data-i18n="wifi.modeAP">Access Point (AP) Mode</label>
                </div>
                <div style="border-top:1px solid #333; padding-top:18px; margin-top:4px;">
                    <div class="section-title" style="margin-top:0;" data-i18n="wifi.sectionSTA">Station Mode Settings</div>
                    <div class="form-grid">
                        <div class="form-group">
                            <label data-i18n="wifi.staSSID">Target Network (SSID)</label>
                            <input type="text" id="wifiPrimarySsid" data-i18n="wifi.staSSIDPlaceholder" placeholder="Your WiFi network name">
                        </div>
                        <div class="form-group">
                            <label data-i18n="wifi.staPassword">Password</label>
                            <input type="password" id="wifiPrimaryPassword" data-i18n="wifi.staPasswordPlaceholder" placeholder="Network password">
                        </div>
                        <div class="form-group checkbox">
                            <input type="checkbox" id="primaryStaticEnabled">
                            <label for="primaryStaticEnabled" data-i18n="wifi.staDHCP">Use DHCP (Automatic IP)</label>
                        </div>
                        <div class="form-group">
                            <label data-i18n="wifi.staIP">IP Address</label>
                            <input type="text" id="primaryIP" placeholder="192.168.1.100">
                        </div>
                        <div class="form-group">
                            <label data-i18n="wifi.staGateway">Gateway</label>
                            <input type="text" id="primaryGateway" placeholder="192.168.1.1">
                        </div>
                        <div class="form-group">
                            <label data-i18n="wifi.staSubnet">Subnet Mask</label>
                            <input type="text" id="primarySubnet" placeholder="255.255.255.0">
                        </div>
                        <div class="form-group">
                            <label data-i18n="wifi.staDNS">Primary DNS</label>
                            <input type="text" id="primaryDNS" placeholder="192.168.1.1">
                        </div>
                    </div>
                </div>
                <div style="border-top:1px dashed #333; margin:28px 0 0; padding-top:18px;">
                    <div class="section-title" style="margin-top:0;">Backup WiFi Network</div>
                    <div class="form-grid">
                        <div class="form-group">
                            <label data-i18n="wifi.staSSID">Target Network (SSID)</label>
                            <input type="text" id="wifiSecondarySsid" data-i18n="wifi.staSSIDPlaceholder" placeholder="Your WiFi network name">
                        </div>
                        <div class="form-group">
                            <label data-i18n="wifi.staPassword">Password</label>
                            <input type="password" id="wifiSecondaryPassword" data-i18n="wifi.staPasswordPlaceholder" placeholder="Network password">
                        </div>
                        <div class="form-group checkbox">
                            <input type="checkbox" id="secondaryStaticEnabled">
                            <label for="secondaryStaticEnabled" data-i18n="wifi.staDHCP">Use DHCP (Automatic IP)</label>
                        </div>
                        <div class="form-group">
                            <label data-i18n="wifi.staIP">IP Address</label>
                            <input type="text" id="secondaryIP" placeholder="192.168.1.101">
                        </div>
                        <div class="form-group">
                            <label data-i18n="wifi.staGateway">Gateway</label>
                            <input type="text" id="secondaryGateway" placeholder="192.168.1.1">
                        </div>
                        <div class="form-group">
                            <label data-i18n="wifi.staSubnet">Subnet Mask</label>
                            <input type="text" id="secondarySubnet" placeholder="255.255.255.0">
                        </div>
                        <div class="form-group">
                            <label data-i18n="wifi.staDNS">Primary DNS</label>
                            <input type="text" id="secondaryDNS" placeholder="192.168.1.1">
                        </div>
                    </div>
                </div>
                <div style="border-top:1px solid #333; margin:28px 0 0; padding-top:24px;">
                    <div style="display:flex; align-items:center; gap:12px; margin-bottom:16px;">
                        <div style="width:28px; height:28px; border:2px solid #ffa500; border-radius:4px; display:flex; align-items:center; justify-content:center; font-weight:900; font-size:18px; color:#ffa500; flex-shrink:0;">!</div>
                        <div style="font-size:1.1em; font-weight:700; letter-spacing:1px; color:#fff;" data-i18n="wifi.emergencyTitle">EMERGENCY INTERNET CONNECTION</div>
                    </div>
                    <div style="display:flex; align-items:center; gap:10px; margin:0 0 18px 40px;">
                        <input type="checkbox" id="wifiAllowOpen" style="width:18px; height:18px; cursor:pointer;">
                        <label for="wifiAllowOpen" style="font-size:0.9em; font-weight:600; color:#ccc; letter-spacing:0.5px; margin:0; cursor:pointer; user-select:none;" data-i18n="wifi.emergencyCheckbox">ALLOW UNSECURED NETWORKS</label>
                    </div>
                    <div style="border:1px solid #444; padding:18px; background:#0a0a0a; font-size:0.85em; line-height:1.7; color:#bbb; margin-left:40px;">
                        <div style="margin-bottom:14px;">
                            <strong style="color:#fff; font-size:0.95em;" data-i18n="wifi.emergencyWhen">When does it work?</strong>
                            <ul style="margin:8px 0 0 20px; padding:0;">
                                <li style="margin:4px 0;" data-i18n="wifi.emergencyWhen1">AP mode is off</li>
                                <li style="margin:4px 0;" data-i18n="wifi.emergencyWhen2">Primary and backup networks cannot be connected (or there is no internet access)</li>
                            </ul>
                        </div>
                        <div style="margin-bottom:14px;">
                            <strong style="color:#fff; font-size:0.95em;" data-i18n="wifi.emergencyHow">How does it work?</strong>
                            <div style="margin-top:8px; white-space:pre-line;" data-i18n="wifi.emergencyHowText">The device scans nearby unsecured (open) WiFi networks and temporarily connects to send emails by checking internet access. If WiFi is connected but there is no internet, it automatically switches to another network.</div>
                        </div>
                        <div style="margin-bottom:10px;">
                            <span style="color:#ff4444; font-weight:700;" data-i18n="wifi.emergencyProtocol">DMF Data Protocol:</span>
                            <span style="margin-left:6px;" data-i18n="wifi.emergencyProtocolText">Email is NEVER lost! It tries indefinitely until internet access is available.</span>
                        </div>
                        <div style="font-style:italic; font-size:0.9em; color:#888; border-top:1px solid #222; padding-top:12px; margin-top:12px; white-space:pre-line;" data-i18n="wifi.emergencyNote">Note: Since it poses a security risk, it is recommended to use only in critical situations. Mail connection is TLS/SSL encrypted.</div>
                    </div>
                </div>
                <div class="button-bar" style="justify-content:flex-start; margin-top:30px;">
                    <button onclick="saveWiFiSettings()" data-i18n="buttons.save">Save</button>
                    <button class="btn-warning" onclick="scanNetworks()" data-i18n="buttons.scan">Scan</button>
                    <button class="btn-danger" onclick="factoryReset()" data-i18n="buttons.factoryReset">Factory Reset</button>
                    <button onclick="rebootDevice()" data-i18n="buttons.reboot">Reboot</button>
                </div>
                <div class="section-title" style="margin-top:34px;">Bulunan Ağlar</div>
                <div class="list" id="wifiScanResults">-</div>
            </div>

            <div id="infoTab" class="tab-pane">
                <div class="section-title" style="margin-top:0;" data-i18n="info.title">What is SmartKraft DMF?</div>
                <div style="font-size:0.9em; line-height:1.6; color:#ccc; margin-bottom:20px;" data-i18n="info.description">
                    SmartKraft DMF (Delayed Message Framework) is an intelligent countdown and alarm device designed for critical timing and notification systems. It provides automatic email delivery, emergency connection management and relay control within the time you specify.
                </div>

                <div class="section-title" data-i18n="info.howToUse">How to Use?</div>
                <div style="font-size:0.85em; line-height:1.6; color:#ccc; margin-bottom:20px; white-space:pre-line;">
<strong data-i18n="info.step1Title">1. Alarm Settings</strong>
<span data-i18n="info.step1Text">• Set countdown duration (days or hours)
• Set alarm count (0-10)
• Enable and start the timer</span>

<strong data-i18n="info.step2Title">2. Mail Settings</strong>
<span data-i18n="info.step2Text">• Enter SMTP server information (ProtonMail recommended)
• Add email recipients (maximum 10)
• Customize Warning and Final email content
• Upload optional file attachments (maximum 5 files, 2MB limit)</span>

<strong data-i18n="info.step3Title">3. Connection Settings</strong>
<span data-i18n="info.step3Text">• Define your primary and backup WiFi networks
• Configure static IP if needed
• Consider allowing connection to unsecured networks for emergency
• Set browser access password (minimum 6 characters)</span>

<strong data-i18n="info.step4Title">4. Physical Button</strong>
<span data-i18n="info.step4Text">• Press the button on the device to reset the countdown
• Perform the same function with 'Physical Button' from the web interface</span>

<strong data-i18n="info.step5Title">5. DMF Protocol</strong>
<span data-i18n="info.step5Text">• Relay triggers when time expires (fail-safe mechanism)
• Final email retries until internet access is achieved
• Automatic switch to open WiFi networks (depending on settings)</span>
                </div>

                <div class="section-title" data-i18n="info.securityTitle">Security Notes</div>
                <div style="font-size:0.85em; line-height:1.6; color:#ccc; margin-bottom:20px;" data-i18n="info.securityText">
                    • All sensitive settings (mail, WiFi, attachments) are protected by browser password
                    • Session duration is 30 minutes; automatically requests re-login
                    • Factory Reset permanently deletes all settings and files
                    • Mail connections are encrypted with TLS/SSL
                </div>

                <div class="section-title" data-i18n="info.technicalTitle">Technical Specifications</div>
                <div style="font-size:0.85em; line-height:1.6; color:#ccc; margin-bottom:20px;" data-i18n="info.technicalText">
                    • Processor: ESP32-C6 (RISC-V, WiFi 6 support)
                    • Memory: LittleFS file system
                    • Connectivity: Dual WiFi AP+STA mode
                    • Power: USB-C (5V)
                    • Output: Relay control pin
                </div>

                <div class="section-title" data-i18n="info.supportTitle">Support and Documentation</div>
                <div style="font-size:0.9em; line-height:1.8; color:#ccc; margin-bottom:20px;">
                    <span data-i18n="info.supportText">For detailed user manual, example scenarios and updates:</span><br>
                    <a href="https://smartkraft.ch/DMF" target="_blank" style="color:#0f0; text-decoration:none; border:1px solid #0f0; padding:8px 16px; display:inline-block; margin-top:12px; text-transform:uppercase; letter-spacing:1px;" data-i18n="info.supportLink">SmartKraft.ch/DMF</a>
                </div>

                <div style="border-top:1px solid #333; padding-top:16px; margin-top:30px; text-align:center; font-size:0.75em; color:#666;" data-i18n="info.footer">
                    SmartKraft DMF v1.0 • Open Source Hardware/Software
                    © 2025 SmartKraft Systems
                </div>
            </div>
        </div>
    </div>

    <script>
        // i18n System
        let i18nData = {};
        let currentLang = localStorage.getItem('lang') || 'en';

        async function loadLanguage(lang) {
            try {
                const response = await fetch(`/api/i18n?lang=${lang}`);
                i18nData = await response.json();
                currentLang = lang;
                localStorage.setItem('lang', lang);
                document.getElementById('htmlRoot').setAttribute('lang', lang);
                applyTranslations();
                updateLangButtons();
            } catch (error) {
                console.error('Failed to load language:', error);
            }
        }

        function applyTranslations() {
            document.querySelectorAll('[data-i18n]').forEach(el => {
                const key = el.getAttribute('data-i18n');
                const translation = getTranslation(key);
                if (translation) {
                    if (el.tagName === 'INPUT' && el.type !== 'checkbox' && el.type !== 'radio') {
                        el.placeholder = translation;
                    } else {
                        // white-space:pre-line olan elementlerde \n korunur
                        // Diğerlerinde \n → <br> dönüşümü
                        const style = window.getComputedStyle(el);
                        if (style.whiteSpace === 'pre-line' || style.whiteSpace === 'pre-wrap') {
                            el.textContent = translation;
                        } else {
                            el.innerHTML = translation.replace(/\n/g, '<br>');
                        }
                    }
                }
            });
            
            // Update dynamic content
            updateStatusDisplay();
        }

        function getTranslation(key) {
            const keys = key.split('.');
            let value = i18nData;
            for (const k of keys) {
                if (value && typeof value === 'object') {
                    value = value[k];
                } else {
                    return null;
                }
            }
            return value;
        }

        function switchLanguage(lang) {
            loadLanguage(lang);
        }

        function updateLangButtons() {
            document.querySelectorAll('.lang-btn').forEach(btn => {
                btn.classList.toggle('active', btn.getAttribute('data-lang') === currentLang);
            });
        }

        function t(key) {
            return getTranslation(key) || key;
        }
        
        function updateStatusDisplay() {
            const s = state.status;
            if (!s) return;
            
            const isPaused = s.paused;
            const isRunning = s.timerActive && !s.paused;
            const isStopped = !s.timerActive;
            
            let statusText = t('timerStates.idle');
            if (isPaused) statusText = t('timerStates.paused');
            else if (isRunning) statusText = t('timerStates.running');
            else if (s.timerActive) statusText = t('timerStates.completed');
            
            const timerStatusEl = document.getElementById('timerStatus');
            if (timerStatusEl) timerStatusEl.textContent = statusText;
        }

        // Initialize i18n and app on page load
        document.addEventListener('DOMContentLoaded', () => {
            console.log('[INIT] DOM loaded, starting app...');
            // i18n'yi paralel yükle, init()'i bloke etme
            loadLanguage(currentLang);
            init();
        });

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
            
            // Timeout ekle (15 saniye)
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), 15000);
            options.signal = controller.signal;
            
            try {
                const response = await fetch(path, options);
                clearTimeout(timeoutId);
                
                if (!response.ok) {
                    const text = await response.text();
                    throw new Error(text || response.statusText);
                }
                if (response.status === 204) return null;
                const text = await response.text();
                try { return JSON.parse(text); } catch { return text; }
            } catch (error) {
                clearTimeout(timeoutId);
                if (error.name === 'AbortError') {
                    throw new Error(t('errors.timeout'));
                }
                throw error;
            }
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
            console.log('[TAB] Opening:', id);
            const tabs = document.querySelectorAll('.tab');
            const panes = document.querySelectorAll('.tab-pane');
            
            console.log('[TAB] Found', tabs.length, 'tabs and', panes.length, 'panes');
            
            tabs.forEach(tab => tab.classList.remove('active'));
            panes.forEach(pane => pane.classList.remove('active'));
            
            if (event && event.currentTarget) {
                event.currentTarget.classList.add('active');
            }
            
            const targetPane = document.getElementById(id);
            if (targetPane) {
                targetPane.classList.add('active');
                console.log('[TAB] Activated pane:', id);
            } else {
                console.error('[TAB] Pane not found:', id);
            }
        }

        function toggleAccordion(header) {
            header.classList.toggle('active');
            const content = header.nextElementSibling;
            content.classList.toggle('active');
        }

        function selectMailPreset(type, updateFields = true) {
            const presets = document.querySelectorAll('.preset-btn');
            presets.forEach(btn => btn.classList.remove('active'));
            
            const passwordLabel = document.getElementById('passwordLabel');
            const passwordHelp = document.getElementById('passwordHelp');
            
            if(type === 'proton') {
                if (presets.length > 0) presets[0].classList.add('active');
                if (passwordLabel) passwordLabel.textContent = 'Proton Mail App Password (16 haneli)';
                if (passwordHelp) passwordHelp.innerHTML = 'Proton Mail > Settings > Account > Security > Two-factor authentication > Create app password';
                if (updateFields) {
                    document.getElementById('smtpServer').value = 'smtp.protonmail.ch';
                    document.getElementById('smtpPort').value = '465'; // SSL/TLS port
                }
            } else if(type === 'gmail') {
                if (presets.length > 1) presets[1].classList.add('active');
                if (passwordLabel) passwordLabel.textContent = 'Gmail App Password (16 haneli)';
                if (passwordHelp) passwordHelp.innerHTML = 'Gmail > Google Account > Security > 2-Step Verification > App passwords > Create app password';
                if (updateFields) {
                    document.getElementById('smtpServer').value = 'smtp.gmail.com';
                    document.getElementById('smtpPort').value = '465'; // SSL/TLS port
                }
            }
        }

        async function sendWarningTest() {
            console.log('[MAIL TEST] Warning test başlatıldı...');
            try {
                const result = await api('/api/mail/test', {
                    method: 'POST',
                    body: { testType: 'warning' }
                });
                
                console.log('[MAIL TEST] Warning başarılı:', result);
                showAlert('mailAlert', t('mail.testSuccess'), 'success');
            } catch (e) {
                console.error('[MAIL TEST] Warning exception:', e);
                showAlert('mailAlert', t('mail.testError') + ': ' + e.message, 'error');
            }
        }

        async function sendFinalTest() {
            if (!confirm(t('mail.testFinalConfirm'))) {
                return;
            }
            
            console.log('[MAIL TEST] Final test başlatıldı...');
            try {
                const result = await api('/api/mail/test', {
                    method: 'POST',
                    body: { testType: 'dmf' }
                });
                
                console.log('[MAIL TEST] Final başarılı:', result);
                showAlert('mailAlert', t('mail.testSuccess'), 'success');
            } catch (e) {
                console.error('[MAIL TEST] Final exception:', e);
                showAlert('mailAlert', t('mail.testError') + ': ' + e.message, 'error');
            }
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
            const deviceIdEl = document.getElementById('deviceId');
            
            if (s.deviceId && deviceIdEl) {
                deviceIdEl.textContent = s.deviceId;
            }
            
            if (connection) {
                if (s.wifiConnected) {
                    const flags = [];
                    if (s.apModeEnabled) flags.push('AP');
                    if (s.allowOpenNetworks) flags.push('OPEN-FALLBACK');
                    connection.textContent = `Wi-Fi: ${s.ssid || '-'} (${s.ip || '-'}) ${flags.length? '['+flags.join(',')+']':''}`;
                    connection.classList.add('online');
                    connection.classList.remove('offline');
                } else {
                    connection.textContent = `Wi-Fi: ${t('status.offline')}`;
                    connection.classList.remove('online');
                    connection.classList.add('offline');
                }
            }

            // Update button visibility based on timer state
            const btnStart = document.getElementById('btnStart');
            const btnPause = document.getElementById('btnPause');
            const btnResume = document.getElementById('btnResume');
            
            if (btnStart && btnPause && btnResume) {
                const isStopped = !s.timerActive;
                const isPaused = s.paused;
                const isRunning = s.timerActive && !s.paused;

                btnStart.style.display = isStopped ? 'inline-block' : 'none';
                btnPause.style.display = isRunning ? 'inline-block' : 'none';
                btnResume.style.display = isPaused ? 'inline-block' : 'none';
            }

            updateStatusDisplay();
            
            const remainingTimeEl = document.getElementById('remainingTime');
            const timerDisplayEl = document.getElementById('timerDisplay');
            const nextAlarmEl = document.getElementById('nextAlarm');
            const wifiStatusEl = document.getElementById('wifiStatus');
            
            if (remainingTimeEl) remainingTimeEl.textContent = formatDuration(s.remainingSeconds || 0);
            if (timerDisplayEl) timerDisplayEl.textContent = formatDuration(s.remainingSeconds || 0);

            if (nextAlarmEl) {
                if (s.alarms && s.alarms.length > s.nextAlarmIndex) {
                    const nextOffset = s.alarms[s.nextAlarmIndex];
                    const total = s.totalSeconds || 0;
                    const elapsed = total - (s.remainingSeconds || 0);
                    const remainingToNext = Math.max(nextOffset - elapsed, 0);
                    nextAlarmEl.textContent = formatDuration(remainingToNext);
                } else {
                    nextAlarmEl.textContent = '-';
                }
            }

            if (wifiStatusEl) {
                const wifiStatus = s.wifiConnected ? `${s.ssid || 'N/A'} (${s.ip || '-'})` : t('status.offline');
                wifiStatusEl.textContent = wifiStatus;
            }

            const scheduleEl = document.getElementById('alarmSchedule');
            if (scheduleEl) {
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
        }

        let connectionRetryCount = 0;
        const MAX_RETRIES = 3;

        async function loadStatus() {
            try {
                const controller = new AbortController();
                const timeoutId = setTimeout(() => controller.abort(), 10000); // 10 saniye timeout
                
                const response = await fetch('/api/status', {
                    signal: controller.signal,
                    headers: { 'Cache-Control': 'no-cache' }
                });
                clearTimeout(timeoutId);
                
                if (!response.ok) throw new Error('HTTP ' + response.status);
                
                state.status = await response.json();
                updateStatusView();
                connectionRetryCount = 0; // Başarılı, retry sayacını sıfırla
                
                // Bağlantı başarılı - status göster
                const connection = document.getElementById('connectionStatus');
                connection.classList.remove('offline');
                connection.classList.add('online');
            } catch (err) {
                console.error('Status load error:', err);
                connectionRetryCount++;
                
                // AP modda veya ilk yüklemede hata mesajını gizle
                const connection = document.getElementById('connectionStatus');
                if (connectionRetryCount >= MAX_RETRIES) {
                    // Çok fazla hata - muhtemelen AP modu, status'u gizle
                    connection.style.display = 'none';
                }
            }
        }

        async function loadTimerSettings() {
            try {
                state.timer = await api('/api/timer');
                const unitEl = document.getElementById('timerUnit');
                const totalEl = document.getElementById('timerTotal');
                const alarmsEl = document.getElementById('timerAlarms');
                const enabledEl = document.getElementById('timerEnabled');
                
                if (unitEl) unitEl.value = state.timer.unit;
                if (totalEl) totalEl.value = state.timer.totalValue;
                if (alarmsEl) alarmsEl.value = state.timer.alarmCount;
                if (enabledEl) enabledEl.checked = state.timer.enabled;
            } catch (err) {
                console.error('[TIMER SETTINGS] Load error:', err);
            }
        }

        async function saveTimerSettings() {
            try {
                const unitEl = document.getElementById('timerUnit');
                const totalEl = document.getElementById('timerTotal');
                const alarmsEl = document.getElementById('timerAlarms');
                const enabledEl = document.getElementById('timerEnabled');
                
                if (!unitEl || !totalEl || !alarmsEl) {
                    throw new Error('Required form elements not found');
                }
                
                const payload = {
                    unit: unitEl.value,
                    totalValue: Number(totalEl.value),
                    alarmCount: Number(alarmsEl.value),
                    enabled: enabledEl ? enabledEl.checked : true  // Default true if element missing
                };
                
                await api('/api/timer', { method: 'PUT', body: payload });
                showAlert('alarmAlert', t('alarm.saveSuccess'));
                await loadStatus();
            } catch (err) {
                console.error('[TIMER SETTINGS] Save error:', err);
                showAlert('alarmAlert', t('alarm.saveError') + ': ' + (err.message || ''), 'error');
            }
        }

        async function startTimer() {
            try {
                await api('/api/timer/start', { method: 'POST' });
                await loadStatus();
            } catch (err) {
                console.error('[TIMER] Start error:', err);
                showAlert('alarmAlert', 'Start error: ' + err.message, 'error');
            }
        }
        
        async function pauseTimer() {
            try {
                await api('/api/timer/stop', { method: 'POST' });
                await loadStatus();
            } catch (err) {
                console.error('[TIMER] Pause error:', err);
                showAlert('alarmAlert', 'Pause error: ' + err.message, 'error');
            }
        }
        
        async function resumeTimer() {
            try {
                await api('/api/timer/resume', { method: 'POST' });
                await loadStatus();
            } catch (err) {
                console.error('[TIMER] Resume error:', err);
                showAlert('alarmAlert', 'Resume error: ' + err.message, 'error');
            }
        }
        
        async function resetTimer() {
            try {
                await api('/api/timer/reset', { method: 'POST' });
                latchRelayDisplay(false);
                await loadStatus();
            } catch (err) {
                console.error('[TIMER] Reset error:', err);
                showAlert('alarmAlert', 'Reset error: ' + err.message, 'error');
            }
        }
        
        async function virtualButton() {
            try {
                await api('/api/timer/virtual-button', { method: 'POST' });
                latchRelayDisplay(false);
                await loadStatus();
            } catch (err) {
                console.error('[TIMER] Virtual button error:', err);
                showAlert('alarmAlert', 'Virtual button error: ' + err.message, 'error');
            }
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
                if (!response.ok) throw new Error(t('mail.uploadError'));
                await loadMailSettings();
                showAlert('mailAlert', t('mail.uploadSuccess'));
            } catch (err) {
                showAlert('mailAlert', err.message || t('mail.uploadError'), 'error');
            } finally {
                event.target.value = '';
            }
        }

        async function deleteAttachment(path) {
            try {
                await api(`/api/attachments?path=${encodeURIComponent(path)}`, { method: 'DELETE' });
                await loadMailSettings();
                showAlert('mailAlert', t('mail.deleteSuccess'));
            } catch (err) {
                showAlert('mailAlert', err.message || t('mail.deleteError'), 'error');
            }
        }

        async function loadMailSettings() {
            try {
                state.mail = await api('/api/mail');
                document.getElementById('smtpServer').value = state.mail.smtpServer || '';
                document.getElementById('smtpPort').value = state.mail.smtpPort || 465;
                document.getElementById('smtpUsername').value = state.mail.username || '';
                document.getElementById('smtpPassword').value = state.mail.password || '';
                document.getElementById('mailRecipients').value = (state.mail.recipients || []).join('\n');
                document.getElementById('warningSubject').value = state.mail.warning?.subject || '';
                document.getElementById('warningBody').value = state.mail.warning?.body || '';
                document.getElementById('warningUrl').value = state.mail.warning?.getUrl || '';
                document.getElementById('finalSubject').value = state.mail.final?.subject || '';
                document.getElementById('finalBody').value = state.mail.final?.body || '';
                document.getElementById('finalUrl').value = state.mail.final?.getUrl || '';
                
                // Set active preset based on server
                const server = (state.mail.smtpServer || '').toLowerCase();
                if (server.includes('protonmail')) {
                    selectMailPreset('proton', false);
                } else if (server.includes('gmail')) {
                    selectMailPreset('gmail', false);
                }
                
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
                    username: document.getElementById('smtpUsername').value,
                    password: document.getElementById('smtpPassword').value,
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
                showAlert('mailAlert', t('mail.saveSuccess'));
            } catch (err) {
                showAlert('mailAlert', t('mail.saveError') + ': ' + (err.message || ''), 'error');
            }
        }

        async function loadWiFiSettings() {
            try {
                state.wifi = await api('/api/wifi');
                const w = state.wifi;
                const map = {
                    wifiPrimarySsid: w.primarySSID,
                    wifiPrimaryPassword: w.primaryPassword,
                    wifiSecondarySsid: w.secondarySSID,
                    wifiSecondaryPassword: w.secondaryPassword,
                    primaryIP: w.primaryIP,
                    primaryGateway: w.primaryGateway,
                    primarySubnet: w.primarySubnet,
                    primaryDNS: w.primaryDNS,
                    secondaryIP: w.secondaryIP,
                    secondaryGateway: w.secondaryGateway,
                    secondarySubnet: w.secondarySubnet,
                    secondaryDNS: w.secondaryDNS
                };
                Object.keys(map).forEach(id => { const el = document.getElementById(id); if (el) el.value = map[id] || ''; });
                document.getElementById('wifiAllowOpen').checked = !!w.allowOpenNetworks;
                document.getElementById('apModeEnabled').checked = !!w.apModeEnabled;
                document.getElementById('primaryStaticEnabled').checked = !!w.primaryStaticEnabled;
                document.getElementById('secondaryStaticEnabled').checked = !!w.secondaryStaticEnabled;
            } catch (err) { console.error(err); }
        }

        async function saveWiFiSettings() {
            try {
                const payload = {
                    primarySSID: document.getElementById('wifiPrimarySsid').value,
                    primaryPassword: document.getElementById('wifiPrimaryPassword').value,
                    secondarySSID: document.getElementById('wifiSecondarySsid').value,
                    secondaryPassword: document.getElementById('wifiSecondaryPassword').value,
                    allowOpenNetworks: document.getElementById('wifiAllowOpen').checked,
                    apModeEnabled: document.getElementById('apModeEnabled').checked,
                    primaryStaticEnabled: document.getElementById('primaryStaticEnabled').checked,
                    primaryIP: document.getElementById('primaryIP').value,
                    primaryGateway: document.getElementById('primaryGateway').value,
                    primarySubnet: document.getElementById('primarySubnet').value,
                    primaryDNS: document.getElementById('primaryDNS').value,
                    secondaryStaticEnabled: document.getElementById('secondaryStaticEnabled').checked,
                    secondaryIP: document.getElementById('secondaryIP').value,
                    secondaryGateway: document.getElementById('secondaryGateway').value,
                    secondarySubnet: document.getElementById('secondarySubnet').value,
                    secondaryDNS: document.getElementById('secondaryDNS').value
                };
                console.log('WiFi kaydet payload:', payload);
                await api('/api/wifi', { method: 'PUT', body: payload });
                showAlert('wifiAlert', t('wifi.saveSuccess'));
            } catch (err) { 
                console.error('WiFi kayıt hatası:', err);
                showAlert('wifiAlert', t('wifi.saveError') + ': ' + (err.message || ''), 'error'); 
            }
        }

        async function scanNetworks() {
            try {
                const result = await api('/api/wifi/scan');
                const target = document.getElementById('wifiScanResults');
                if (!result.networks || result.networks.length === 0) { target.innerHTML = 'Ağ bulunamadı'; }
                else {
                    target.innerHTML = result.networks.map(net => `<div class="list-item">${net.ssid || '<adı yok>'}<span class="badge">RSSI ${net.rssi}</span><span class="badge">${net.open ? 'AÇIK' : 'ŞİFRELİ'}</span>${net.current ? '<span class=\"badge\">AKTİF</span>' : ''}</div>`).join('');
                }
            } catch (err) { showAlert('wifiAlert', err.message || 'Taramada hata', 'error'); }
        }

        async function factoryReset() {
            if(!confirm(t('info.factoryResetConfirm'))) return;
            try {
                await api('/api/factory-reset', { method: 'POST' });
                location.reload();
            } catch(e){ 
                showAlert('wifiAlert', e.message || t('errors.unknown'),'error'); 
            }
        }

        async function rebootDevice() {
            if(!confirm(t('info.rebootConfirm'))) return;
            try {
                await api('/api/reboot', { method: 'POST' });
                showAlert('wifiAlert', t('info.rebootSuccess'), 'success');
            } catch(e){ 
                showAlert('wifiAlert', e.message || t('errors.unknown'),'error'); 
            }
        }

        function bindStaticIpToggles(){
            function toggle(prefix){
                const en = document.getElementById(prefix+"StaticEnabled")?.checked;
                ["IP","Gateway","Subnet","DNS"].forEach(s=>{
                    const el = document.getElementById(prefix.toLowerCase()+s);
                    if(el) el.disabled = !en;
                });
            }
            const p = document.getElementById('primaryStaticEnabled');
            const s = document.getElementById('secondaryStaticEnabled');
            if(p) p.addEventListener('change', ()=>toggle('primary'));
            if(s) s.addEventListener('change', ()=>toggle('secondary'));
            toggle('primary');
            toggle('secondary');
        }

        let initialized = false; // Global flag to prevent double init

        async function init() {
            if (initialized) {
                console.warn('[INIT] Already initialized, skipping...');
                return;
            }
            initialized = true;
            
            console.log('[INIT] Starting initialization...');
            
            // 1. DİL BUTONLARINI KUR (en önce - global)
            console.log('[INIT] Setting up language buttons...');
            document.querySelectorAll('.lang-btn').forEach(btn => {
                const lang = btn.getAttribute('data-lang');
                console.log('[LANG] Attaching listener to:', lang);
                btn.addEventListener('click', function(e) {
                    console.log('[LANG] Switching to:', lang);
                    e.preventDefault();
                    switchLanguage(lang);
                });
            });
            
            // 2. TAB SİSTEMİNİ KUR
            console.log('[INIT] Setting up tab navigation...');
            document.querySelectorAll('.tab').forEach((tab, index) => {
                const tabId = tab.getAttribute('data-tab');
                console.log('[TAB] Attaching listener to tab', index, ':', tabId);
                tab.addEventListener('click', function(e) {
                    console.log('[TAB] Click event on', tabId);
                    e.preventDefault();
                    e.stopPropagation();
                    openTab(e, tabId);
                }, true); // Use capture phase
            });
            
            // 3. İlk tab'ı aktif et
            console.log('[INIT] Activating first tab...');
            const firstTab = document.querySelector('.tab[data-tab="alarmTab"]');
            if (firstTab) {
                openTab({ currentTarget: firstTab }, 'alarmTab');
            }
            
            // 4. MAIL TEST BUTONLARINI KUR
            console.log('[INIT] Setting up mail test buttons...');
            const btnTestWarning = document.getElementById('btnTestWarning');
            const btnTestFinal = document.getElementById('btnTestFinal');
            const btnSaveMail = document.getElementById('btnSaveMail');
            
            if (btnTestWarning) {
                // Disable during test to prevent double-click
                btnTestWarning.addEventListener('click', async function(e) {
                    e.preventDefault();
                    if (btnTestWarning.disabled) {
                        console.log('[BUTTON] Test Warning - Already running, ignored');
                        return;
                    }
                    console.log('[BUTTON] Test Warning clicked');
                    btnTestWarning.disabled = true;
                    try {
                        await sendWarningTest();
                    } finally {
                        setTimeout(() => { btnTestWarning.disabled = false; }, 1000);
                    }
                });
            }
            
            if (btnTestFinal) {
                // Disable during test to prevent double-click
                btnTestFinal.addEventListener('click', async function(e) {
                    e.preventDefault();
                    if (btnTestFinal.disabled) {
                        console.log('[BUTTON] Test Final - Already running, ignored');
                        return;
                    }
                    console.log('[BUTTON] Test Final clicked');
                    btnTestFinal.disabled = true;
                    try {
                        await sendFinalTest();
                    } finally {
                        setTimeout(() => { btnTestFinal.disabled = false; }, 1000);
                    }
                });
            }
            
            if (btnSaveMail) {
                btnSaveMail.addEventListener('click', function(e) {
                    e.preventDefault();
                    console.log('[BUTTON] Save Mail clicked');
                    saveMailSettings();
                });
            }
            
            // 5. Diğer ayarları yükle (paralel, bloke etmeden)
            console.log('[INIT] Loading settings...');
            document.getElementById('deviceId').textContent = "";
            loadStatus(); // async ama await etme
            loadTimerSettings(); // async ama await etme
            loadMailSettings(); // async ama await etme
            loadWiFiSettings(); // async ama await etme
            bindStaticIpToggles();
            
            // 6. Düzenli status güncelleme (sadece sayfa görünürken)
            console.log('[INIT] Setting up status polling...');
            let statusInterval = setInterval(loadStatus, 3000);
            
            // Page Visibility API - Sayfa arka plandayken polling'i durdur
            document.addEventListener('visibilitychange', function() {
                if (document.hidden) {
                    // Sayfa arka planda - interval'i durdur
                    if (statusInterval) {
                        clearInterval(statusInterval);
                        statusInterval = null;
                    }
                } else {
                    // Sayfa ön planda - interval'i yeniden başlat
                    if (!statusInterval) {
                        loadStatus(); // Hemen bir kez çalıştır
                        statusInterval = setInterval(loadStatus, 3000);
                    }
                }
            });
            
            console.log('[INIT] Initialization complete!');
        }

        // init() will be called from DOMContentLoaded
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
    
    server->on("/api/i18n", HTTP_GET, [this]() { handleI18n(); });

    server->on("/api/logs", HTTP_GET, [this]() { handleLogs(); });
    server->on("/api/reboot", HTTP_POST, [this]() { handleReboot(); });
    server->on("/api/factory-reset", HTTP_POST, [this]() { handleFactoryReset(); });

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
            Serial.println(F("[WiFi] STA bağlantısı başarısız"));
        }
    } else {
        Serial.println(F("[WiFi] Kayıtlı WiFi yok"));
    }
    
    // AP modu ayarına göre karar ver
    bool shouldStartAP = wifiConfig.apModeEnabled;
    
    // GÜVENLİK: Sadece AP mode açıksa VE STA bağlantısı başarısızsa AP'yi aç
    // İlk kurulumda (hiç WiFi ayarı yoksa) hemen AP aç
    if (!hasStoredWiFi) {
        shouldStartAP = true;
        Serial.println(F("[WiFi] İlk kurulum - AP modu zorla açılıyor"));
    } else if (!staConnected && wifiConfig.apModeEnabled) {
        Serial.println(F("[WiFi] STA başarısız ve AP etkin - AP modu açılıyor"));
    } else if (!staConnected && !wifiConfig.apModeEnabled) {
        Serial.println(F("[WiFi] STA başarısız ama AP devre dışı - sadece STA modunda tekrar denenecek"));
        shouldStartAP = false;
    }
    
    // WiFi modunu ayarla
    if (shouldStartAP && staConnected) {
        // Dual mode: hem AP hem STA
        WiFi.mode(WIFI_AP_STA);
        Serial.println(F("[WiFi] Mod: WIFI_AP_STA (Dual)"));
    } else if (shouldStartAP && !staConnected) {
        // Sadece AP modu
        WiFi.mode(WIFI_AP);
        Serial.println(F("[WiFi] Mod: WIFI_AP (Sadece AP)"));
    } else if (!shouldStartAP && staConnected) {
        // Sadece STA modu
        WiFi.mode(WIFI_STA);
        Serial.println(F("[WiFi] Mod: WIFI_STA (Sadece STA)"));
    }
    
    delay(100);
    
    // AP modunu başlat (eğer gerekiyorsa)
    if (shouldStartAP) {
        WiFi.softAP("SmartKraft-DMF", "12345678");
        delay(500);
        
        // Captive portal için DNS server başlat
        if (dnsServer) {
            dnsServer->start(53, "*", WiFi.softAPIP());
            Serial.println(F("[DNS] Captive portal DNS başlatıldı"));
        }
        
        Serial.printf("[Web] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println(F("[Web] AP modu kapalı (kullanıcı ayarı)"));
        
        // DNS sunucusunu durdur
        if (dnsServer) {
            dnsServer->stop();
        }
    }
    
    // WebServer'ı başlat
    server->begin();
    
    // WiFi güç yönetimini tamamen kapat (light sleep engellensin)
    WiFi.setSleep(WIFI_PS_NONE);
    Serial.println(F("[Web] WiFi power save: NONE"));
    
    if (staConnected) {
        Serial.printf("[Web] STA IP: %s\n", WiFi.localIP().toString().c_str());
    }
    
    if (shouldStartAP && staConnected) {
        Serial.println(F("[Web] Dual mode aktif (AP + STA)"));
    } else if (shouldStartAP && !staConnected) {
        Serial.println(F("[Web] Sadece AP modunda"));
    } else if (!shouldStartAP && staConnected) {
        Serial.println(F("[Web] Sadece STA modunda (AP kapalı)"));
    }
    
    Serial.println(F("[Web] WebServer başlatıldı"));
}

void WebInterface::loop() {
    if (!server) return;

    // Captive portal DNS işle
    if (dnsServer) {
        dnsServer->processNextRequest();
    }
    
    // Web client isteklerini işle
    server->handleClient();
    
    // Periyodik status güncellemesi (2 saniyede bir)
    if (millis() - lastStatusPush > 2000) {
        broadcastStatus();
        lastStatusPush = millis();
    }
    
    // WiFi bağlantısını kontrol et ve gerekirse yeniden bağlan
    static unsigned long lastWiFiCheck = 0;
    static bool wasConnected = false;
    static unsigned long lastDebugPrint = 0;
    
    if (millis() - lastWiFiCheck > 30000) { // 30 saniyede bir kontrol
        wifi_mode_t mode = WiFi.getMode();
        wl_status_t status = WiFi.status();
        
        // Debug log (60 saniyede bir)
        if (millis() - lastDebugPrint > 60000) {
            Serial.printf("[WiFi Loop] Mode: %d, Status: %d, STA IP: %s, AP IP: %s\n",
                mode, status,
                WiFi.localIP().toString().c_str(),
                WiFi.softAPIP().toString().c_str());
            lastDebugPrint = millis();
        }
        
        // Sadece STA veya DUAL modda ve GERÇEKTEN bağlantı kaybedildiyse
        if (mode == WIFI_STA || mode == WIFI_AP_STA) {
            bool nowConnected = (status == WL_CONNECTED);
            
            // Bağlantı KESİLDİ (önceden bağlıydı şimdi değil)
            if (wasConnected && !nowConnected) {
                Serial.println(F("[WiFi] Bağlantı koptu, yeniden bağlanılıyor..."));
                
                // Reconnect yerine network manager'ı kullan (conflict yok)
                if (network) {
                    network->connectToKnown();
                }
            }
            
            // HİÇ BAĞLANMAMIŞ (önceden de bağlı değildi, şimdi de değil)
            // ANCAK sadece WL_DISCONNECTED veya WL_CONNECTION_LOST durumunda
            else if (!wasConnected && !nowConnected) {
                // WL_IDLE_STATUS artık reconnect tetiklemez (AP modunda her zaman idle)
                // Sadece açık disconnect durumlarında reconnect dene
                if (status == WL_DISCONNECTED || status == WL_CONNECTION_LOST) {
                    Serial.printf("[WiFi] Bağlantı yok (status: %d), deneme yapılıyor...\n", status);
                    if (network) {
                        network->connectToKnown();
                    }
                }
            }
            
            wasConnected = nowConnected;
        }
        
        lastWiFiCheck = millis();
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
    WiFiSettings wifi = network->getConfig();
    doc["allowOpenNetworks"] = wifi.allowOpenNetworks;
    doc["apModeEnabled"] = wifi.apModeEnabled;
    doc["primaryStaticEnabled"] = wifi.primaryStaticEnabled;
    doc["secondaryStaticEnabled"] = wifi.secondaryStaticEnabled;
    
    sendJson(doc);
}

void WebInterface::handleTimerGet() {
    StaticJsonDocument<JSON_CAPACITY> doc;
    auto settings = scheduler->settings();
    
    // Dakika/Saat/Gün seçimi
    if (settings.unit == TimerSettings::MINUTES) {
        doc["unit"] = "minutes";
    } else if (settings.unit == TimerSettings::HOURS) {
        doc["unit"] = "hours";
    } else {
        doc["unit"] = "days";
    }
    
    doc["totalValue"] = settings.totalValue;
    doc["alarmCount"] = settings.alarmCount;
    doc["enabled"] = settings.enabled;
    sendJson(doc);
}

void WebInterface::handleTimerUpdate() {
    Serial.println(F("[API] /api/timer PUT - Alarm ayarları güncelleniyor"));
    
    if (!server->hasArg("plain")) {
        Serial.println(F("[API] HATA - Body yok"));
        server->send(400, "application/json", "{\"error\":\"JSON bekleniyor\"}");
        return;
    }
    
    String body = server->arg("plain");
    Serial.printf("[API] Request body: %s\n", body.c_str());
    
    StaticJsonDocument<JSON_CAPACITY> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        Serial.printf("[API] JSON parse HATASI: %s\n", error.c_str());
        server->send(400, "application/json", "{\"error\":\"JSON parse error\"}");
        return;
    }
    
    try {
        TimerSettings settings = scheduler->settings();
        String unit = doc["unit"].as<String>();
        
        // Dakika/Saat/Gün seçimi
        if (unit == "minutes") {
            settings.unit = TimerSettings::MINUTES;
        } else if (unit == "hours") {
            settings.unit = TimerSettings::HOURS;
        } else {
            settings.unit = TimerSettings::DAYS;
        }
        
        settings.totalValue = doc["totalValue"].as<uint16_t>();
        settings.totalValue = constrain(settings.totalValue, (uint16_t)1, (uint16_t)60);
        settings.alarmCount = doc["alarmCount"].as<uint8_t>();
        settings.alarmCount = constrain(settings.alarmCount, (uint8_t)0, (uint8_t)MAX_ALARMS);
        settings.enabled = doc["enabled"].as<bool>();

        Serial.printf("[API] Alarm ayarları: %s %d, %d alarm, %s\n", 
                      unit.c_str(), settings.totalValue, settings.alarmCount, 
                      settings.enabled ? "Aktif" : "Pasif");

        Serial.println(F("[API] scheduler->configure() çağrılıyor..."));
        scheduler->configure(settings);
        Serial.println(F("[API] configure() tamamlandı"));
        
        Serial.println(F("[API] Response gönderiliyor..."));
        server->send(200, "application/json", "{\"status\":\"ok\"}");
        Serial.println(F("[API] Alarm ayarları başarıyla kaydedildi"));
        
    } catch (...) {
        Serial.println(F("[API] EXCEPTION - configure() patladı!"));
        server->send(500, "application/json", "{\"error\":\"Internal server error\"}");
    }
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
    // WiFi kontrolü önce
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[MAIL TEST] HATA - WiFi yok"));
        server->send(503, "application/json", "{\"error\":\"WiFi required\"}");
        return;
    }
    
    // Request body'yi al
    String body = server->arg("plain");
    Serial.println(F("========== MAIL TEST BAŞLADI =========="));
    Serial.printf("[MAIL TEST] Body: %s\n", body.c_str());
    
    // JSON parse
    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, body);
    
    if (err) {
        Serial.printf("[MAIL TEST] JSON HATA: %s\n", err.c_str());
        server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    // testType oku (char array olarak direkt karşılaştır)
    const char* testTypeRaw = doc["testType"] | "warning";
    Serial.printf("[MAIL TEST] testType: '%s'\n", testTypeRaw);
    
    // Karşılaştırma (strcmp kullan - C-style string comparison)
    bool isDMF = (strcmp(testTypeRaw, "dmf") == 0);
    Serial.printf("[MAIL TEST] isDMF: %s\n", isDMF ? "TRUE" : "FALSE");
    
    // Mail gönder
    ScheduleSnapshot snap = scheduler->snapshot();
    String errorMsg;
    bool success = false;
    unsigned long start = millis();
    
    if (isDMF) {
        Serial.println(F("[MAIL TEST] >>> DMF TEST ÇAĞRILIYOR <<<"));
        success = mail->sendFinalTest(snap, errorMsg);
    } else {
        Serial.println(F("[MAIL TEST] >>> WARNING TEST ÇAĞRILIYOR <<<"));
        success = mail->sendWarningTest(snap, errorMsg);
    }
    
    unsigned long elapsed = millis() - start;
    Serial.printf("[MAIL TEST] Sonuç: %s (%lu ms)\n", success ? "OK" : "FAIL", elapsed);
    Serial.println(F("========== MAIL TEST BİTTİ =========="));
    
    // Response
    if (success) {
        server->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        Serial.printf("[MAIL TEST] Hata: %s\n", errorMsg.c_str());
        server->send(500, "application/json", "{\"error\":\"" + errorMsg + "\"}");
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
    doc["apModeEnabled"] = wifi.apModeEnabled;
    doc["primaryStaticEnabled"] = wifi.primaryStaticEnabled;
    doc["primaryIP"] = wifi.primaryIP;
    doc["primaryGateway"] = wifi.primaryGateway;
    doc["primarySubnet"] = wifi.primarySubnet;
    doc["primaryDNS"] = wifi.primaryDNS;
    doc["secondaryStaticEnabled"] = wifi.secondaryStaticEnabled;
    doc["secondaryIP"] = wifi.secondaryIP;
    doc["secondaryGateway"] = wifi.secondaryGateway;
    doc["secondarySubnet"] = wifi.secondarySubnet;
    doc["secondaryDNS"] = wifi.secondaryDNS;
    sendJson(doc);
}

void WebInterface::handleWiFiUpdate() {
    if (!server->hasArg("plain")) { server->send(400, "application/json", "{\"error\":\"json\"}"); return; }
    StaticJsonDocument<JSON_CAPACITY> doc;
    if (deserializeJson(doc, server->arg("plain"))) { server->send(400, "application/json", "{\"error\":\"json\"}"); return; }
    
    WiFiSettings wifi = network->getConfig();
    wifi.primarySSID = doc["primarySSID"].as<String>();
    wifi.primaryPassword = doc["primaryPassword"].as<String>();
    wifi.secondarySSID = doc["secondarySSID"].as<String>();
    wifi.secondaryPassword = doc["secondaryPassword"].as<String>();
    wifi.allowOpenNetworks = doc["allowOpenNetworks"].as<bool>();
    wifi.apModeEnabled = doc["apModeEnabled"].as<bool>();
    wifi.primaryStaticEnabled = doc["primaryStaticEnabled"].as<bool>();
    wifi.primaryIP = doc["primaryIP"].as<String>();
    wifi.primaryGateway = doc["primaryGateway"].as<String>();
    wifi.primarySubnet = doc["primarySubnet"].as<String>();
    wifi.primaryDNS = doc["primaryDNS"].as<String>();
    wifi.secondaryStaticEnabled = doc["secondaryStaticEnabled"].as<bool>();
    wifi.secondaryIP = doc["secondaryIP"].as<String>();
    wifi.secondaryGateway = doc["secondaryGateway"].as<String>();
    wifi.secondarySubnet = doc["secondarySubnet"].as<String>();
    wifi.secondaryDNS = doc["secondaryDNS"].as<String>();
    
    // Ayarları kaydet
    network->setConfig(wifi);
    
    // Şu anki WiFi durumunu kontrol et
    bool isStaConnected = (WiFi.status() == WL_CONNECTED);
    
    // WiFi modunu ayarla
    if (wifi.apModeEnabled && isStaConnected) {
        // AP + STA (Dual mode)
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP("SmartKraft-DMF", "12345678");
        if (dnsServer) dnsServer->start(53, "*", WiFi.softAPIP());
        Serial.println(F("[WiFi] AP modu açıldı (Dual mode)"));
    } else if (wifi.apModeEnabled && !isStaConnected) {
        // Sadece AP
        WiFi.mode(WIFI_AP);
        WiFi.softAP("SmartKraft-DMF", "12345678");
        if (dnsServer) dnsServer->start(53, "*", WiFi.softAPIP());
        Serial.println(F("[WiFi] AP modu açıldı (Sadece AP)"));
    } else if (!wifi.apModeEnabled && isStaConnected) {
        // Sadece STA - AP'yi kapat
        if (dnsServer) dnsServer->stop();
        WiFi.mode(WIFI_STA);
        Serial.println(F("[WiFi] AP modu kapatıldı (Sadece STA)"));
    } else {
        // AP kapalı, STA da bağlı değil
        if (dnsServer) dnsServer->stop();
        WiFi.mode(WIFI_STA);
        Serial.println(F("[WiFi] AP modu kapatıldı, STA deneniyor"));
    }
    
    delay(100);
    
    // Yeni ayarlarla bağlantıyı dene
    network->ensureConnected(false);
    
    server->send(204);
}

void WebInterface::handleWiFiScan() {
    auto list = network->scanNetworks();
    StaticJsonDocument<JSON_CAPACITY> doc;
    auto arr = doc.createNestedArray("networks");
    String cur = WiFi.SSID();
    for (auto &net : list) {
        auto item = arr.createNestedObject();
        item["ssid"] = net.ssid;
        item["rssi"] = net.rssi;
        item["open"] = net.open;
        item["current"] = (net.ssid == cur);
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

void WebInterface::handleI18n() {
    String lang = server->arg("lang");
    if (lang.isEmpty()) {
        lang = "en";
    }
    
    const char* i18nData = nullptr;
    
    if (lang == "en") {
        i18nData = I18N_EN;
    } else if (lang == "de") {
        i18nData = I18N_DE;
    } else if (lang == "tr") {
        i18nData = I18N_TR;
    } else {
        i18nData = I18N_EN; // default to English
    }
    
    server->send(200, "application/json", String(i18nData));
}

void WebInterface::sendJson(const JsonDocument &doc) {
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WebInterface::handleFactoryReset() {
    store->eraseAll();
    StaticJsonDocument<64> doc; 
    doc["status"] = "reset"; 
    sendJson(doc);
    delay(1000);
    ESP.restart();
}

void WebInterface::handleReboot() {
    StaticJsonDocument<64> doc; 
    doc["status"] = "rebooting"; 
    sendJson(doc);
    delay(200);
    ESP.restart();
}

/**
 * @file web_server.cpp
 * @brief Implementation of HTTP Web Server
 * 
 * LOGIC:
 * - REST API with JSON responses
 * - Simple HTML dashboard in PROGMEM
 * - CORS headers for development
 * 
 * RULES: #HTTP(24) #JSON(23)
 */

#include "web_server.h"
#include <logger.h>
#include <ArduinoJson.h>

//=============================================================================
// HTML DASHBOARD (PROGMEM to save RAM)
//=============================================================================
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TuoiCay v1.0</title>
    <style>*{box-sizing:border-box;margin:0;padding:0}body{font-family:Arial,sans-serif;background:#1a1a2e;color:#eee;padding:20px}.container{max-width:500px;margin:0 auto}h1{color:#00d9ff;text-align:center;margin-bottom:20px}.card{background:#16213e;border-radius:10px;padding:20px;margin-bottom:15px}.card h2{color:#00d9ff;font-size:14px;margin-bottom:10px;text-transform:uppercase}.value{font-size:36px;font-weight:bold;color:#fff}.unit{font-size:18px;color:#888}.status{display:inline-block;padding:5px 15px;border-radius:20px;font-weight:bold}.status.on{background:#00c853;color:#fff}.status.off{background:#ff5252;color:#fff}.status.auto{background:#2196f3;color:#fff}.status.manual{background:#ff9800;color:#fff}.btn{display:block;width:100%;padding:15px;border:none;border-radius:8px;font-size:16px;font-weight:bold;cursor:pointer;margin-top:10px}.btn-pump{background:#00d9ff;color:#1a1a2e}.btn-mode{background:#7c4dff;color:#fff}.btn:active{transform:scale(0.98)}.row{display:flex;gap:15px}.row .card{flex:1}.config{display:flex;align-items:center;gap:10px;margin-top:10px}.config input{flex:1;padding:10px;border:1px solid #333;border-radius:5px;background:#0f0f23;color:#fff}.info{font-size:12px;color:#666;text-align:center;margin-top:20px}.schedule-item{display:flex;align-items:center;gap:8px;margin:8px 0;padding:10px;background:#0f0f23;border-radius:8px}.schedule-item input[type="time"]{padding:8px;border:1px solid #333;border-radius:5px;background:#1a1a2e;color:#fff}.schedule-item input[type="number"]{width:60px;padding:8px;border:1px solid #333;border-radius:5px;background:#1a1a2e;color:#fff}.schedule-item label{font-size:12px;color:#888}.switch{position:relative;width:50px;height:26px}.switch input{opacity:0;width:0;height:0}.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#333;border-radius:26px;transition:0.3s}.slider:before{position:absolute;content:"";height:20px;width:20px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:0.3s}input:checked+.slider{background:#00c853}input:checked+.slider:before{transform:translateX(24px)}.btn-small{padding:8px 15px;font-size:14px}</style>
</head>
<body>
    <div class="container">
        <h1>🌱 TuoiCay v1.0</h1>
        
        <div class="card">
            <h2>Độ ẩm đất</h2>
            <span class="value" id="moisture">--</span><span class="unit">%</span>
        </div>
        
        <div class="row">
            <div class="card">
                <h2>Máy bơm</h2>
                <span class="status off" id="pumpStatus">OFF</span>
                <div id="pumpInfo" style="font-size:12px; color:#888; margin-top:5px;"></div>
                <button class="btn btn-pump" onclick="togglePump()">BẬT/TẮT BƠM</button>
            </div>
            <div class="card">
                <h2>Chế độ</h2>
                <span class="status manual" id="modeStatus">MANUAL</span>
                <button class="btn btn-mode" onclick="toggleMode()">ĐỔI CHẾ ĐỘ</button>
            </div>
        </div>
        
        <div class="card">
            <h2>🎚️ Tốc độ bơm</h2>
            <div style="display:flex; align-items:center; gap:15px; margin:10px 0;">
                <input type="range" id="pumpSpeed" min="30" max="100" value="100" 
                       style="flex:1; height:8px;" oninput="updateSpeedLabel(this.value)">
                <span id="speedLabel" style="min-width:50px; font-weight:bold;">100%</span>
            </div>
            <button class="btn btn-mode" onclick="setSpeed()">💾 Áp dụng tốc độ</button>
        </div>
        
        <div class="card">
            <h2>Cài đặt ngưỡng</h2>
            <div class="config">
                <label>Khô:</label>
                <input type="number" id="dryThreshold" min="0" max="100" value="30">
                <label>Ướt:</label>
                <input type="number" id="wetThreshold" min="0" max="100" value="50">
                <button class="btn" style="width:auto; padding:10px 20px;" onclick="saveConfig()">Lưu</button>
            </div>
        </div>
        
        <div class="card">
            <h2>⏰ Lịch tưới tự động</h2>
            <div style="display:flex; align-items:center; justify-content:space-between; margin-bottom:10px;">
                <span>Bật lịch tưới</span>
                <label class="switch">
                    <input type="checkbox" id="scheduleEnabled" onchange="toggleSchedule()">
                    <span class="slider"></span>
                </label>
            </div>
            <div id="scheduleList">
                <div class="schedule-item">
                    <label>Lịch 1:</label>
                    <input type="time" id="sched0_time" value="06:00">
                    <input type="number" id="sched0_dur" min="10" max="300" value="30" placeholder="giây">
                    <label>giây</label>
                    <label class="switch">
                        <input type="checkbox" id="sched0_en">
                        <span class="slider"></span>
                    </label>
                </div>
                <div class="schedule-item">
                    <label>Lịch 2:</label>
                    <input type="time" id="sched1_time" value="18:00">
                    <input type="number" id="sched1_dur" min="10" max="300" value="30" placeholder="giây">
                    <label>giây</label>
                    <label class="switch">
                        <input type="checkbox" id="sched1_en">
                        <span class="slider"></span>
                    </label>
                </div>
                <div class="schedule-item">
                    <label>Lịch 3:</label>
                    <input type="time" id="sched2_time" value="12:00">
                    <input type="number" id="sched2_dur" min="10" max="300" value="30" placeholder="giây">
                    <label>giây</label>
                    <label class="switch">
                        <input type="checkbox" id="sched2_en">
                        <span class="slider"></span>
                    </label>
                </div>
                <div class="schedule-item">
                    <label>Lịch 4:</label>
                    <input type="time" id="sched3_time" value="00:00">
                    <input type="number" id="sched3_dur" min="10" max="300" value="30" placeholder="giây">
                    <label>giây</label>
                    <label class="switch">
                        <input type="checkbox" id="sched3_en">
                        <span class="slider"></span>
                    </label>
                </div>
            </div>
            <button class="btn btn-mode" onclick="saveSchedule()">💾 Lưu lịch tưới</button>
            <div id="scheduleInfo" style="font-size:12px; color:#888; margin-top:10px; text-align:center;"></div>
        </div>
        
        <div class="info">
            Uptime: <span id="uptime">--</span>s | IP: <span id="ip">--</span>
        </div>
    </div>
    
    <script>
        console.log('TuoiCay script loaded');
        
        function fetchStatus() {
            fetch('/api/status')
                .then(r => {
                    console.log('Status response:', r.status);
                    return r.json();
                })
                .then(d => {
                    console.log('Status data:', d);
                    document.getElementById('moisture').textContent = d.moisture;
                    
                    const ps = document.getElementById('pumpStatus');
                    ps.textContent = d.pump ? 'ON' : 'OFF';
                    ps.className = 'status ' + (d.pump ? 'on' : 'off');
                    
                    const info = d.pump ? `${d.reason} - ${d.runtime}s` : '';
                    document.getElementById('pumpInfo').textContent = info;
                    
                    const ms = document.getElementById('modeStatus');
                    ms.textContent = d.autoMode ? 'AUTO' : 'MANUAL';
                    ms.className = 'status ' + (d.autoMode ? 'auto' : 'manual');
                    
                    // Only update threshold inputs if not focused (user is not editing)
                    const dryInput = document.getElementById('dryThreshold');
                    const wetInput = document.getElementById('wetThreshold');
                    if (document.activeElement !== dryInput) {
                        dryInput.value = d.thresholdDry;
                    }
                    if (document.activeElement !== wetInput) {
                        wetInput.value = d.thresholdWet;
                    }
                    
                    document.getElementById('uptime').textContent = d.uptime;
                    document.getElementById('ip').textContent = d.ip;
                })
                .catch(e => {
                    console.error('fetchStatus error:', e);
                });
        }
        
        function togglePump() {
            console.log('togglePump called');
            fetch('/api/pump', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({action: 'toggle'})
            })
            .then(r => {
                console.log('Pump response status:', r.status);
                return r.json();
            })
            .then(d => {
                console.log('Pump response:', d);
                if (d.ok) {
                    const ps = document.getElementById('pumpStatus');
                    ps.textContent = d.pump ? 'ON' : 'OFF';
                    ps.className = 'status ' + (d.pump ? 'on' : 'off');
                } else if (d.error) {
                    alert(d.error);
                }
                setTimeout(fetchStatus, 500);
            })
            .catch(e => {
                console.error('togglePump error:', e);
                alert('Lỗi: ' + e.message);
            });
        }
        
        function toggleMode() {
            console.log('toggleMode called');
            fetch('/api/mode', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({toggle: true})
            })
            .then(r => {
                console.log('Mode response status:', r.status);
                return r.json();
            })
            .then(d => {
                console.log('Mode response:', d);
                if (d.ok) {
                    const ms = document.getElementById('modeStatus');
                    ms.textContent = d.autoMode ? 'AUTO' : 'MANUAL';
                    ms.className = 'status ' + (d.autoMode ? 'auto' : 'manual');
                }
                setTimeout(fetchStatus, 500);
            })
            .catch(e => {
                console.error('toggleMode error:', e);
                alert('Lỗi: ' + e.message);
            });
        }
        
        function saveConfig() {
            const dry = parseInt(document.getElementById('dryThreshold').value);
            const wet = parseInt(document.getElementById('wetThreshold').value);
            console.log('saveConfig called with: dry=' + dry + ', wet=' + wet);
            
            if (dry >= wet) {
                alert('Lỗi: Ngưỡng khô phải nhỏ hơn ngưỡng ướt!');
                return;
            }
            
            fetch('/api/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({threshold_dry: dry, threshold_wet: wet})
            })
            .then(r => {
                console.log('Config response status:', r.status);
                return r.json();
            })
            .then(d => {
                console.log('Config response:', d);
                if (d.ok) {
                    alert('Đã lưu ngưỡng: Khô=' + dry + '%, Ướt=' + wet + '%');
                    fetchStatus();
                } else if (d.error) {
                    alert('Lỗi: ' + d.error);
                }
            })
            .catch(e => {
                console.error('saveConfig error:', e);
                alert('Lỗi khi lưu: ' + e.message);
            });
        }
        
        // Speed control functions
        function updateSpeedLabel(val) {
            document.getElementById('speedLabel').textContent = val + '%';
        }
        
        function setSpeed() {
            const speed = parseInt(document.getElementById('pumpSpeed').value);
            console.log('setSpeed called with:', speed);
            fetch('/api/speed', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({speed: speed})
            })
            .then(r => {
                console.log('Speed response status:', r.status);
                return r.json();
            })
            .then(d => {
                console.log('Speed response:', d);
                if (d.ok) {
                    alert('Đã áp dụng tốc độ ' + d.speed + '%');
                } else if (d.error) {
                    alert('Lỗi: ' + d.error);
                }
            })
            .catch(e => {
                console.error('setSpeed error:', e);
                alert('Lỗi: ' + e.message);
            });
        }
        
        function fetchSpeed() {
            fetch('/api/speed')
                .then(r => r.json())
                .then(d => {
                    if (d.speed) {
                        document.getElementById('pumpSpeed').value = d.speed;
                        document.getElementById('speedLabel').textContent = d.speed + '%';
                    }
                })
                .catch(e => console.error('Error:', e));
        }
        
        // Schedule functions
        function fetchSchedule() {
            fetch('/api/schedule')
                .then(r => r.json())
                .then(d => {
                    document.getElementById('scheduleEnabled').checked = d.enabled;
                    if (d.schedules) {
                        for (let i = 0; i < 4; i++) {
                            const s = d.schedules[i];
                            if (s) {
                                const h = String(s.hour).padStart(2,'0');
                                const m = String(s.minute).padStart(2,'0');
                                document.getElementById('sched'+i+'_time').value = h+':'+m;
                                document.getElementById('sched'+i+'_dur').value = s.duration;
                                document.getElementById('sched'+i+'_en').checked = s.enabled;
                            }
                        }
                    }
                    updateScheduleInfo(d);
                })
                .catch(e => console.error('Schedule error:', e));
        }
        
        function updateScheduleInfo(d) {
            const info = document.getElementById('scheduleInfo');
            if (d.nextRun) {
                info.textContent = 'Lịch tiếp theo: ' + d.nextRun;
            } else if (!d.enabled) {
                info.textContent = 'Lịch tưới đang TẮT';
            } else {
                info.textContent = '';
            }
        }
        
        function toggleSchedule() {
            const enabled = document.getElementById('scheduleEnabled').checked;
            fetch('/api/schedule', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({enabled: enabled, toggle: true})
            })
            .then(r => r.json())
            .then(d => {
                if (d.ok) {
                    document.getElementById('scheduleEnabled').checked = d.enabled;
                    updateScheduleInfo(d);
                }
            })
            .catch(e => console.error('Toggle schedule error:', e));
        }
        
        function saveSchedule() {
            const schedules = [];
            for (let i = 0; i < 4; i++) {
                const time = document.getElementById('sched'+i+'_time').value.split(':');
                schedules.push({
                    hour: parseInt(time[0]),
                    minute: parseInt(time[1]),
                    duration: parseInt(document.getElementById('sched'+i+'_dur').value),
                    enabled: document.getElementById('sched'+i+'_en').checked
                });
            }
            fetch('/api/schedule', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({schedules: schedules})
            })
            .then(r => r.json())
            .then(d => {
                if (d.ok) {
                    alert('Đã lưu lịch tưới!');
                    fetchSchedule();
                }
            })
            .catch(e => console.error('Save schedule error:', e));
        }
        
        // Initialize
        try {
            console.log('Initializing...');
            fetchStatus();
            fetchSchedule();
            fetchSpeed();
            setInterval(fetchStatus, 1000);   // Update every 1s (fastest)
            setInterval(fetchSchedule, 30000);
            console.log('Initialization complete');
        } catch (e) {
            console.error('Init error:', e);
            alert('Lỗi khởi tạo: ' + e.message);
        }
    </script>
</body>
</html>
)rawliteral";

//=============================================================================
// WEB SERVER IMPLEMENTATION
//=============================================================================

WebServerManager::WebServerManager(uint16_t port)
    : _server(port)
    , _running(false)
    , _getMoisture(nullptr)
    , _getPumpState(nullptr)
    , _getPumpReason(nullptr)
    , _getPumpRuntime(nullptr)
    , _getAutoMode(nullptr)
    , _setPump(nullptr)
    , _setAutoMode(nullptr)
    , _setThresholds(nullptr)
    , _getSpeed(nullptr)
    , _setSpeed(nullptr)
    , _thresholdDry(nullptr)
    , _thresholdWet(nullptr)
    , _getSchedule(nullptr)
    , _setScheduleEnabled(nullptr)
    , _setScheduleEntry(nullptr)
    , _saveSchedule(nullptr)
{
}

bool WebServerManager::begin() {
    // Set up routes
    _server.on("/", HTTP_GET, [this]() { _handleRoot(); });
    _server.on("/api/status", HTTP_GET, [this]() { _handleStatus(); });
    _server.on("/api/pump", HTTP_POST, [this]() { _handlePump(); });
    _server.on("/api/mode", HTTP_POST, [this]() { _handleMode(); });
    _server.on("/api/config", HTTP_POST, [this]() { _handleConfig(); });
    _server.on("/api/speed", HTTP_GET, [this]() { _handleSpeed(); });
    _server.on("/api/speed", HTTP_POST, [this]() { _handleSpeed(); });
    _server.on("/api/schedule", HTTP_GET, [this]() { _handleSchedule(); });
    _server.on("/api/schedule", HTTP_POST, [this]() { _handleSchedule(); });
    _server.onNotFound([this]() { _handleNotFound(); });
    
    _server.begin();
    _running = true;
    
    LOG_INF(MOD_WEB, "init", "Web server started on port 80");
    
    return true;
}

void WebServerManager::update() {
    if (_running) {
        _server.handleClient();
    }
}

void WebServerManager::stop() {
    _server.stop();
    _running = false;
    LOG_INF(MOD_WEB, "stop", "Web server stopped");
}

void WebServerManager::setDataProviders(
    GetMoistureFunc getMoisture,
    GetPumpStateFunc getPumpState,
    GetPumpReasonFunc getPumpReason,
    GetPumpRuntimeFunc getPumpRuntime,
    GetAutoModeFunc getAutoMode
) {
    _getMoisture = getMoisture;
    _getPumpState = getPumpState;
    _getPumpReason = getPumpReason;
    _getPumpRuntime = getPumpRuntime;
    _getAutoMode = getAutoMode;
}

void WebServerManager::setControlCallbacks(
    SetPumpFunc setPump,
    SetAutoModeFunc setAutoMode,
    SetThresholdsFunc setThresholds
) {
    _setPump = setPump;
    _setAutoMode = setAutoMode;
    _setThresholds = setThresholds;
}

void WebServerManager::_handleRoot() {
    LOG_DBG(MOD_WEB, "req", "GET /");
    _server.send_P(200, "text/html", INDEX_HTML);
}

void WebServerManager::_handleStatus() {
    LOG_DBG(MOD_WEB, "req", "GET /api/status");
    
    JsonDocument doc;
    
    doc["moisture"] = _getMoisture ? _getMoisture() : 0;
    doc["pump"] = _getPumpState ? _getPumpState() : false;
    doc["reason"] = _getPumpReason ? _getPumpReason() : "none";
    doc["runtime"] = _getPumpRuntime ? _getPumpRuntime() : 0;
    doc["autoMode"] = _getAutoMode ? _getAutoMode() : false;
    
    uint8_t dryVal = _thresholdDry ? *_thresholdDry : 30;
    uint8_t wetVal = _thresholdWet ? *_thresholdWet : 50;
    doc["thresholdDry"] = dryVal;
    doc["thresholdWet"] = wetVal;
    
    LOG_DBG(MOD_WEB, "status", "Returning thresholds: dry=%d, wet=%d", dryVal, wetVal);
    
    doc["uptime"] = millis() / 1000;
    doc["ip"] = WiFi.localIP().toString();
    doc["heap"] = ESP.getFreeHeap();
    
    String json;
    serializeJson(doc, json);
    _sendJson(200, json);
}

void WebServerManager::_handlePump() {
    LOG_DBG(MOD_WEB, "req", "POST /api/pump");
    
    if (!_server.hasArg("plain")) {
        _sendError(400, "No body");
        return;
    }
    
    // Check if in AUTO mode - cannot manually control pump
    bool isAutoMode = _getAutoMode ? _getAutoMode() : false;
    if (isAutoMode) {
        LOG_WRN(MOD_WEB, "pump", "Cannot control pump in AUTO mode!");
        _sendJson(200, "{\"ok\":false,\"error\":\"Đang ở chế độ TỰ ĐỘNG. Chuyển sang THỦ CÔNG để điều khiển bơm.\",\"autoMode\":true}");
        return;
    }
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    
    if (err) {
        LOG_WRN(MOD_WEB, "pump", "JSON parse error: %s", err.c_str());
        _sendError(400, "Invalid JSON");
        return;
    }
    
    const char* action = doc["action"] | "";
    
    if (_setPump) {
        if (strcmp(action, "on") == 0) {
            _setPump(true);
            LOG_INF(MOD_WEB, "pump", "Pump ON via web");
        } else if (strcmp(action, "off") == 0) {
            _setPump(false);
            LOG_INF(MOD_WEB, "pump", "Pump OFF via web");
        } else if (strcmp(action, "toggle") == 0) {
            bool current = _getPumpState ? _getPumpState() : false;
            _setPump(!current);
            LOG_INF(MOD_WEB, "pump", "Pump TOGGLE -> %s via web", !current ? "ON" : "OFF");
        }
    }
    
    // Return current state so UI can update immediately
    bool pumpState = _getPumpState ? _getPumpState() : false;
    String response = "{\"ok\":true,\"pump\":";
    response += pumpState ? "true" : "false";
    response += "}";
    _sendJson(200, response);
}

void WebServerManager::_handleMode() {
    LOG_DBG(MOD_WEB, "req", "POST /api/mode");
    
    if (!_server.hasArg("plain")) {
        _sendError(400, "No body");
        return;
    }
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    
    if (err) {
        _sendError(400, "Invalid JSON");
        return;
    }
    
    if (_setAutoMode) {
        if (doc["toggle"].is<bool>() && doc["toggle"].as<bool>()) {
            bool current = _getAutoMode ? _getAutoMode() : false;
            _setAutoMode(!current);
            LOG_INF(MOD_WEB, "mode", "Mode TOGGLE -> %s via web", !current ? "AUTO" : "MANUAL");
        } else if (doc["mode"].is<const char*>()) {
            const char* mode = doc["mode"];
            _setAutoMode(strcmp(mode, "auto") == 0);
            LOG_INF(MOD_WEB, "mode", "Mode SET -> %s via web", mode);
        }
    }
    
    // Return current state so UI can update immediately
    bool modeState = _getAutoMode ? _getAutoMode() : false;
    String response = "{\"ok\":true,\"autoMode\":";
    response += modeState ? "true" : "false";
    response += "}";
    _sendJson(200, response);
}

void WebServerManager::_handleConfig() {
    LOG_DBG(MOD_WEB, "req", "POST /api/config");
    
    if (!_server.hasArg("plain")) {
        LOG_WRN(MOD_WEB, "config", "No body in request");
        _sendError(400, "No body");
        return;
    }
    
    LOG_DBG(MOD_WEB, "config", "Request body: %s", _server.arg("plain").c_str());
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    
    if (err) {
        LOG_WRN(MOD_WEB, "config", "JSON parse error: %s", err.c_str());
        _sendError(400, "Invalid JSON");
        return;
    }
    
    if (_setThresholds && doc["threshold_dry"].is<int>() && doc["threshold_wet"].is<int>()) {
        uint8_t dry = doc["threshold_dry"];
        uint8_t wet = doc["threshold_wet"];
        
        LOG_DBG(MOD_WEB, "config", "Received: dry=%d, wet=%d", dry, wet);
        
        if (dry < wet && dry >= 0 && wet <= 100) {
            _setThresholds(dry, wet);
            LOG_INF(MOD_WEB, "config", "Thresholds updated: dry=%d, wet=%d", dry, wet);
            
            // Return success with actual values
            String response = "{\"ok\":true,\"dry\":";
            response += dry;
            response += ",\"wet\":";
            response += wet;
            response += "}";
            _sendJson(200, response);
        } else {
            LOG_WRN(MOD_WEB, "config", "Invalid range: dry=%d, wet=%d", dry, wet);
            _sendJson(400, "{\"ok\":false,\"error\":\"Ngưỡng không hợp lệ (phải: 0 <= khô < ướt <= 100)\"}");
            return;
        }
    } else {
        LOG_WRN(MOD_WEB, "config", "Missing threshold parameters");
        _sendJson(400, "{\"ok\":false,\"error\":\"Thiếu tham số ngưỡng\"}");
    }
}

void WebServerManager::_handleSpeed() {
    // Handle GET - return current speed
    if (_server.method() == HTTP_GET) {
        LOG_DBG(MOD_WEB, "req", "GET /api/speed");
        
        uint8_t speed = _getSpeed ? _getSpeed() : 100;
        String response = "{\"speed\":";
        response += speed;
        response += "}";
        _sendJson(200, response);
        return;
    }
    
    // Handle POST - set speed
    LOG_DBG(MOD_WEB, "req", "POST /api/speed");
    
    if (!_server.hasArg("plain")) {
        _sendError(400, "No body");
        return;
    }
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    
    if (err) {
        _sendError(400, "Invalid JSON");
        return;
    }
    
    if (_setSpeed && doc["speed"].is<int>()) {
        uint8_t speed = doc["speed"];
        if (speed >= 30 && speed <= 100) {
            _setSpeed(speed);
            LOG_INF(MOD_WEB, "speed", "Pump speed set to %d%%", speed);
            
            String response = "{\"ok\":true,\"speed\":";
            response += speed;
            response += "}";
            _sendJson(200, response);
        } else {
            _sendError(400, "Speed must be 30-100%");
        }
    } else {
        _sendError(400, "Missing speed parameter");
    }
}

void WebServerManager::setScheduleCallbacks(
    GetScheduleConfigFunc getSchedule,
    SetScheduleEnabledFunc setEnabled,
    SetScheduleEntryFunc setEntry,
    SaveScheduleFunc saveSchedule
) {
    _getSchedule = getSchedule;
    _setScheduleEnabled = setEnabled;
    _setScheduleEntry = setEntry;
    _saveSchedule = saveSchedule;
}

void WebServerManager::_handleSchedule() {
    // Handle GET - return schedule config
    if (_server.method() == HTTP_GET) {
        LOG_DBG(MOD_WEB, "req", "GET /api/schedule");
        
        JsonDocument doc;
        
        if (_getSchedule) {
            WebScheduleConfig config;
            String nextRun;
            _getSchedule(&config, &nextRun);
            
            doc["enabled"] = config.enabled;
            doc["nextRun"] = nextRun;
            
            JsonArray schedules = doc["schedules"].to<JsonArray>();
            for (int i = 0; i < 4; i++) {
                JsonObject s = schedules.add<JsonObject>();
                s["hour"] = config.entries[i].hour;
                s["minute"] = config.entries[i].minute;
                s["duration"] = config.entries[i].duration;
                s["enabled"] = config.entries[i].enabled;
            }
        } else {
            doc["enabled"] = false;
            doc["error"] = "Schedule not available";
        }
        
        String json;
        serializeJson(doc, json);
        _sendJson(200, json);
        return;
    }
    
    // Handle POST - update schedule
    LOG_DBG(MOD_WEB, "req", "POST /api/schedule");
    
    if (!_server.hasArg("plain")) {
        _sendError(400, "No body");
        return;
    }
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    
    if (err) {
        _sendError(400, "Invalid JSON");
        return;
    }
    
    // Handle toggle enabled
    if (doc["toggle"].is<bool>() && doc["toggle"].as<bool>()) {
        if (_setScheduleEnabled) {
            bool enabled = doc["enabled"].as<bool>();
            _setScheduleEnabled(enabled);
            LOG_INF(MOD_WEB, "schedule", "Schedule %s via web", enabled ? "ENABLED" : "DISABLED");
            
            // Save and return new state
            if (_saveSchedule) _saveSchedule();
            
            JsonDocument resp;
            resp["ok"] = true;
            resp["enabled"] = enabled;
            
            if (_getSchedule) {
                WebScheduleConfig config;
                String nextRun;
                _getSchedule(&config, &nextRun);
                resp["nextRun"] = nextRun;
            }
            
            String json;
            serializeJson(resp, json);
            _sendJson(200, json);
            return;
        }
    }
    
    // Handle update schedules
    if (doc["schedules"].is<JsonArray>()) {
        JsonArray schedules = doc["schedules"].as<JsonArray>();
        
        if (_setScheduleEntry) {
            int i = 0;
            for (JsonObject s : schedules) {
                if (i >= 4) break;
                
                uint8_t hour = s["hour"] | 0;
                uint8_t minute = s["minute"] | 0;
                uint16_t duration = s["duration"] | 30;
                bool enabled = s["enabled"] | false;
                
                _setScheduleEntry(i, hour, minute, duration, enabled);
                LOG_INF(MOD_WEB, "schedule", "Entry %d: %02d:%02d dur=%ds en=%d", 
                        i, hour, minute, duration, enabled);
                i++;
            }
            
            // Save changes
            if (_saveSchedule) _saveSchedule();
        }
        
        _sendJson(200, "{\"ok\":true}");
        return;
    }
    
    _sendError(400, "Invalid request");
}

void WebServerManager::_handleNotFound() {
    _sendError(404, "Not found");
}

void WebServerManager::_sendJson(int code, const String& json) {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.send(code, "application/json", json);
}

void WebServerManager::_sendError(int code, const char* message) {
    String json = "{\"error\":\"";
    json += message;
    json += "\"}";
    _sendJson(code, json);
}

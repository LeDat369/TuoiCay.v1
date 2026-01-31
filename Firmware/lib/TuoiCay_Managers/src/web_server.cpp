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
    <title>TuoiCay Dashboard</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body { font-family: Arial, sans-serif; background: #1a1a2e; color: #eee; padding: 20px; }
        .container { max-width: 500px; margin: 0 auto; }
        h1 { color: #00d9ff; text-align: center; margin-bottom: 20px; }
        .card { background: #16213e; border-radius: 10px; padding: 20px; margin-bottom: 15px; }
        .card h2 { color: #00d9ff; font-size: 14px; margin-bottom: 10px; text-transform: uppercase; }
        .value { font-size: 36px; font-weight: bold; color: #fff; }
        .unit { font-size: 18px; color: #888; }
        .status { display: inline-block; padding: 5px 15px; border-radius: 20px; font-weight: bold; }
        .status.on { background: #00c853; color: #fff; }
        .status.off { background: #ff5252; color: #fff; }
        .status.auto { background: #2196f3; color: #fff; }
        .status.manual { background: #ff9800; color: #fff; }
        .btn { display: block; width: 100%; padding: 15px; border: none; border-radius: 8px; font-size: 16px; font-weight: bold; cursor: pointer; margin-top: 10px; }
        .btn-pump { background: #00d9ff; color: #1a1a2e; }
        .btn-mode { background: #7c4dff; color: #fff; }
        .btn:active { transform: scale(0.98); }
        .row { display: flex; gap: 15px; }
        .row .card { flex: 1; }
        .config { display: flex; align-items: center; gap: 10px; margin-top: 10px; }
        .config input { flex: 1; padding: 10px; border: 1px solid #333; border-radius: 5px; background: #0f0f23; color: #fff; }
        .info { font-size: 12px; color: #666; text-align: center; margin-top: 20px; }
    </style>
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
            <h2>Cài đặt ngưỡng</h2>
            <div class="config">
                <label>Khô:</label>
                <input type="number" id="dryThreshold" min="0" max="100" value="30">
                <label>Ướt:</label>
                <input type="number" id="wetThreshold" min="0" max="100" value="50">
                <button class="btn" style="width:auto; padding:10px 20px;" onclick="saveConfig()">Lưu</button>
            </div>
        </div>
        
        <div class="info">
            Uptime: <span id="uptime">--</span>s | IP: <span id="ip">--</span>
        </div>
    </div>
    
    <script>
        function fetchStatus() {
            fetch('/api/status')
                .then(r => r.json())
                .then(d => {
                    document.getElementById('moisture').textContent = d.moisture;
                    
                    const ps = document.getElementById('pumpStatus');
                    ps.textContent = d.pump ? 'ON' : 'OFF';
                    ps.className = 'status ' + (d.pump ? 'on' : 'off');
                    
                    const info = d.pump ? `${d.reason} - ${d.runtime}s` : '';
                    document.getElementById('pumpInfo').textContent = info;
                    
                    const ms = document.getElementById('modeStatus');
                    ms.textContent = d.autoMode ? 'AUTO' : 'MANUAL';
                    ms.className = 'status ' + (d.autoMode ? 'auto' : 'manual');
                    
                    document.getElementById('dryThreshold').value = d.thresholdDry;
                    document.getElementById('wetThreshold').value = d.thresholdWet;
                    document.getElementById('uptime').textContent = d.uptime;
                    document.getElementById('ip').textContent = d.ip;
                })
                .catch(e => console.error('Error:', e));
        }
        
        function togglePump() {
            fetch('/api/pump', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({action: 'toggle'})
            }).then(() => setTimeout(fetchStatus, 200));
        }
        
        function toggleMode() {
            fetch('/api/mode', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({toggle: true})
            }).then(() => setTimeout(fetchStatus, 200));
        }
        
        function saveConfig() {
            const dry = parseInt(document.getElementById('dryThreshold').value);
            const wet = parseInt(document.getElementById('wetThreshold').value);
            fetch('/api/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({threshold_dry: dry, threshold_wet: wet})
            }).then(() => { alert('Đã lưu!'); fetchStatus(); });
        }
        
        fetchStatus();
        setInterval(fetchStatus, 5000);
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
    , _thresholdDry(nullptr)
    , _thresholdWet(nullptr)
{
}

bool WebServerManager::begin() {
    // Set up routes
    _server.on("/", HTTP_GET, [this]() { _handleRoot(); });
    _server.on("/api/status", HTTP_GET, [this]() { _handleStatus(); });
    _server.on("/api/pump", HTTP_POST, [this]() { _handlePump(); });
    _server.on("/api/mode", HTTP_POST, [this]() { _handleMode(); });
    _server.on("/api/config", HTTP_POST, [this]() { _handleConfig(); });
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
    doc["thresholdDry"] = _thresholdDry ? *_thresholdDry : 30;
    doc["thresholdWet"] = _thresholdWet ? *_thresholdWet : 50;
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
        } else if (strcmp(action, "off") == 0) {
            _setPump(false);
        } else if (strcmp(action, "toggle") == 0) {
            bool current = _getPumpState ? _getPumpState() : false;
            _setPump(!current);
        }
    }
    
    _sendJson(200, "{\"ok\":true}");
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
        } else if (doc["mode"].is<const char*>()) {
            const char* mode = doc["mode"];
            _setAutoMode(strcmp(mode, "auto") == 0);
        }
    }
    
    _sendJson(200, "{\"ok\":true}");
}

void WebServerManager::_handleConfig() {
    LOG_DBG(MOD_WEB, "req", "POST /api/config");
    
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
    
    if (_setThresholds && doc["threshold_dry"].is<int>() && doc["threshold_wet"].is<int>()) {
        uint8_t dry = doc["threshold_dry"];
        uint8_t wet = doc["threshold_wet"];
        
        if (dry < wet && dry >= 0 && wet <= 100) {
            _setThresholds(dry, wet);
            LOG_INF(MOD_WEB, "config", "Thresholds updated: dry=%d, wet=%d", dry, wet);
        } else {
            _sendError(400, "Invalid thresholds");
            return;
        }
    }
    
    _sendJson(200, "{\"ok\":true}");
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

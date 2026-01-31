/**
 * @file captive_portal.cpp
 * @brief WiFi Provisioning via Captive Portal - Implementation
 * @version 1.0.0
 */

#include "captive_portal.h"
#include <logger.h>

// Logger module name
#define MOD_PORTAL  "PORTAL"

//=============================================================================
// CONSTRUCTOR
//=============================================================================

CaptivePortal::CaptivePortal()
    : _server(nullptr)
    , _isActive(false)
    , _hasConfig(false)
    , _startTime(0)
    , _timeout(CAPTIVE_PORTAL_TIMEOUT)
    , _mqttPort(1883)
    , _onCredentials(nullptr)
    , _onMqttConfig(nullptr)
    , _onTimeout(nullptr)
    , _scanResultCount(0)
    , _lastScanTime(0)
{
}

//=============================================================================
// BEGIN / STOP
//=============================================================================

bool CaptivePortal::begin(const char* apSSID, const char* apPassword) {
    if (_isActive) {
        LOG_WRN(MOD_PORTAL, "begin", "Already active");
        return false;
    }
    
    LOG_INF(MOD_PORTAL, "begin", "Starting Captive Portal...");
    LOG_INF(MOD_PORTAL, "begin", "AP SSID: %s", apSSID);
    
    // Stop any existing WiFi connection
    WiFi.disconnect(true);
    delay(100);
    
    // Start SoftAP
    WiFi.mode(WIFI_AP);
    
    bool apStarted;
    if (apPassword && strlen(apPassword) >= 8) {
        apStarted = WiFi.softAP(apSSID, apPassword);
    } else {
        apStarted = WiFi.softAP(apSSID);
    }
    
    if (!apStarted) {
        LOG_ERR(MOD_PORTAL, "begin", "Failed to start AP");
        return false;
    }
    
    // Configure AP IP
    IPAddress apIP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, gateway, subnet);
    
    LOG_INF(MOD_PORTAL, "begin", "AP IP: %s", WiFi.softAPIP().toString().c_str());
    
    // Start DNS server for captive portal redirect
    _dnsServer.start(DNS_PORT, "*", apIP);
    
    // Start web server
    _server = new ESP8266WebServer(80);
    
    // Setup routes
    _server->on("/", HTTP_GET, std::bind(&CaptivePortal::_handleRoot, this));
    _server->on("/scan", HTTP_GET, std::bind(&CaptivePortal::_handleScan, this));
    _server->on("/save", HTTP_POST, std::bind(&CaptivePortal::_handleSave, this));
    _server->on("/status", HTTP_GET, std::bind(&CaptivePortal::_handleStatus, this));
    _server->on("/generate_204", HTTP_GET, std::bind(&CaptivePortal::_handleRoot, this));  // Android captive portal
    _server->on("/fwlink", HTTP_GET, std::bind(&CaptivePortal::_handleRoot, this));        // Microsoft captive portal
    _server->onNotFound(std::bind(&CaptivePortal::_handleNotFound, this));
    
    _server->begin();
    
    _isActive = true;
    _hasConfig = false;
    _startTime = millis();
    
    // Start initial WiFi scan
    WiFi.scanNetworks(true);
    
    LOG_INF(MOD_PORTAL, "begin", "Captive Portal started");
    return true;
}

void CaptivePortal::stop() {
    if (!_isActive) return;
    
    LOG_INF(MOD_PORTAL, "stop", "Stopping Captive Portal...");
    
    // Stop servers
    _dnsServer.stop();
    
    if (_server) {
        _server->stop();
        delete _server;
        _server = nullptr;
    }
    
    // Stop AP
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    
    _isActive = false;
    
    LOG_INF(MOD_PORTAL, "stop", "Captive Portal stopped");
}

//=============================================================================
// UPDATE
//=============================================================================

void CaptivePortal::update() {
    if (!_isActive) return;
    
    // Process DNS requests
    _dnsServer.processNextRequest();
    
    // Handle HTTP requests
    if (_server) {
        _server->handleClient();
    }
    
    // Check WiFi scan completion
    int8_t scanResult = WiFi.scanComplete();
    if (scanResult >= 0) {
        _scanResultCount = scanResult;
        _lastScanTime = millis();
    }
    
    // Check timeout
    if (_timeout > 0 && (millis() - _startTime) > _timeout) {
        LOG_WRN(MOD_PORTAL, "update", "Timeout - no configuration received");
        if (_onTimeout) {
            _onTimeout();
        }
        stop();
    }
}

//=============================================================================
// HTTP HANDLERS
//=============================================================================

void CaptivePortal::_handleRoot() {
    LOG_DBG(MOD_PORTAL, "http", "Serving config page");
    _server->send(200, "text/html", _generateConfigPage());
}

void CaptivePortal::_handleScan() {
    LOG_DBG(MOD_PORTAL, "http", "Serving scan results");
    
    // Trigger new scan if results are old
    if (millis() - _lastScanTime > 10000) {
        WiFi.scanNetworks(true);
    }
    
    _server->send(200, "application/json", _generateScanResultsJSON());
}

void CaptivePortal::_handleSave() {
    LOG_INF(MOD_PORTAL, "save", "Processing configuration...");
    
    // Get WiFi credentials
    _configuredSSID = _server->arg("ssid");
    _configuredPassword = _server->arg("password");
    
    // Get MQTT config (optional)
    _mqttServer = _server->arg("mqtt_server");
    String portStr = _server->arg("mqtt_port");
    _mqttPort = portStr.length() > 0 ? portStr.toInt() : 1883;
    _mqttUser = _server->arg("mqtt_user");
    _mqttPass = _server->arg("mqtt_pass");
    
    // Validate
    if (_configuredSSID.length() == 0) {
        _server->send(400, "text/html", 
            "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
            "<meta http-equiv='refresh' content='3;url=/'></head>"
            "<body><h1>Lỗi: Chưa chọn mạng WiFi</h1></body></html>");
        return;
    }
    
    LOG_INF(MOD_PORTAL, "save", "WiFi SSID: %s", _configuredSSID.c_str());
    if (_mqttServer.length() > 0) {
        LOG_INF(MOD_PORTAL, "save", "MQTT Server: %s:%d", _mqttServer.c_str(), _mqttPort);
    }
    
    _hasConfig = true;
    
    // Call callbacks
    if (_onCredentials) {
        _onCredentials(_configuredSSID, _configuredPassword);
    }
    
    if (_mqttServer.length() > 0 && _onMqttConfig) {
        _onMqttConfig(_mqttServer, _mqttPort, _mqttUser, _mqttPass);
    }
    
    // Send success page
    _server->send(200, "text/html", _generateSuccessPage());
}

void CaptivePortal::_handleStatus() {
    String json = "{";
    json += "\"active\":" + String(_isActive ? "true" : "false") + ",";
    json += "\"hasConfig\":" + String(_hasConfig ? "true" : "false") + ",";
    json += "\"uptime\":" + String((millis() - _startTime) / 1000) + ",";
    json += "\"stations\":" + String(getStationCount());
    json += "}";
    
    _server->send(200, "application/json", json);
}

void CaptivePortal::_handleNotFound() {
    // Redirect all unknown URLs to root (captive portal behavior)
    _server->sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    _server->send(302, "text/plain", "");
}

//=============================================================================
// HTML GENERATION
//=============================================================================

String CaptivePortal::_getCSS() {
    return R"rawliteral(
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Oxygen,Ubuntu,sans-serif;
background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;padding:20px}
.container{max-width:400px;margin:0 auto;background:#fff;border-radius:20px;padding:30px;box-shadow:0 10px 40px rgba(0,0,0,0.2)}
h1{color:#333;text-align:center;margin-bottom:10px;font-size:24px}
.subtitle{color:#666;text-align:center;margin-bottom:25px;font-size:14px}
.icon{text-align:center;font-size:60px;margin-bottom:15px}
label{display:block;color:#555;margin-bottom:5px;font-weight:500;font-size:14px}
input,select{width:100%;padding:12px 15px;border:2px solid #e0e0e0;border-radius:10px;font-size:16px;
margin-bottom:15px;transition:border-color 0.3s}
input:focus,select:focus{outline:none;border-color:#667eea}
.btn{width:100%;padding:15px;border:none;border-radius:10px;font-size:16px;font-weight:600;
cursor:pointer;transition:transform 0.2s,box-shadow 0.2s}
.btn-primary{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff}
.btn-primary:hover{transform:translateY(-2px);box-shadow:0 5px 20px rgba(102,126,234,0.4)}
.btn-secondary{background:#f0f0f0;color:#333;margin-top:10px}
.networks{max-height:200px;overflow-y:auto;margin-bottom:15px;border:2px solid #e0e0e0;border-radius:10px}
.network{padding:12px 15px;border-bottom:1px solid #eee;cursor:pointer;display:flex;justify-content:space-between;align-items:center}
.network:hover{background:#f5f5f5}
.network:last-child{border-bottom:none}
.signal{color:#667eea;font-size:12px}
.section{margin-top:20px;padding-top:20px;border-top:1px solid #eee}
.section h2{font-size:16px;color:#333;margin-bottom:15px}
.optional{color:#999;font-size:12px;font-weight:normal}
.loading{text-align:center;padding:20px;color:#666}
</style>
)rawliteral";
}

String CaptivePortal::_generateConfigPage() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Cấu hình TuoiCay</title>
)rawliteral";
    
    html += _getCSS();
    
    html += R"rawliteral(
</head>
<body>
<div class="container">
<div class="icon">🌱</div>
<h1>TuoiCay Setup</h1>
<p class="subtitle">Cấu hình kết nối WiFi cho thiết bị</p>

<form id="configForm" action="/save" method="POST">

<label>Mạng WiFi</label>
<div class="networks" id="networks">
<div class="loading">🔍 Đang quét mạng WiFi...</div>
</div>
<input type="hidden" name="ssid" id="ssid">

<label>Mật khẩu WiFi</label>
<input type="password" name="password" id="password" placeholder="Nhập mật khẩu WiFi">

<div class="section">
<h2>MQTT Server <span class="optional">(không bắt buộc)</span></h2>
<label>Địa chỉ Server</label>
<input type="text" name="mqtt_server" placeholder="192.168.1.100">
<label>Cổng</label>
<input type="number" name="mqtt_port" value="1883" placeholder="1883">
<label>Username</label>
<input type="text" name="mqtt_user" placeholder="Để trống nếu không có">
<label>Password</label>
<input type="password" name="mqtt_pass" placeholder="Để trống nếu không có">
</div>

<button type="submit" class="btn btn-primary">💾 Lưu cấu hình</button>
<button type="button" class="btn btn-secondary" onclick="scanNetworks()">🔄 Quét lại</button>

</form>
</div>

<script>
let selectedSSID = '';

function scanNetworks() {
    document.getElementById('networks').innerHTML = '<div class="loading">🔍 Đang quét...</div>';
    fetch('/scan')
        .then(r => r.json())
        .then(data => {
            let html = '';
            if (data.length === 0) {
                html = '<div class="loading">Không tìm thấy mạng WiFi</div>';
            } else {
                data.forEach(n => {
                    const signal = n.rssi > -50 ? '📶' : (n.rssi > -70 ? '📶' : '📶');
                    const lock = n.secure ? '🔒' : '';
                    html += `<div class="network" onclick="selectNetwork('${n.ssid.replace(/'/g, "\\'")}')">
                        <span>${n.ssid} ${lock}</span>
                        <span class="signal">${signal} ${n.rssi}dBm</span>
                    </div>`;
                });
            }
            document.getElementById('networks').innerHTML = html;
        })
        .catch(e => {
            document.getElementById('networks').innerHTML = '<div class="loading">Lỗi quét mạng</div>';
        });
}

function selectNetwork(ssid) {
    selectedSSID = ssid;
    document.getElementById('ssid').value = ssid;
    document.querySelectorAll('.network').forEach(el => {
        el.style.background = el.textContent.includes(ssid) ? '#e8f4fd' : '';
    });
}

document.getElementById('configForm').onsubmit = function(e) {
    if (!document.getElementById('ssid').value) {
        alert('Vui lòng chọn mạng WiFi!');
        e.preventDefault();
        return false;
    }
    return true;
};

// Auto scan on load
setTimeout(scanNetworks, 500);
</script>
</body>
</html>
)rawliteral";
    
    return html;
}

String CaptivePortal::_generateSuccessPage() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Cấu hình thành công</title>
)rawliteral";
    
    html += _getCSS();
    
    html += R"rawliteral(
<style>
.success{color:#27ae60}
.info{background:#e8f5e9;border-radius:10px;padding:15px;margin:20px 0;font-size:14px}
</style>
</head>
<body>
<div class="container">
<div class="icon">✅</div>
<h1 class="success">Cấu hình thành công!</h1>
<p class="subtitle">Thiết bị sẽ khởi động lại và kết nối WiFi</p>

<div class="info">
<strong>Mạng WiFi:</strong> )rawliteral";
    
    html += _escapeHTML(_configuredSSID);
    
    html += R"rawliteral(<br>
<strong>Trạng thái:</strong> Đang kết nối...
</div>

<p style="text-align:center;color:#666;font-size:14px">
Thiết bị sẽ tự động khởi động lại trong vài giây.<br>
Sau khi khởi động, bạn có thể truy cập thiết bị qua địa chỉ IP mới.
</p>
</div>

<script>
setTimeout(function(){
    document.querySelector('.subtitle').textContent = 'Đang khởi động lại...';
}, 3000);
</script>
</body>
</html>
)rawliteral";
    
    return html;
}

String CaptivePortal::_generateScanResultsJSON() {
    String json = "[";
    
    int n = WiFi.scanComplete();
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            if (i > 0) json += ",";
            json += "{";
            json += "\"ssid\":\"" + _escapeHTML(WiFi.SSID(i)) + "\",";
            json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
            json += "\"secure\":" + String(WiFi.encryptionType(i) != ENC_TYPE_NONE ? "true" : "false");
            json += "}";
        }
    }
    
    json += "]";
    return json;
}

//=============================================================================
// HELPERS
//=============================================================================

String CaptivePortal::getAPIP() const {
    if (!_isActive) return "";
    return WiFi.softAPIP().toString();
}

uint8_t CaptivePortal::getStationCount() const {
    if (!_isActive) return 0;
    return WiFi.softAPgetStationNum();
}

String CaptivePortal::_escapeHTML(const String& str) {
    String escaped = str;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\"", "&quot;");
    escaped.replace("'", "&#39;");
    return escaped;
}

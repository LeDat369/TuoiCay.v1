/**
 * @file wifi_manager.cpp
 * @brief Implementation of WiFi Connection Manager
 * 
 * LOGIC:
 * - Non-blocking connection using WiFi.begin() and status checking
 * - Exponential backoff: 2s -> 4s -> 8s -> 16s -> 30s (max)
 * - LED blink while connecting, solid when connected
 * 
 * RULES: #WIFI(8) #ERROR(6)
 */

#include "wifi_manager.h"
#include <pins.h>
#include <logger.h>
#include <error_codes.h>

//=============================================================================
// WIFI MANAGER IMPLEMENTATION
//=============================================================================

WiFiManager::WiFiManager()
    : _state(TCWiFiState::IDLE)
    , _callback(nullptr)
    , _connectStartTime(0)
    , _lastReconnectTime(0)
    , _reconnectDelay(WIFI_RECONNECT_MIN_MS)
    , _reconnectCount(0)
    , _ledPin(-1)
    , _initialized(false)
{
}

bool WiFiManager::begin(const char* ssid, const char* password) {
    if (ssid == nullptr || strlen(ssid) == 0) {
        LOG_ERR(MOD_WIFI, "begin", "SSID is empty!");
        return false;
    }
    
    _ssid = ssid;
    _password = password ? password : "";
    
    // Configure WiFi
    WiFi.mode(WIFI_STA);
    WiFi.setAutoConnect(false);     // We handle reconnect
    WiFi.setAutoReconnect(false);   // We handle reconnect
    WiFi.persistent(false);         // Don't save to flash
    
    _initialized = true;
    _state = TCWiFiState::IDLE;
    
    LOG_INF(MOD_WIFI, "init", "WiFi manager ready, SSID=%s", ssid);
    
    return true;
}

bool WiFiManager::connect() {
    if (!_initialized) {
        LOG_ERR(MOD_WIFI, "conn", "Not initialized!");
        return false;
    }
    
    if (_state == TCWiFiState::CONNECTED) {
        LOG_WRN(MOD_WIFI, "conn", "Already connected");
        return true;
    }
    
    LOG_INF(MOD_WIFI, "conn", "Connecting to %s...", _ssid.c_str());
    
    WiFi.begin(_ssid.c_str(), _password.c_str());
    
    _connectStartTime = millis();
    _setState(TCWiFiState::CONNECTING);
    
    return true;
}

void WiFiManager::disconnect() {
    LOG_INF(MOD_WIFI, "disc", "Disconnecting...");
    WiFi.disconnect(true);
    _setState(TCWiFiState::IDLE);
    _reconnectCount = 0;
    _reconnectDelay = WIFI_RECONNECT_MIN_MS;
}

void WiFiManager::update() {
    if (!_initialized) return;
    
    unsigned long now = millis();
    
    // Update LED
    _updateLED();
    
    switch (_state) {
        case TCWiFiState::CONNECTING: {
            // Check if connected
            if (WiFi.status() == WL_CONNECTED) {
                _reconnectCount = 0;
                _reconnectDelay = WIFI_RECONNECT_MIN_MS;
                _setState(TCWiFiState::CONNECTED);
                
                LOG_INF(MOD_WIFI, "conn", "Connected! IP=%s, RSSI=%d dBm",
                        getIPString().c_str(), getRSSI());
                break;
            }
            
            // Check for timeout
            if (now - _connectStartTime >= WIFI_CONNECT_TIMEOUT_MS) {
                LOG_WRN(MOD_WIFI, "conn", "Connection timeout after %lums",
                        WIFI_CONNECT_TIMEOUT_MS);
                
                WiFi.disconnect(true);
                _reconnectCount++;
                _calculateBackoff();
                _lastReconnectTime = now;
                _setState(TCWiFiState::DISCONNECTED);
            }
            break;
        }
        
        case TCWiFiState::CONNECTED: {
            // Check if still connected
            if (WiFi.status() != WL_CONNECTED) {
                LOG_WRN(MOD_WIFI, "conn", "Connection lost!");
                _lastReconnectTime = now;
                _setState(TCWiFiState::DISCONNECTED);
            }
            break;
        }
        
        case TCWiFiState::DISCONNECTED: {
            // Auto-reconnect with backoff
            if (now - _lastReconnectTime >= _reconnectDelay) {
                LOG_INF(MOD_WIFI, "reconn", "Attempting reconnect #%d (delay=%lums)",
                        _reconnectCount + 1, _reconnectDelay);
                connect();
            }
            break;
        }
        
        case TCWiFiState::IDLE:
        default:
            break;
    }
}

bool WiFiManager::isConnected() const {
    return _state == TCWiFiState::CONNECTED && WiFi.status() == WL_CONNECTED;
}

const char* WiFiManager::getStateString() const {
    switch (_state) {
        case TCWiFiState::IDLE:         return "IDLE";
        case TCWiFiState::CONNECTING:   return "CONNECTING";
        case TCWiFiState::CONNECTED:    return "CONNECTED";
        case TCWiFiState::DISCONNECTED: return "DISCONNECTED";
        default:                        return "UNKNOWN";
    }
}

IPAddress WiFiManager::getIP() const {
    return WiFi.localIP();
}

String WiFiManager::getIPString() const {
    return WiFi.localIP().toString();
}

int32_t WiFiManager::getRSSI() const {
    return WiFi.RSSI();
}

String WiFiManager::getMACString() const {
    return WiFi.macAddress();
}

String WiFiManager::getDeviceId() const {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    return mac;
}

void WiFiManager::setStatusLED(int8_t pin) {
    _ledPin = pin;
    if (pin >= 0) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LED_OFF);
    }
}

void WiFiManager::_setState(TCWiFiState newState) {
    if (_state != newState) {
        TCWiFiState oldState = _state;
        _state = newState;
        
        LOG_DBG(MOD_WIFI, "state", "%s -> %s",
                oldState == TCWiFiState::IDLE ? "IDLE" :
                oldState == TCWiFiState::CONNECTING ? "CONNECTING" :
                oldState == TCWiFiState::CONNECTED ? "CONNECTED" : "DISCONNECTED",
                getStateString());
        
        if (_callback) {
            _callback(newState);
        }
    }
}

void WiFiManager::_updateLED() {
    if (_ledPin < 0) return;
    
    switch (_state) {
        case TCWiFiState::CONNECTING: {
            // Blink fast (100ms on/off)
            bool on = ((millis() / 100) % 2) == 0;
            digitalWrite(_ledPin, on ? LED_ON : LED_OFF);
            break;
        }
        
        case TCWiFiState::CONNECTED:
            // Solid on
            digitalWrite(_ledPin, LED_ON);
            break;
        
        case TCWiFiState::DISCONNECTED: {
            // Blink slow (500ms on/off)
            bool on = ((millis() / 500) % 2) == 0;
            digitalWrite(_ledPin, on ? LED_ON : LED_OFF);
            break;
        }
        
        default:
            // Off
            digitalWrite(_ledPin, LED_OFF);
            break;
    }
}

void WiFiManager::_calculateBackoff() {
    // Exponential backoff: 2s -> 4s -> 8s -> 16s -> 30s (max)
    _reconnectDelay *= 2;
    if (_reconnectDelay > WIFI_RECONNECT_MAX_MS) {
        _reconnectDelay = WIFI_RECONNECT_MAX_MS;
    }
}

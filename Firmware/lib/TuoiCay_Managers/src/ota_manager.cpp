/**
 * @file ota_manager.cpp
 * @brief Implementation of OTA Manager
 * 
 * LOGIC:
 * - Uses ArduinoOTA library
 * - Callbacks for start, progress, end, error
 * - LED indication during update
 * 
 * RULES: #OTA
 */

#include "ota_manager.h"
#include <logger.h>
#include <pins.h>

// Global instance
OtaManager otaManager;

//=============================================================================
// OTA MANAGER IMPLEMENTATION
//=============================================================================

bool OtaManager::begin(const char* hostname, const char* password) {
    if (_initialized) return true;
    
    LOG_INF(MOD_OTA, "init", "Initializing OTA...");
    
    // Set hostname
    ArduinoOTA.setHostname(hostname);
    
    // Set password if provided
    if (password != nullptr && strlen(password) > 0) {
        ArduinoOTA.setPassword(password);
        LOG_INF(MOD_OTA, "init", "Password protection enabled");
    }
    
    // Setup callbacks
    _setupCallbacks();
    
    // Start OTA
    ArduinoOTA.begin();
    
    _initialized = true;
    _updating = false;
    _progress = 0;
    
    LOG_INF(MOD_OTA, "init", "OTA ready, hostname=%s", hostname);
    return true;
}

void OtaManager::update() {
    if (!_initialized) return;
    ArduinoOTA.handle();
}

void OtaManager::_setupCallbacks() {
    // On start
    ArduinoOTA.onStart([this]() {
        _updating = true;
        _progress = 0;
        
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "firmware";
        } else {  // U_FS
            type = "filesystem";
        }
        
        LOG_INF(MOD_OTA, "start", "Update starting (%s)", type.c_str());
        
        // Turn on LED to indicate update
        digitalWrite(PIN_LED_STATUS, LED_ON);
    });
    
    // On end
    ArduinoOTA.onEnd([this]() {
        _updating = false;
        _progress = 100;
        
        LOG_INF(MOD_OTA, "done", "Update complete!");
        
        // Blink LED
        for (int i = 0; i < 5; i++) {
            digitalWrite(PIN_LED_STATUS, LED_OFF);
            delay(100);
            digitalWrite(PIN_LED_STATUS, LED_ON);
            delay(100);
        }
    });
    
    // On progress
    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
        _progress = (progress * 100) / total;
        
        // Log every 10%
        static uint8_t lastLogged = 0;
        if (_progress / 10 > lastLogged / 10) {
            LOG_INF(MOD_OTA, "prog", "Progress: %u%%", _progress);
            lastLogged = _progress;
        }
        
        // Blink LED during update
        digitalWrite(PIN_LED_STATUS, (_progress % 2) ? LED_ON : LED_OFF);
    });
    
    // On error
    ArduinoOTA.onError([this](ota_error_t error) {
        _updating = false;
        
        const char* errStr;
        switch (error) {
            case OTA_AUTH_ERROR:    errStr = "Auth failed"; break;
            case OTA_BEGIN_ERROR:   errStr = "Begin failed"; break;
            case OTA_CONNECT_ERROR: errStr = "Connect failed"; break;
            case OTA_RECEIVE_ERROR: errStr = "Receive failed"; break;
            case OTA_END_ERROR:     errStr = "End failed"; break;
            default:                errStr = "Unknown"; break;
        }
        
        LOG_ERR(MOD_OTA, "error", "Update failed: %s (%d)", errStr, error);
        
        // Fast blink to indicate error
        for (int i = 0; i < 10; i++) {
            digitalWrite(PIN_LED_STATUS, LED_OFF);
            delay(50);
            digitalWrite(PIN_LED_STATUS, LED_ON);
            delay(50);
        }
    });
}

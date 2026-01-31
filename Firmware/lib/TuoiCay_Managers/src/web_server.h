/**
 * @file web_server.h
 * @brief HTTP Web Server for control and monitoring
 * 
 * LOGIC:
 * - REST API endpoints for status and control
 * - HTML dashboard served from PROGMEM or LittleFS
 * - JSON responses for API calls
 * 
 * ENDPOINTS:
 * - GET /           -> HTML dashboard
 * - GET /api/status -> JSON status
 * - POST /api/pump  -> Pump control
 * - POST /api/mode  -> Mode control
 * - POST /api/config -> Configuration
 * 
 * RULES: #HTTP(24) #JSON(23)
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <config.h>

// Forward declarations
class SensorManager;
class PumpController;

//=============================================================================
// CALLBACK TYPES FOR GETTING DATA
//=============================================================================
typedef uint8_t (*GetMoistureFunc)();
typedef bool (*GetPumpStateFunc)();
typedef const char* (*GetPumpReasonFunc)();
typedef uint16_t (*GetPumpRuntimeFunc)();
typedef bool (*GetAutoModeFunc)();

typedef void (*SetPumpFunc)(bool on);
typedef void (*SetAutoModeFunc)(bool enabled);
typedef void (*SetThresholdsFunc)(uint8_t dry, uint8_t wet);

// Speed control callbacks
typedef uint8_t (*GetPumpSpeedFunc)();
typedef void (*SetPumpSpeedFunc)(uint8_t percent);

// Schedule callbacks
struct WebScheduleEntry {
    uint8_t hour;
    uint8_t minute;
    uint16_t duration;
    bool enabled;
};

struct WebScheduleConfig {
    bool enabled;
    WebScheduleEntry entries[4];
};

typedef bool (*GetScheduleConfigFunc)(WebScheduleConfig* config, String* nextRun);
typedef void (*SetScheduleEnabledFunc)(bool enabled);
typedef void (*SetScheduleEntryFunc)(uint8_t index, uint8_t hour, uint8_t minute, uint16_t duration, bool enabled);
typedef void (*SaveScheduleFunc)();

//=============================================================================
// WEB SERVER CLASS
//=============================================================================

/**
 * @class WebServer
 * @brief HTTP server for control and monitoring
 */
class WebServerManager {
public:
    /**
     * @brief Constructor
     * @param port HTTP port (default 80)
     */
    WebServerManager(uint16_t port = 80);
    
    /**
     * @brief Initialize web server and set up routes
     * @return true if successful
     */
    bool begin();
    
    /**
     * @brief Handle incoming requests (call in loop)
     */
    void update();
    
    /**
     * @brief Stop web server
     */
    void stop();
    
    /**
     * @brief Check if server is running
     */
    bool isRunning() const { return _running; }
    
    /**
     * @brief Set data providers (callbacks to get current values)
     */
    void setDataProviders(
        GetMoistureFunc getMoisture,
        GetPumpStateFunc getPumpState,
        GetPumpReasonFunc getPumpReason,
        GetPumpRuntimeFunc getPumpRuntime,
        GetAutoModeFunc getAutoMode
    );
    
    /**
     * @brief Set control callbacks
     */
    void setControlCallbacks(
        SetPumpFunc setPump,
        SetAutoModeFunc setAutoMode,
        SetThresholdsFunc setThresholds
    );
    
    /**
     * @brief Get thresholds
     */
    void setThresholdPointers(uint8_t* dry, uint8_t* wet) {
        _thresholdDry = dry;
        _thresholdWet = wet;
    }
    
    /**
     * @brief Set speed control callbacks
     */
    void setSpeedCallbacks(GetPumpSpeedFunc getSpeed, SetPumpSpeedFunc setSpeed) {
        _getSpeed = getSpeed;
        _setSpeed = setSpeed;
    }
    
    /**
     * @brief Set schedule callbacks
     */
    void setScheduleCallbacks(
        GetScheduleConfigFunc getSchedule,
        SetScheduleEnabledFunc setEnabled,
        SetScheduleEntryFunc setEntry,
        SaveScheduleFunc saveSchedule
    );

private:
    ESP8266WebServer _server;
    bool _running;
    
    // Data providers
    GetMoistureFunc _getMoisture;
    GetPumpStateFunc _getPumpState;
    GetPumpReasonFunc _getPumpReason;
    GetPumpRuntimeFunc _getPumpRuntime;
    GetAutoModeFunc _getAutoMode;
    
    // Control callbacks
    SetPumpFunc _setPump;
    SetAutoModeFunc _setAutoMode;
    SetThresholdsFunc _setThresholds;
    
    // Speed callbacks
    GetPumpSpeedFunc _getSpeed;
    SetPumpSpeedFunc _setSpeed;
    
    // Threshold pointers
    uint8_t* _thresholdDry;
    uint8_t* _thresholdWet;
    
    // Schedule callbacks
    GetScheduleConfigFunc _getSchedule;
    SetScheduleEnabledFunc _setScheduleEnabled;
    SetScheduleEntryFunc _setScheduleEntry;
    SaveScheduleFunc _saveSchedule;
    
    // Route handlers
    void _handleRoot();
    void _handleStatus();
    void _handlePump();
    void _handleMode();
    void _handleConfig();
    void _handleSpeed();
    void _handleSchedule();
    void _handleNotFound();
    
    /**
     * @brief Send JSON response
     */
    void _sendJson(int code, const String& json);
    
    /**
     * @brief Send error response
     */
    void _sendError(int code, const char* message);
};

#endif // WEB_SERVER_H

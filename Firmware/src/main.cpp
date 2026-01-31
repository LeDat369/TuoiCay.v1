/**
 * @file main.cpp
 * @brief Entry point for TuoiCay Automatic Plant Watering System
 * 
 * LOGIC:
 * - setup(): Initialize all components in safe order
 *   1. GPIO safe state (pump OFF first!)
 *   2. Serial/Logger
 *   3. Watchdog timer
 *   4. Boot reason detection
 *   5. Other peripherals (TODO in later tasks)
 * 
 * - loop(): Main execution cycle (non-blocking!)
 *   1. Feed watchdog
 *   2. Check sensors (TODO)
 *   3. Update pump (TODO)
 *   4. Handle WiFi/MQTT (TODO)
 *   5. Handle web requests (TODO)
 * 
 * RULES: #CORE(1.2) #SAFETY(2) #GPIO(11)
 */

#include <Arduino.h>
#include <Ticker.h>
#include <user_interface.h>     // ESP8266 SDK for rst_info

// Project headers
#include <config.h>
#include <pins.h>
#include <error_codes.h>
#include <logger.h>

// Drivers
#include <sensor_driver.h>
#include <pump_driver.h>

// Managers
#include <wifi_manager.h>
#include <web_server.h>
#include <mqtt_manager.h>
#include <storage_manager.h>
#include <ota_manager.h>
#include <time_manager.h>
#include <scheduler.h>
#include <captive_portal.h>

// JSON for MQTT payloads
#include <ArduinoJson.h>

// Secrets (WiFi credentials)
#include <secrets.h>

//=============================================================================
// FORWARD DECLARATIONS
//=============================================================================
void watchdog_init();
void watchdog_feed();
void print_boot_reason();

//=============================================================================
// GLOBAL VARIABLES
//=============================================================================
Ticker wdtTicker;                       // Software watchdog ticker
volatile bool wdtFlag = false;          // Watchdog feed flag

// Drivers (TASK 2.1, 2.2)
SensorManager sensors;                  // Soil moisture sensors
PumpController pump(PIN_PUMP);          // Pump controller

// Managers (TASK 3.1, 3.2, 4.1, 5.2)
WiFiManager wifiMgr;                    // WiFi connection manager
WebServerManager webServer;             // HTTP web server
MqttManager mqttMgr;                    // MQTT client manager
CaptivePortal captivePortal;            // WiFi provisioning portal

// Flags
bool needProvisioningMode = false;      // Set true to enter provisioning

// Timing
unsigned long lastSensorRead = 0;       // Last sensor read timestamp
unsigned long lastMqttPublish = 0;      // Last MQTT sensor publish

// Auto watering state (TASK 2.3)
bool autoModeEnabled = true;            // Auto watering mode
uint8_t thresholdDry = DEFAULT_THRESHOLD_DRY;   // Start watering below this
uint8_t thresholdWet = DEFAULT_THRESHOLD_WET;   // Stop watering above this

//=============================================================================
// WEB SERVER CALLBACKS
//=============================================================================
uint8_t getMoisture() { return sensors.getAverageMoisture(); }
bool getPumpState() { return pump.isRunning(); }
const char* getPumpReason() { return pump.getReasonString(); }
uint16_t getPumpRuntime() { return pump.getRuntime(); }
bool getAutoMode() { return autoModeEnabled; }

void setPump(bool on) {
    // Khi AUTO mode, không cho phép điều khiển thủ công
    if (autoModeEnabled) {
        LOG_WRN(MOD_PUMP, "manual", "Cannot control pump in AUTO mode!");
        return;
    }
    
    if (on) {
        pump.turnOn(PumpReason::MANUAL);
    } else {
        pump.turnOff();
    }
}

void setAutoMode(bool enabled) {
    autoModeEnabled = enabled;
    LOG_INF(MOD_SYSTEM, "mode", "Auto mode: %s", enabled ? "ON" : "OFF");
}

void setThresholds(uint8_t dry, uint8_t wet) {
    thresholdDry = dry;
    thresholdWet = wet;
    LOG_INF(MOD_SYSTEM, "config", "Thresholds: dry=%d%%, wet=%d%%", dry, wet);
}

//=============================================================================
// SPEED CONTROL CALLBACKS
//=============================================================================
uint8_t getPumpSpeed() {
    return pump.getSpeed();
}

void setPumpSpeed(uint8_t percent) {
    pump.setSpeed(percent);
}

//=============================================================================
// SCHEDULE CALLBACKS
//=============================================================================
bool getScheduleConfig(WebScheduleConfig* config, String* nextRun) {
    if (!config) return false;
    
    ScheduleConfig& schedConfig = scheduler.getConfig();
    config->enabled = schedConfig.enabled;
    
    for (int i = 0; i < 4; i++) {
        ScheduleEntry* entry = scheduler.getEntry(i);
        if (entry) {
            config->entries[i].hour = entry->hour;
            config->entries[i].minute = entry->minute;
            config->entries[i].duration = entry->duration;
            config->entries[i].enabled = entry->enabled;
        }
    }
    
    if (nextRun) {
        *nextRun = scheduler.getNextScheduleString();
    }
    
    return true;
}

void setScheduleEnabled(bool enabled) {
    scheduler.setEnabled(enabled);
    LOG_INF(MOD_SYSTEM, "schedule", "Schedule %s", enabled ? "enabled" : "disabled");
}

void setScheduleEntry(uint8_t index, uint8_t hour, uint8_t minute, 
                      uint16_t duration, bool enabled) {
    if (index < 4) {
        scheduler.setEntry(index, hour, minute, duration, enabled);
    }
}

void saveScheduleConfig() {
    if (scheduler.saveSchedule()) {
        LOG_INF(MOD_SYSTEM, "schedule", "Schedule saved to storage");
    }
}

//=============================================================================
// MQTT FUNCTIONS (TASK 4.2, 4.3)
//=============================================================================

/**
 * @brief Publish sensor data via MQTT
 * Topic: devices/{deviceId}/sensor/data
 */
void mqttPublishSensorData() {
    if (!mqttMgr.isConnected()) return;
    
    JsonDocument doc;
    doc["moisture1"] = sensors.getSensor1().getMoisturePercent();
    doc["moisture2"] = sensors.getSensor2().getMoisturePercent();
    doc["moistureAvg"] = sensors.getAverageMoisture();
    doc["moistureRaw"] = sensors.getSensor2().readAnalogRaw();
    doc["ts"] = millis() / 1000;  // Uptime in seconds (will use NTP later)
    
    char payload[128];
    serializeJson(doc, payload, sizeof(payload));
    
    mqttMgr.publish("sensor/data", payload, 0, false);  // QoS 0, no retain
}

/**
 * @brief Publish pump status via MQTT
 * Topic: devices/{deviceId}/pump/status
 */
void mqttPublishPumpStatus() {
    if (!mqttMgr.isConnected()) return;
    
    JsonDocument doc;
    doc["running"] = pump.isRunning();
    doc["runtime"] = pump.getRuntime();
    doc["reason"] = pump.getReasonString();
    doc["ts"] = millis() / 1000;
    
    char payload[128];
    serializeJson(doc, payload, sizeof(payload));
    
    mqttMgr.publish("pump/status", payload, 1, false);  // QoS 1, no retain
}

/**
 * @brief Publish mode status via MQTT
 * Topic: devices/{deviceId}/mode
 */
void mqttPublishMode() {
    if (!mqttMgr.isConnected()) return;
    
    JsonDocument doc;
    doc["mode"] = autoModeEnabled ? "auto" : "manual";
    doc["threshold_dry"] = thresholdDry;
    doc["threshold_wet"] = thresholdWet;
    doc["ts"] = millis() / 1000;
    
    char payload[128];
    serializeJson(doc, payload, sizeof(payload));
    
    mqttMgr.publish("mode", payload, 1, true);  // QoS 1, retain
}

/**
 * @brief MQTT message callback - handle incoming commands
 * 
 * Topics handled:
 * - devices/{deviceId}/pump/control   -> {"action": "on"|"off"|"toggle", "duration": 30}
 * - devices/{deviceId}/config         -> {"threshold_dry": 30, "threshold_wet": 50}
 * - devices/{deviceId}/mode/control   -> {"mode": "auto"|"manual"}
 */
void mqttMessageCallback(const char* topic, const uint8_t* payload, unsigned int length) {
    // Null-terminate payload for parsing
    char payloadStr[256];
    size_t copyLen = (length < sizeof(payloadStr) - 1) ? length : sizeof(payloadStr) - 1;
    memcpy(payloadStr, payload, copyLen);
    payloadStr[copyLen] = '\0';
    
    LOG_INF(MOD_MQTT, "recv", "%s: %s", topic, payloadStr);
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payloadStr);
    if (error) {
        LOG_WRN(MOD_MQTT, "recv", "JSON parse error: %s", error.c_str());
        return;
    }
    
    // Check topic suffix
    String topicStr(topic);
    
    // Handle pump/control
    if (topicStr.endsWith("pump/control")) {
        const char* action = doc["action"];
        if (action) {
            if (strcmp(action, "on") == 0) {
                int duration = doc["duration"] | PUMP_MAX_RUNTIME_SEC;
                pump.setMaxRuntime(duration);
                pump.turnOn(PumpReason::MANUAL);
                LOG_INF(MOD_MQTT, "cmd", "Pump ON (duration=%ds)", duration);
            } else if (strcmp(action, "off") == 0) {
                pump.turnOff();
                LOG_INF(MOD_MQTT, "cmd", "Pump OFF");
            } else if (strcmp(action, "toggle") == 0) {
                if (pump.isRunning()) {
                    pump.turnOff();
                } else {
                    pump.turnOn(PumpReason::MANUAL);
                }
                LOG_INF(MOD_MQTT, "cmd", "Pump TOGGLE -> %s", pump.isRunning() ? "ON" : "OFF");
            }
            mqttPublishPumpStatus();  // Respond with status
        }
        return;
    }
    
    // Handle mode/control
    if (topicStr.endsWith("mode/control")) {
        const char* mode = doc["mode"];
        if (mode) {
            if (strcmp(mode, "auto") == 0) {
                autoModeEnabled = true;
                LOG_INF(MOD_MQTT, "cmd", "Mode -> AUTO");
            } else if (strcmp(mode, "manual") == 0) {
                autoModeEnabled = false;
                LOG_INF(MOD_MQTT, "cmd", "Mode -> MANUAL");
            }
            mqttPublishMode();  // Respond with status
        }
        return;
    }
    
    // Handle config
    if (topicStr.endsWith("config")) {
        bool changed = false;
        
        if (doc["threshold_dry"].is<int>()) {
            thresholdDry = doc["threshold_dry"];
            changed = true;
        }
        if (doc["threshold_wet"].is<int>()) {
            thresholdWet = doc["threshold_wet"];
            changed = true;
        }
        if (doc["max_runtime"].is<int>()) {
            pump.setMaxRuntime(doc["max_runtime"]);
            changed = true;
        }
        
        if (changed) {
            LOG_INF(MOD_MQTT, "cmd", "Config updated: dry=%d%%, wet=%d%%", thresholdDry, thresholdWet);
            mqttPublishMode();  // Respond with updated config
        }
        return;
    }
}

/**
 * @brief Subscribe to MQTT command topics
 */
void mqttSubscribeTopics() {
    mqttMgr.subscribe("pump/control", 1);
    mqttMgr.subscribe("config", 1);
    mqttMgr.subscribe("mode/control", 1);
}

//=============================================================================
// WIFI PROVISIONING (TASK 5.2)
//=============================================================================

/**
 * @brief Enter WiFi provisioning mode (Captive Portal)
 * 
 * Called when:
 * - WiFi connection fails multiple times
 * - User presses reset button long
 * - MQTT command received
 */
void enterProvisioningMode() {
    LOG_INF(MOD_SYSTEM, "prov", "Entering provisioning mode...");
    
    // Stop normal operation
    webServer.stop();
    wifiMgr.disconnect();
    
    // Start captive portal
    captivePortal.setTimeout(300000);  // 5 minutes
    
    captivePortal.onCredentialsReceived([](const String& ssid, const String& password) {
        LOG_INF(MOD_SYSTEM, "prov", "Credentials received: %s", ssid.c_str());
        
        // Save to storage
        StorageManager storage;
        if (storage.begin()) {
            storage.saveWiFi(ssid.c_str(), password.c_str());
            LOG_INF(MOD_SYSTEM, "prov", "WiFi config saved");
        }
    });
    
    captivePortal.onMqttConfigReceived([](const String& server, uint16_t port, 
                                          const String& user, const String& pass) {
        LOG_INF(MOD_SYSTEM, "prov", "MQTT config: %s:%d", server.c_str(), port);
        
        StorageManager storage;
        if (storage.begin()) {
            storage.saveMqtt(server.c_str(), port, user.c_str(), pass.c_str());
            LOG_INF(MOD_SYSTEM, "prov", "MQTT config saved");
        }
    });
    
    captivePortal.onTimeout([]() {
        LOG_WRN(MOD_SYSTEM, "prov", "Provisioning timeout, restarting...");
        delay(1000);
        ESP.restart();
    });
    
    captivePortal.begin("TuoiCay-Setup");
    needProvisioningMode = false;
}

/**
 * @brief Exit provisioning mode and restart
 */
void exitProvisioningMode() {
    if (captivePortal.isActive()) {
        captivePortal.stop();
        LOG_INF(MOD_SYSTEM, "prov", "Exiting provisioning, restarting...");
        delay(1000);
        ESP.restart();
    }
}

//=============================================================================
// WATCHDOG IMPLEMENTATION (ESP8266 Software WDT)
//=============================================================================

/**
 * @brief Watchdog timer callback - called every second
 * 
 * LOGIC:
 * - If wdtFlag not cleared within WDT_TIMEOUT_SEC, reset ESP
 * - This catches infinite loops and blocking code
 */
static uint8_t wdtCounter = 0;

void IRAM_ATTR wdt_callback() {
    wdtCounter++;
    if (wdtCounter >= WDT_TIMEOUT_SEC) {
        LOG_ERR(MOD_SYSTEM, "wdt", "TIMEOUT! Resetting...");
        ESP.restart();
    }
}

/**
 * @brief Initialize software watchdog timer
 */
void watchdog_init() {
    wdtCounter = 0;
    wdtTicker.attach(1.0, wdt_callback);  // Call every 1 second
    LOG_INF(MOD_SYSTEM, "wdt", "Initialized, timeout=%ds", WDT_TIMEOUT_SEC);
}

/**
 * @brief Feed the watchdog - call this regularly in loop()
 */
void watchdog_feed() {
    wdtCounter = 0;
    ESP.wdtFeed();  // Also feed hardware WDT
}

//=============================================================================
// BOOT REASON DETECTION
//=============================================================================

/**
 * @brief Print the reason for last reset
 * 
 * ESP8266 Reset Reasons:
 * 0 = Power on
 * 1 = Hardware WDT reset
 * 2 = Exception reset
 * 3 = Software WDT reset
 * 4 = Software restart
 * 5 = Deep sleep wake
 * 6 = External reset
 */
void print_boot_reason() {
    rst_info* resetInfo = ESP.getResetInfoPtr();
    
    const char* reason;
    switch (resetInfo->reason) {
        case REASON_DEFAULT_RST:      reason = "Power on"; break;
        case REASON_WDT_RST:          reason = "Hardware WDT"; break;
        case REASON_EXCEPTION_RST:    reason = "Exception"; break;
        case REASON_SOFT_WDT_RST:     reason = "Software WDT"; break;
        case REASON_SOFT_RESTART:     reason = "Software restart"; break;
        case REASON_DEEP_SLEEP_AWAKE: reason = "Deep sleep wake"; break;
        case REASON_EXT_SYS_RST:      reason = "External reset"; break;
        default:                      reason = "Unknown"; break;
    }
    
    LOG_INF(MOD_SYSTEM, "boot", "Reset reason: %s (%d)", reason, resetInfo->reason);
    
    // Check for watchdog reset - may indicate problem
    if (resetInfo->reason == REASON_WDT_RST || 
        resetInfo->reason == REASON_SOFT_WDT_RST) {
        LOG_WRN(MOD_SYSTEM, "boot", "WDT reset detected! Check for blocking code");
    }
}

//=============================================================================
// SETUP
//=============================================================================

void setup() {
    //-------------------------------------------------------------------------
    // STEP 1: GPIO Safe State (CRITICAL - do this FIRST!)
    //-------------------------------------------------------------------------
    pins_init_safe();  // Pump OFF, LED OFF
    
    //-------------------------------------------------------------------------
    // STEP 2: Initialize Serial/Logger
    //-------------------------------------------------------------------------
    logger_init(SERIAL_BAUD_RATE);
    
    LOG_INF(MOD_SYSTEM, "boot", "================================");
    LOG_INF(MOD_SYSTEM, "boot", "%s FW v%s started", FW_NAME, FW_VERSION);
    LOG_INF(MOD_SYSTEM, "boot", "================================");
    
    //-------------------------------------------------------------------------
    // STEP 3: Boot Reason Detection
    //-------------------------------------------------------------------------
    print_boot_reason();
    
    //-------------------------------------------------------------------------
    // STEP 4: Initialize Watchdog
    //-------------------------------------------------------------------------
    watchdog_init();
    
    //-------------------------------------------------------------------------
    // STEP 5: Log System Info
    //-------------------------------------------------------------------------
    LOG_INF(MOD_SYSTEM, "init", "Chip ID: %08X", ESP.getChipId());
    LOG_INF(MOD_SYSTEM, "init", "Flash size: %u KB", ESP.getFlashChipSize() / 1024);
    LOG_INF(MOD_SYSTEM, "init", "Free heap: %u bytes", ESP.getFreeHeap());
    LOG_INF(MOD_SYSTEM, "init", "CPU freq: %u MHz", ESP.getCpuFreqMHz());
    
    //-------------------------------------------------------------------------
    // STEP 6: Confirm Safe State
    //-------------------------------------------------------------------------
    gpio_set_safe();  // Extra safety - ensure pump is OFF
    LOG_INF(MOD_SYSTEM, "init", "GPIO safe state confirmed (pump OFF)");
    
    //-------------------------------------------------------------------------
    // STEP 7: Initialize Sensors (TASK 2.1)
    //-------------------------------------------------------------------------
    if (!sensors.begin()) {
        LOG_ERR(MOD_SYSTEM, "init", "Sensor init failed!");
    }
    
    //-------------------------------------------------------------------------
    // STEP 8: Initialize Pump (TASK 2.2)
    //-------------------------------------------------------------------------
    if (!pump.begin()) {
        LOG_ERR(MOD_SYSTEM, "init", "Pump init failed!");
    }
    
    //-------------------------------------------------------------------------
    // STEP 9: Initialize WiFi (TASK 3.1)
    //-------------------------------------------------------------------------
    wifiMgr.setStatusLED(PIN_LED_STATUS);
    if (wifiMgr.begin(WIFI_SSID, WIFI_PASSWORD)) {
        wifiMgr.connect();
    } else {
        LOG_ERR(MOD_SYSTEM, "init", "WiFi init failed!");
    }
    
    //-------------------------------------------------------------------------
    // STEP 10: Initialize Web Server (TASK 3.2)
    // Note: Server will start after WiFi connects
    //-------------------------------------------------------------------------
    webServer.setDataProviders(getMoisture, getPumpState, getPumpReason, getPumpRuntime, getAutoMode);
    webServer.setControlCallbacks(setPump, setAutoMode, setThresholds);
    webServer.setThresholdPointers(&thresholdDry, &thresholdWet);
    webServer.setSpeedCallbacks(getPumpSpeed, setPumpSpeed);
    webServer.setScheduleCallbacks(getScheduleConfig, setScheduleEnabled, setScheduleEntry, saveScheduleConfig);
    
    //-------------------------------------------------------------------------
    // STEP 11: Initialize MQTT (TASK 4.1)
    // Note: MQTT will connect after WiFi connects
    //-------------------------------------------------------------------------
    String deviceId = wifiMgr.getDeviceId();  // MAC address without colons
    if (mqttMgr.begin(MQTT_BROKER, MQTT_PORT, deviceId.c_str())) {
        mqttMgr.setMessageCallback(mqttMessageCallback);
        mqttSubscribeTopics();
        LOG_INF(MOD_MQTT, "init", "MQTT ready, deviceId=%s", deviceId.c_str());
    } else {
        LOG_ERR(MOD_SYSTEM, "init", "MQTT init failed!");
    }
    
    //-------------------------------------------------------------------------
    // STEP 12: Initialize Storage (TASK 5.1)
    //-------------------------------------------------------------------------
    if (storage.begin()) {
        // Load saved config
        DeviceConfig savedConfig;
        if (storage.loadConfig(savedConfig)) {
            thresholdDry = savedConfig.thresholdDry;
            thresholdWet = savedConfig.thresholdWet;
            autoModeEnabled = savedConfig.autoMode;
            pump.setMaxRuntime(savedConfig.maxRuntime);
            LOG_INF(MOD_STORAGE, "load", "Config loaded from storage");
        }
        storage.listFiles();  // Debug: show stored files
    } else {
        LOG_ERR(MOD_SYSTEM, "init", "Storage init failed!");
    }
    
    //-------------------------------------------------------------------------
    // STEP 13: Initialize OTA (TASK 6.3)
    // Note: OTA will be active after WiFi connects
    //-------------------------------------------------------------------------
    String otaHostname = String(DEVICE_PREFIX) + "_" + deviceId;
    if (!otaManager.begin(otaHostname.c_str(), OTA_PASSWORD)) {
        LOG_ERR(MOD_SYSTEM, "init", "OTA init failed!");
    }
    
    //-------------------------------------------------------------------------
    // STEP 14: Initialize Time Manager (TASK 6.1)
    //-------------------------------------------------------------------------
    if (!timeManager.begin()) {
        LOG_ERR(MOD_SYSTEM, "init", "TimeManager init failed!");
    }
    
    //-------------------------------------------------------------------------
    // STEP 15: Initialize Scheduler (TASK 6.2)
    //-------------------------------------------------------------------------
    if (scheduler.begin()) {
        // Set moisture check callback
        scheduler.setMoistureCallback([]() -> bool {
            // Return true if soil NEEDS water (is dry)
            return sensors.getAverageMoisture() < thresholdDry;
        });
        
        // Set pump control callback
        scheduler.setPumpCallback([](bool on, uint16_t duration) {
            if (on) {
                pump.setMaxRuntime(duration);
                pump.turnOn(PumpReason::SCHEDULE);
            } else {
                pump.turnOff();
            }
        });
    } else {
        LOG_ERR(MOD_SYSTEM, "init", "Scheduler init failed!");
    }
    
    LOG_INF(MOD_SYSTEM, "init", "Setup complete! Entering main loop...");
    LOG_INF(MOD_SYSTEM, "init", "================================");
}

//=============================================================================
// AUTO WATERING LOGIC (TASK 2.3)
//=============================================================================

/**
 * @brief Auto watering logic with hysteresis
 * 
 * LOGIC:
 * - If moisture < thresholdDry (30%) -> Start pump
 * - If moisture > thresholdWet (50%) -> Stop pump
 * - Hysteresis prevents rapid on/off cycling
 */
void autoWatering() {
    if (!autoModeEnabled) return;
    
    uint8_t moisture = sensors.getAverageMoisture();
    
    if (!pump.isRunning()) {
        // Check if we should start watering
        if (moisture < thresholdDry) {
            // Soil is dry - start pump
            if (pump.turnOn(PumpReason::AUTO)) {
                LOG_INF(MOD_PUMP, "auto", "Soil dry (%d%% < %d%%), starting pump",
                        moisture, thresholdDry);
            } else {
                // Log why pump didn't start (likely cooldown)
                static unsigned long lastLogTime = 0;
                if (millis() - lastLogTime > 10000) {  // Log every 10s max
                    LOG_DBG(MOD_PUMP, "auto", "Pump not started (moisture=%d%%, state=%d, cooldown=%ds)",
                            moisture, (int)pump.getState(), pump.getCooldownRemaining());
                    lastLogTime = millis();
                }
            }
        }
    } else {
        // Pump is running - check if we should stop
        if (moisture > thresholdWet) {
            // Soil is wet enough - stop pump
            pump.turnOff();
            LOG_INF(MOD_PUMP, "auto", "Soil wet (%d%% > %d%%), stopping pump",
                    moisture, thresholdWet);
        }
    }
}

//=============================================================================
// LOOP
//=============================================================================

void loop() {
    //-------------------------------------------------------------------------
    // Feed watchdog FIRST (every loop iteration)
    //-------------------------------------------------------------------------
    watchdog_feed();
    
    //-------------------------------------------------------------------------
    // TASK 5.2: Handle Captive Portal if active
    //-------------------------------------------------------------------------
    if (captivePortal.isActive()) {
        captivePortal.update();
        
        // Check if config received - restart to apply
        if (captivePortal.hasConfig()) {
            LOG_INF(MOD_SYSTEM, "prov", "Config received, restarting in 2s...");
            delay(2000);
            ESP.restart();
        }
        
        yield();
        delay(10);
        return;  // Skip normal loop when in provisioning mode
    }
    
    // Check if we need to enter provisioning mode
    if (needProvisioningMode) {
        enterProvisioningMode();
        return;
    }
    
    unsigned long now = millis();
    
    //-------------------------------------------------------------------------
    // TASK 2.1: Read sensors periodically
    //-------------------------------------------------------------------------
    if (now - lastSensorRead >= SENSOR_READ_INTERVAL_MS) {
        lastSensorRead = now;
        sensors.update();
        sensors.logReadings();
    }
    
    //-------------------------------------------------------------------------
    // TASK 2.2: Update pump (check safety timeouts)
    //-------------------------------------------------------------------------
    pump.update();
    
    //-------------------------------------------------------------------------
    // TASK 2.3: Auto watering logic
    //-------------------------------------------------------------------------
    autoWatering();
    
    //-------------------------------------------------------------------------
    // TASK 3.1: Update WiFi (handle reconnect)
    //-------------------------------------------------------------------------
    wifiMgr.update();
    
    // Start web server and MQTT when WiFi connects (one-time)
    static bool webServerStarted = false;
    static bool mqttStarted = false;
    
    if (wifiMgr.isConnected()) {
        // Start web server
        if (!webServerStarted) {
            webServer.begin();
            webServerStarted = true;
        }
        
        // Connect MQTT
        if (!mqttStarted) {
            mqttMgr.connect();
            mqttStarted = true;
        }
    } else {
        // WiFi disconnected - reset flags
        webServerStarted = false;
        mqttStarted = false;
    }
    
    //-------------------------------------------------------------------------
    // TASK 3.2: Handle web requests
    //-------------------------------------------------------------------------
    if (wifiMgr.isConnected()) {
        webServer.update();
    }
    
    //-------------------------------------------------------------------------
    // TASK 4.1, 4.2, 4.3: Handle MQTT
    //-------------------------------------------------------------------------
    mqttMgr.update();
    
    // Publish sensor data periodically (every 5 seconds)
    if (mqttMgr.isConnected() && now - lastMqttPublish >= SENSOR_READ_INTERVAL_MS) {
        lastMqttPublish = now;
        mqttPublishSensorData();
    }
    
    //-------------------------------------------------------------------------
    // TASK 6.3: Handle OTA updates
    //-------------------------------------------------------------------------
    if (wifiMgr.isConnected()) {
        otaManager.update();
    }
    
    //-------------------------------------------------------------------------
    // TASK 6.1: Update Time Manager (NTP sync)
    //-------------------------------------------------------------------------
    timeManager.update();
    
    //-------------------------------------------------------------------------
    // TASK 6.2: Update Scheduler (check scheduled watering)
    //-------------------------------------------------------------------------
    if (autoModeEnabled) {
        scheduler.update();
    }
    
    //-------------------------------------------------------------------------
    // Small yield to prevent WDT (ESP8266 needs this)
    //-------------------------------------------------------------------------
    yield();
    
    //-------------------------------------------------------------------------
    // Non-blocking delay to reduce CPU usage
    //-------------------------------------------------------------------------
    delay(10);  // 10ms = 100 loops/second
}

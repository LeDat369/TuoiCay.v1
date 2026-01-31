/**
 * @file storage_manager.h
 * @brief Configuration Storage Manager using LittleFS
 * 
 * LOGIC:
 * - Store configuration in JSON file on LittleFS
 * - CRC verification for data integrity
 * - Factory reset to clear all config
 * - Separate files for WiFi, MQTT, and device config
 * 
 * RULES: #NVS(18) #FS(25)
 */

#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <config.h>

//=============================================================================
// FILE PATHS
//=============================================================================
#define CONFIG_FILE         "/config.json"
#define WIFI_FILE           "/wifi.json"
#define SCHEDULE_FILE       "/schedule.json"

//=============================================================================
// CONFIGURATION STRUCTURES
//=============================================================================

/**
 * @brief Device configuration
 */
struct DeviceConfig {
    // Thresholds
    uint8_t thresholdDry;       // Start watering below this (0-100%)
    uint8_t thresholdWet;       // Stop watering above this (0-100%)
    
    // Pump settings
    uint16_t maxRuntime;        // Max pump runtime in seconds
    uint32_t minOffTime;        // Min time between pump runs (ms)
    
    // Operating mode
    bool autoMode;              // true = auto, false = manual
    
    // CRC for verification
    uint16_t crc;
    
    // Initialize with defaults
    void setDefaults() {
        thresholdDry = DEFAULT_THRESHOLD_DRY;
        thresholdWet = DEFAULT_THRESHOLD_WET;
        maxRuntime = PUMP_MAX_RUNTIME_SEC;
        minOffTime = PUMP_MIN_OFF_TIME_MS;
        autoMode = true;
        crc = 0;
    }
};

/**
 * @brief WiFi configuration
 */
struct WiFiConfig {
    char ssid[33];              // Max SSID length 32 + null
    char password[65];          // Max password length 64 + null
    bool configured;            // true if WiFi has been configured
    uint16_t crc;
    
    void setDefaults() {
        ssid[0] = '\0';
        password[0] = '\0';
        configured = false;
        crc = 0;
    }
};

/**
 * @brief MQTT configuration
 */
struct MqttConfig {
    char broker[65];            // Broker hostname/IP
    uint16_t port;              // Broker port
    char username[33];          // MQTT username
    char password[65];          // MQTT password
    bool configured;
    uint16_t crc;
    
    void setDefaults() {
        broker[0] = '\0';
        port = 1883;
        username[0] = '\0';
        password[0] = '\0';
        configured = false;
        crc = 0;
    }
};

/**
 * @brief Schedule entry
 */
struct ScheduleEntry {
    uint8_t hour;               // Hour (0-23)
    uint8_t minute;             // Minute (0-59)
    uint16_t duration;          // Duration in seconds
    bool enabled;               // Is this entry enabled
    
    void setDefaults() {
        hour = 6;               // 6:00 AM
        minute = 0;
        duration = 30;          // 30 seconds
        enabled = false;
    }
};

/**
 * @brief Schedule configuration
 */
#define MAX_SCHEDULE_ENTRIES    4

struct ScheduleConfig {
    bool enabled;                               // Global schedule enable
    ScheduleEntry entries[MAX_SCHEDULE_ENTRIES];
    uint16_t crc;
    
    void setDefaults() {
        enabled = false;
        for (int i = 0; i < MAX_SCHEDULE_ENTRIES; i++) {
            entries[i].setDefaults();
        }
        // Set default schedules (disabled)
        entries[0].hour = 6;    // 6:00
        entries[1].hour = 18;   // 18:00
        crc = 0;
    }
};

//=============================================================================
// STORAGE MANAGER CLASS
//=============================================================================

/**
 * @class StorageManager
 * @brief Manages persistent configuration storage
 */
class StorageManager {
public:
    /**
     * @brief Initialize storage (mount LittleFS)
     * @return true if successful
     */
    bool begin();
    
    /**
     * @brief Check if storage is ready
     */
    bool isReady() const { return _initialized; }
    
    //-------------------------------------------------------------------------
    // Device Config
    //-------------------------------------------------------------------------
    
    /**
     * @brief Save device configuration
     * @return true if successful
     */
    bool saveConfig(const DeviceConfig& config);
    
    /**
     * @brief Load device configuration
     * @param config Output configuration
     * @return true if loaded successfully (with valid CRC)
     */
    bool loadConfig(DeviceConfig& config);
    
    //-------------------------------------------------------------------------
    // WiFi Config
    //-------------------------------------------------------------------------
    
    /**
     * @brief Save WiFi credentials
     * @param ssid WiFi SSID
     * @param password WiFi password
     * @return true if successful
     */
    bool saveWiFi(const char* ssid, const char* password);
    
    /**
     * @brief Load WiFi credentials
     * @param config Output WiFi configuration
     * @return true if loaded successfully and configured
     */
    bool loadWiFi(WiFiConfig& config);
    
    /**
     * @brief Check if WiFi is configured
     */
    bool isWiFiConfigured();
    
    //-------------------------------------------------------------------------
    // MQTT Config
    //-------------------------------------------------------------------------
    
    /**
     * @brief Save MQTT configuration
     * @return true if successful
     */
    bool saveMqtt(const char* broker, uint16_t port, const char* username = "", const char* password = "");
    
    /**
     * @brief Load MQTT configuration
     * @param config Output MQTT configuration
     * @return true if loaded successfully and configured
     */
    bool loadMqtt(MqttConfig& config);
    
    //-------------------------------------------------------------------------
    // Schedule Config
    //-------------------------------------------------------------------------
    
    /**
     * @brief Save schedule configuration
     * @return true if successful
     */
    bool saveSchedule(const ScheduleConfig& config);
    
    /**
     * @brief Load schedule configuration
     * @param config Output schedule configuration
     * @return true if loaded successfully
     */
    bool loadSchedule(ScheduleConfig& config);
    
    //-------------------------------------------------------------------------
    // Utilities
    //-------------------------------------------------------------------------
    
    /**
     * @brief Factory reset - clear all configuration
     */
    void factoryReset();
    
    /**
     * @brief Format filesystem (delete all files)
     */
    bool format();
    
    /**
     * @brief Get free space in bytes
     */
    size_t getFreeSpace();
    
    /**
     * @brief Get used space in bytes
     */
    size_t getUsedSpace();
    
    /**
     * @brief List all files (for debugging)
     */
    void listFiles();

private:
    bool _initialized;
    
    /**
     * @brief Calculate CRC16 of data
     */
    uint16_t _calcCRC(const uint8_t* data, size_t length);
    
    /**
     * @brief Read JSON file
     * @param filename File path
     * @param doc Output JSON document
     * @return true if read successfully
     */
    bool _readJsonFile(const char* filename, JsonDocument& doc);
    
    /**
     * @brief Write JSON file
     * @param filename File path
     * @param doc JSON document to write
     * @return true if written successfully
     */
    bool _writeJsonFile(const char* filename, const JsonDocument& doc);
};

// Global instance
extern StorageManager storage;

#endif // STORAGE_MANAGER_H

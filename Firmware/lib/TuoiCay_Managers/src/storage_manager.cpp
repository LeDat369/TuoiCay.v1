/**
 * @file storage_manager.cpp
 * @brief Implementation of Storage Manager using LittleFS
 * 
 * LOGIC:
 * - Each config type stored in separate JSON file
 * - CRC16 verification on load
 * - Factory reset clears all files
 * 
 * RULES: #NVS(18) #FS(25)
 */

#include "storage_manager.h"
#include <logger.h>
#include <error_codes.h>
#include <crc_utils.h>

// Global instance
StorageManager storage;

//=============================================================================
// STORAGE MANAGER IMPLEMENTATION
//=============================================================================

bool StorageManager::begin() {
    if (_initialized) return true;
    
    LOG_INF(MOD_STORAGE, "init", "Mounting LittleFS...");
    
    if (!LittleFS.begin()) {
        LOG_WRN(MOD_STORAGE, "init", "Mount failed, formatting...");
        
        if (!LittleFS.format()) {
            LOG_ERR(MOD_STORAGE, "init", "Format failed!");
            return false;
        }
        
        if (!LittleFS.begin()) {
            LOG_ERR(MOD_STORAGE, "init", "Mount failed after format!");
            return false;
        }
    }
    
    _initialized = true;
    
    FSInfo fs_info;
    LittleFS.info(fs_info);
    LOG_INF(MOD_STORAGE, "init", "LittleFS mounted, total=%uKB, used=%uKB",
            fs_info.totalBytes / 1024, fs_info.usedBytes / 1024);
    
    return true;
}

//=============================================================================
// DEVICE CONFIG
//=============================================================================

bool StorageManager::saveConfig(const DeviceConfig& config) {
    if (!_initialized) return false;
    
    JsonDocument doc;
    doc["thresholdDry"] = config.thresholdDry;
    doc["thresholdWet"] = config.thresholdWet;
    doc["maxRuntime"] = config.maxRuntime;
    doc["minOffTime"] = config.minOffTime;
    doc["autoMode"] = config.autoMode;
    
    // Calculate CRC (excluding CRC field itself)
    uint16_t crc = _calcCRC((const uint8_t*)&config, sizeof(DeviceConfig) - sizeof(uint16_t));
    doc["crc"] = crc;
    
    if (_writeJsonFile(CONFIG_FILE, doc)) {
        LOG_INF(MOD_STORAGE, "save", "Config saved (dry=%d%%, wet=%d%%, auto=%d)",
                config.thresholdDry, config.thresholdWet, config.autoMode);
        return true;
    }
    
    return false;
}

bool StorageManager::loadConfig(DeviceConfig& config) {
    if (!_initialized) {
        config.setDefaults();
        return false;
    }
    
    JsonDocument doc;
    if (!_readJsonFile(CONFIG_FILE, doc)) {
        LOG_WRN(MOD_STORAGE, "load", "Config file not found, using defaults");
        config.setDefaults();
        return false;
    }
    
    config.thresholdDry = doc["thresholdDry"] | DEFAULT_THRESHOLD_DRY;
    config.thresholdWet = doc["thresholdWet"] | DEFAULT_THRESHOLD_WET;
    config.maxRuntime = doc["maxRuntime"] | PUMP_MAX_RUNTIME_SEC;
    config.minOffTime = doc["minOffTime"] | PUMP_MIN_OFF_TIME_MS;
    config.autoMode = doc["autoMode"] | true;
    config.crc = doc["crc"] | 0;
    
    // Verify CRC
    uint16_t calcCrc = _calcCRC((const uint8_t*)&config, sizeof(DeviceConfig) - sizeof(uint16_t));
    if (config.crc != calcCrc) {
        LOG_WRN(MOD_STORAGE, "load", "Config CRC mismatch (stored=0x%04X, calc=0x%04X), using defaults",
                config.crc, calcCrc);
        config.setDefaults();
        return false;
    }
    
    LOG_INF(MOD_STORAGE, "load", "Config loaded (dry=%d%%, wet=%d%%, auto=%d)",
            config.thresholdDry, config.thresholdWet, config.autoMode);
    return true;
}

//=============================================================================
// WIFI CONFIG
//=============================================================================

bool StorageManager::saveWiFi(const char* ssid, const char* password) {
    if (!_initialized) return false;
    
    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["password"] = password;
    doc["configured"] = true;
    
    // Calculate simple CRC
    WiFiConfig cfg;
    strncpy(cfg.ssid, ssid, sizeof(cfg.ssid) - 1);
    strncpy(cfg.password, password, sizeof(cfg.password) - 1);
    cfg.configured = true;
    uint16_t crc = _calcCRC((const uint8_t*)&cfg, sizeof(WiFiConfig) - sizeof(uint16_t));
    doc["crc"] = crc;
    
    if (_writeJsonFile(WIFI_FILE, doc)) {
        LOG_INF(MOD_STORAGE, "save", "WiFi credentials saved (SSID=%s)", ssid);
        return true;
    }
    
    return false;
}

bool StorageManager::loadWiFi(WiFiConfig& config) {
    if (!_initialized) {
        config.setDefaults();
        return false;
    }
    
    JsonDocument doc;
    if (!_readJsonFile(WIFI_FILE, doc)) {
        LOG_WRN(MOD_STORAGE, "load", "WiFi file not found");
        config.setDefaults();
        return false;
    }
    
    const char* ssid = doc["ssid"] | "";
    const char* password = doc["password"] | "";
    
    strncpy(config.ssid, ssid, sizeof(config.ssid) - 1);
    config.ssid[sizeof(config.ssid) - 1] = '\0';
    
    strncpy(config.password, password, sizeof(config.password) - 1);
    config.password[sizeof(config.password) - 1] = '\0';
    
    config.configured = doc["configured"] | false;
    config.crc = doc["crc"] | 0;
    
    if (!config.configured || strlen(config.ssid) == 0) {
        LOG_WRN(MOD_STORAGE, "load", "WiFi not configured");
        return false;
    }
    
    LOG_INF(MOD_STORAGE, "load", "WiFi loaded (SSID=%s)", config.ssid);
    return true;
}

bool StorageManager::isWiFiConfigured() {
    WiFiConfig config;
    return loadWiFi(config);
}

//=============================================================================
// MQTT CONFIG
//=============================================================================

bool StorageManager::saveMqtt(const char* broker, uint16_t port, const char* username, const char* password) {
    if (!_initialized) return false;
    
    JsonDocument doc;
    doc["broker"] = broker;
    doc["port"] = port;
    doc["username"] = username;
    doc["password"] = password;
    doc["configured"] = true;
    
    if (_writeJsonFile("/mqtt.json", doc)) {
        LOG_INF(MOD_STORAGE, "save", "MQTT config saved (broker=%s:%d)", broker, port);
        return true;
    }
    
    return false;
}

bool StorageManager::loadMqtt(MqttConfig& config) {
    if (!_initialized) {
        config.setDefaults();
        return false;
    }
    
    JsonDocument doc;
    if (!_readJsonFile("/mqtt.json", doc)) {
        config.setDefaults();
        return false;
    }
    
    const char* broker = doc["broker"] | "";
    strncpy(config.broker, broker, sizeof(config.broker) - 1);
    config.broker[sizeof(config.broker) - 1] = '\0';
    
    config.port = doc["port"] | 1883;
    
    const char* username = doc["username"] | "";
    strncpy(config.username, username, sizeof(config.username) - 1);
    config.username[sizeof(config.username) - 1] = '\0';
    
    const char* password = doc["password"] | "";
    strncpy(config.password, password, sizeof(config.password) - 1);
    config.password[sizeof(config.password) - 1] = '\0';
    
    config.configured = doc["configured"] | false;
    
    if (!config.configured || strlen(config.broker) == 0) {
        return false;
    }
    
    LOG_INF(MOD_STORAGE, "load", "MQTT loaded (broker=%s:%d)", config.broker, config.port);
    return true;
}

//=============================================================================
// SCHEDULE CONFIG
//=============================================================================

bool StorageManager::saveSchedule(const ScheduleConfig& config) {
    if (!_initialized) return false;
    
    JsonDocument doc;
    doc["enabled"] = config.enabled;
    
    JsonArray entries = doc["entries"].to<JsonArray>();
    for (int i = 0; i < MAX_SCHEDULE_ENTRIES; i++) {
        JsonObject entry = entries.add<JsonObject>();
        entry["hour"] = config.entries[i].hour;
        entry["minute"] = config.entries[i].minute;
        entry["duration"] = config.entries[i].duration;
        entry["enabled"] = config.entries[i].enabled;
    }
    
    if (_writeJsonFile(SCHEDULE_FILE, doc)) {
        LOG_INF(MOD_STORAGE, "save", "Schedule saved (enabled=%d)", config.enabled);
        return true;
    }
    
    return false;
}

bool StorageManager::loadSchedule(ScheduleConfig& config) {
    if (!_initialized) {
        config.setDefaults();
        return false;
    }
    
    JsonDocument doc;
    if (!_readJsonFile(SCHEDULE_FILE, doc)) {
        config.setDefaults();
        return false;
    }
    
    config.enabled = doc["enabled"] | false;
    
    JsonArray entries = doc["entries"];
    for (int i = 0; i < MAX_SCHEDULE_ENTRIES && i < entries.size(); i++) {
        JsonObject entry = entries[i];
        config.entries[i].hour = entry["hour"] | 0;
        config.entries[i].minute = entry["minute"] | 0;
        config.entries[i].duration = entry["duration"] | 30;
        config.entries[i].enabled = entry["enabled"] | false;
    }
    
    LOG_INF(MOD_STORAGE, "load", "Schedule loaded (enabled=%d)", config.enabled);
    return true;
}

//=============================================================================
// UTILITIES
//=============================================================================

void StorageManager::factoryReset() {
    LOG_WRN(MOD_STORAGE, "reset", "Factory reset - clearing all config!");
    
    LittleFS.remove(CONFIG_FILE);
    LittleFS.remove(WIFI_FILE);
    LittleFS.remove("/mqtt.json");
    LittleFS.remove(SCHEDULE_FILE);
    
    LOG_INF(MOD_STORAGE, "reset", "Factory reset complete");
}

bool StorageManager::format() {
    LOG_WRN(MOD_STORAGE, "format", "Formatting filesystem!");
    
    if (LittleFS.format()) {
        LOG_INF(MOD_STORAGE, "format", "Format complete");
        return true;
    }
    
    LOG_ERR(MOD_STORAGE, "format", "Format failed!");
    return false;
}

size_t StorageManager::getFreeSpace() {
    FSInfo fs_info;
    LittleFS.info(fs_info);
    return fs_info.totalBytes - fs_info.usedBytes;
}

size_t StorageManager::getUsedSpace() {
    FSInfo fs_info;
    LittleFS.info(fs_info);
    return fs_info.usedBytes;
}

void StorageManager::listFiles() {
    LOG_INF(MOD_STORAGE, "list", "--- Files ---");
    
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        LOG_INF(MOD_STORAGE, "list", "  %s (%u bytes)", 
                dir.fileName().c_str(), dir.fileSize());
    }
    
    LOG_INF(MOD_STORAGE, "list", "-------------");
}

//=============================================================================
// PRIVATE METHODS
//=============================================================================

uint16_t StorageManager::_calcCRC(const uint8_t* data, size_t length) {
    return crc16(data, length);
}

bool StorageManager::_readJsonFile(const char* filename, JsonDocument& doc) {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        LOG_DBG(MOD_STORAGE, "read", "Failed to open %s", filename);
        return false;
    }
    
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        LOG_WRN(MOD_STORAGE, "read", "JSON parse error in %s: %s", filename, error.c_str());
        return false;
    }
    
    LOG_DBG(MOD_STORAGE, "read", "Read %s", filename);
    return true;
}

bool StorageManager::_writeJsonFile(const char* filename, const JsonDocument& doc) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        LOG_ERR(MOD_STORAGE, "write", "Failed to open %s for writing", filename);
        return false;
    }
    
    size_t written = serializeJson(doc, file);
    file.close();
    
    if (written == 0) {
        LOG_ERR(MOD_STORAGE, "write", "Failed to write %s", filename);
        return false;
    }
    
    LOG_DBG(MOD_STORAGE, "write", "Wrote %u bytes to %s", written, filename);
    return true;
}

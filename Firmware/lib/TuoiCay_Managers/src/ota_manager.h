#define OTA_PASSWORD        "your_ota_password"/**
 * @file ota_manager.h
 * @brief Over-The-Air Update Manager using ArduinoOTA
 * 
 * LOGIC:
 * - ArduinoOTA for wireless firmware updates
 * - Password protection
 * - Progress reporting
 * 
 * RULES: #OTA
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <config.h>

//=============================================================================
// OTA MANAGER CLASS
//=============================================================================

/**
 * @class OtaManager
 * @brief Manages OTA firmware updates
 */
class OtaManager {
public:
    /**
     * @brief Initialize OTA with device hostname and password
     * @param hostname Device hostname for mDNS
     * @param password OTA password (can be nullptr for no password)
     * @return true if successful
     */
    bool begin(const char* hostname, const char* password = nullptr);
    
    /**
     * @brief Handle OTA events (call in loop)
     */
    void update();
    
    /**
     * @brief Check if OTA is initialized
     */
    bool isReady() const { return _initialized; }
    
    /**
     * @brief Check if OTA is in progress
     */
    bool isUpdating() const { return _updating; }
    
    /**
     * @brief Get update progress (0-100%)
     */
    uint8_t getProgress() const { return _progress; }

private:
    bool _initialized;
    bool _updating;
    uint8_t _progress;
    
    /**
     * @brief Setup OTA callbacks
     */
    void _setupCallbacks();
};

// Global instance
extern OtaManager otaManager;

#endif // OTA_MANAGER_H

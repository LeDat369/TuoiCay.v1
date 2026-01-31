/**
 * @file time_manager.h
 * @brief NTP Time Synchronization Manager
 * 
 * LOGIC:
 * - Sync time from NTP server on boot and periodically
 * - Timezone support (Vietnam UTC+7)
 * - Formatted time strings for logging and display
 * 
 * RULES: #TIME(12)
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <time.h>
#include <config.h>

//=============================================================================
// NTP CONFIGURATION
//=============================================================================
#define NTP_SERVER_1        "pool.ntp.org"
#define NTP_SERVER_2        "time.nist.gov"
#define NTP_SERVER_3        "time.google.com"

// Vietnam timezone: UTC+7, no DST
#define TZ_OFFSET_SEC       (NTP_TIMEZONE_OFFSET * 3600)
#define DST_OFFSET_SEC      0

//=============================================================================
// TIME MANAGER CLASS
//=============================================================================

/**
 * @class TimeManager
 * @brief Manages NTP time synchronization
 */
class TimeManager {
public:
    /**
     * @brief Initialize time manager and start NTP sync
     * @return true if initialization successful
     */
    bool begin();
    
    /**
     * @brief Update time manager (call in loop for periodic sync)
     */
    void update();
    
    /**
     * @brief Force NTP synchronization
     * @return true if sync started
     */
    bool syncNow();
    
    /**
     * @brief Check if time has been synchronized
     */
    bool isSynced() const { return _synced; }
    
    /**
     * @brief Get current epoch time (seconds since 1970)
     */
    time_t getEpoch() const;
    
    /**
     * @brief Get current hour (0-23)
     */
    uint8_t getHour() const;
    
    /**
     * @brief Get current minute (0-59)
     */
    uint8_t getMinute() const;
    
    /**
     * @brief Get current second (0-59)
     */
    uint8_t getSecond() const;
    
    /**
     * @brief Get day of week (0=Sunday, 6=Saturday)
     */
    uint8_t getDayOfWeek() const;
    
    /**
     * @brief Get day of month (1-31)
     */
    uint8_t getDay() const;
    
    /**
     * @brief Get month (1-12)
     */
    uint8_t getMonth() const;
    
    /**
     * @brief Get year (e.g., 2024)
     */
    uint16_t getYear() const;
    
    /**
     * @brief Get formatted time string "HH:MM:SS"
     */
    String getTimeString() const;
    
    /**
     * @brief Get formatted date string "YYYY-MM-DD"
     */
    String getDateString() const;
    
    /**
     * @brief Get formatted datetime string "YYYY-MM-DD HH:MM:SS"
     */
    String getDateTimeString() const;
    
    /**
     * @brief Get time since last sync (seconds)
     */
    unsigned long getSecondsSinceSync() const;

private:
    bool _initialized;
    bool _synced;
    unsigned long _lastSyncTime;    // millis() when last synced
    time_t _lastSyncEpoch;          // epoch when last synced
    
    /**
     * @brief Get current time struct
     */
    struct tm* _getTimeInfo() const;
};

// Global instance
extern TimeManager timeManager;

#endif // TIME_MANAGER_H

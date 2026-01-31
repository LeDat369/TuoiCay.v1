/**
 * @file time_manager.cpp
 * @brief Implementation of NTP Time Manager
 * 
 * LOGIC:
 * - Uses ESP8266 built-in SNTP
 * - Configures timezone and DST
 * - Periodic sync every 6 hours
 * 
 * RULES: #TIME(12)
 */

#include "time_manager.h"
#include <logger.h>
#include <ESP8266WiFi.h>
#include <coredecls.h>  // settimeofday_cb()

// Global instance
TimeManager timeManager;

// Callback flag for time sync
static volatile bool _timeSyncCallback = false;

// Callback when time is set
void _onTimeSync() {
    _timeSyncCallback = true;
}

//=============================================================================
// TIME MANAGER IMPLEMENTATION
//=============================================================================

bool TimeManager::begin() {
    if (_initialized) return true;
    
    LOG_INF(MOD_TIME, "init", "Initializing NTP (TZ=UTC+%d)...", NTP_TIMEZONE_OFFSET);
    
    // Set timezone
    configTime(TZ_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
    
    // Register callback for time sync
    settimeofday_cb(_onTimeSync);
    
    _initialized = true;
    _synced = false;
    _lastSyncTime = 0;
    _lastSyncEpoch = 0;
    
    LOG_INF(MOD_TIME, "init", "NTP initialized, waiting for sync...");
    return true;
}

void TimeManager::update() {
    if (!_initialized) return;
    
    // Check if time was synced via callback
    if (_timeSyncCallback) {
        _timeSyncCallback = false;
        
        time_t now = time(nullptr);
        if (now > 1609459200) {  // After 2021-01-01 (valid time)
            _synced = true;
            _lastSyncTime = millis();
            _lastSyncEpoch = now;
            
            LOG_INF(MOD_TIME, "sync", "Time synced: %s", getDateTimeString().c_str());
        }
    }
    
    // Periodic resync
    if (_synced && millis() - _lastSyncTime >= NTP_SYNC_INTERVAL_MS) {
        LOG_INF(MOD_TIME, "sync", "Periodic NTP resync...");
        syncNow();
    }
}

bool TimeManager::syncNow() {
    if (!_initialized) return false;
    
    // Trigger resync by reconfiguring
    configTime(TZ_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
    
    LOG_INF(MOD_TIME, "sync", "NTP sync requested");
    return true;
}

time_t TimeManager::getEpoch() const {
    return time(nullptr);
}

struct tm* TimeManager::_getTimeInfo() const {
    time_t now = time(nullptr);
    return localtime(&now);
}

uint8_t TimeManager::getHour() const {
    struct tm* ti = _getTimeInfo();
    return ti ? ti->tm_hour : 0;
}

uint8_t TimeManager::getMinute() const {
    struct tm* ti = _getTimeInfo();
    return ti ? ti->tm_min : 0;
}

uint8_t TimeManager::getSecond() const {
    struct tm* ti = _getTimeInfo();
    return ti ? ti->tm_sec : 0;
}

uint8_t TimeManager::getDayOfWeek() const {
    struct tm* ti = _getTimeInfo();
    return ti ? ti->tm_wday : 0;
}

uint8_t TimeManager::getDay() const {
    struct tm* ti = _getTimeInfo();
    return ti ? ti->tm_mday : 1;
}

uint8_t TimeManager::getMonth() const {
    struct tm* ti = _getTimeInfo();
    return ti ? (ti->tm_mon + 1) : 1;  // tm_mon is 0-11
}

uint16_t TimeManager::getYear() const {
    struct tm* ti = _getTimeInfo();
    return ti ? (ti->tm_year + 1900) : 2000;  // tm_year is years since 1900
}

String TimeManager::getTimeString() const {
    char buf[12];
    struct tm* ti = _getTimeInfo();
    if (ti) {
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", 
                 ti->tm_hour, ti->tm_min, ti->tm_sec);
    } else {
        strcpy(buf, "00:00:00");
    }
    return String(buf);
}

String TimeManager::getDateString() const {
    char buf[12];
    struct tm* ti = _getTimeInfo();
    if (ti) {
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                 ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday);
    } else {
        strcpy(buf, "2000-01-01");
    }
    return String(buf);
}

String TimeManager::getDateTimeString() const {
    char buf[24];
    struct tm* ti = _getTimeInfo();
    if (ti) {
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                 ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
                 ti->tm_hour, ti->tm_min, ti->tm_sec);
    } else {
        strcpy(buf, "2000-01-01 00:00:00");
    }
    return String(buf);
}

unsigned long TimeManager::getSecondsSinceSync() const {
    if (!_synced) return 0;
    return (millis() - _lastSyncTime) / 1000;
}

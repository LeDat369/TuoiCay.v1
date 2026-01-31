/**
 * @file scheduler.cpp
 * @brief Implementation of Watering Scheduler
 * 
 * LOGIC:
 * - Check time every loop iteration
 * - Trigger only once per minute (prevent multiple triggers)
 * - Check moisture before watering (skip if wet)
 * - Auto-stop after duration
 * 
 * RULES: #TIME(12) #ACTUATOR(15)
 */

#include "scheduler.h"
#include <logger.h>

// Global instance
Scheduler scheduler;

//=============================================================================
// SCHEDULER IMPLEMENTATION
//=============================================================================

bool Scheduler::begin() {
    if (_initialized) return true;
    
    LOG_INF(MOD_SCHED, "init", "Initializing scheduler...");
    
    _config.setDefaults();
    _moistureCb = nullptr;
    _pumpCb = nullptr;
    _isWatering = false;
    _lastCheckedMinute = 255;  // Invalid value to trigger first check
    _currentEntryIndex = 0;
    _wateringStartTime = 0;
    _wateringDuration = 0;
    
    // Load saved schedule
    loadSchedule();
    
    _initialized = true;
    
    LOG_INF(MOD_SCHED, "init", "Scheduler ready (enabled=%d, entries=%d)",
            _config.enabled, getEnabledCount());
    
    return true;
}

void Scheduler::update() {
    if (!_initialized) return;
    
    // Check if currently watering (scheduled)
    if (_isWatering) {
        // Check if duration elapsed
        if (millis() - _wateringStartTime >= _wateringDuration * 1000UL) {
            _stopWatering();
        }
        return;
    }
    
    // Don't check if scheduler is disabled
    if (!_config.enabled) return;
    
    // Don't check if time not synced
    if (!timeManager.isSynced()) return;
    
    // Get current time
    uint8_t currentHour = timeManager.getHour();
    uint8_t currentMinute = timeManager.getMinute();
    
    // Only check once per minute
    if (currentMinute == _lastCheckedMinute) return;
    _lastCheckedMinute = currentMinute;
    
    // Check each schedule entry
    for (uint8_t i = 0; i < MAX_SCHEDULE_ENTRIES; i++) {
        if (!_config.entries[i].enabled) continue;
        
        if (_matchesTime(_config.entries[i])) {
            LOG_INF(MOD_SCHED, "trigger", "Schedule #%d triggered at %02d:%02d",
                    i, currentHour, currentMinute);
            
            // Check if soil needs water
            if (_moistureCb && !_moistureCb()) {
                LOG_INF(MOD_SCHED, "skip", "Skipping - soil is wet enough");
                continue;
            }
            
            // Start watering
            _startWatering(i);
            break;  // Only one schedule at a time
        }
    }
}

bool Scheduler::loadSchedule() {
    if (storage.loadSchedule(_config)) {
        LOG_INF(MOD_SCHED, "load", "Schedule loaded from storage");
        return true;
    }
    
    LOG_WRN(MOD_SCHED, "load", "No saved schedule, using defaults");
    _config.setDefaults();
    return false;
}

bool Scheduler::saveSchedule() {
    if (storage.saveSchedule(_config)) {
        LOG_INF(MOD_SCHED, "save", "Schedule saved to storage");
        return true;
    }
    
    LOG_ERR(MOD_SCHED, "save", "Failed to save schedule!");
    return false;
}

void Scheduler::setEnabled(bool enabled) {
    _config.enabled = enabled;
    LOG_INF(MOD_SCHED, "cfg", "Scheduler %s", enabled ? "ENABLED" : "DISABLED");
}

ScheduleEntry* Scheduler::getEntry(uint8_t index) {
    if (index >= MAX_SCHEDULE_ENTRIES) return nullptr;
    return &_config.entries[index];
}

void Scheduler::setEntry(uint8_t index, uint8_t hour, uint8_t minute,
                         uint16_t duration, bool enabled) {
    if (index >= MAX_SCHEDULE_ENTRIES) return;
    
    _config.entries[index].hour = hour;
    _config.entries[index].minute = minute;
    _config.entries[index].duration = duration;
    _config.entries[index].enabled = enabled;
    
    LOG_INF(MOD_SCHED, "cfg", "Entry #%d: %02d:%02d, %ds, %s",
            index, hour, minute, duration, enabled ? "ON" : "OFF");
}

uint8_t Scheduler::getEnabledCount() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_SCHEDULE_ENTRIES; i++) {
        if (_config.entries[i].enabled) count++;
    }
    return count;
}

String Scheduler::getNextScheduleString() const {
    if (!_config.enabled) return "Disabled";
    
    uint8_t currentHour = timeManager.getHour();
    uint8_t currentMinute = timeManager.getMinute();
    
    // Find next schedule
    int bestIndex = -1;
    int bestDiff = 24 * 60 + 1;  // Max minutes in a day + 1
    
    for (uint8_t i = 0; i < MAX_SCHEDULE_ENTRIES; i++) {
        if (!_config.entries[i].enabled) continue;
        
        int schedMinutes = _config.entries[i].hour * 60 + _config.entries[i].minute;
        int currentMinutes = currentHour * 60 + currentMinute;
        
        int diff = schedMinutes - currentMinutes;
        if (diff <= 0) diff += 24 * 60;  // Wrap to next day
        
        if (diff < bestDiff) {
            bestDiff = diff;
            bestIndex = i;
        }
    }
    
    if (bestIndex < 0) return "None";
    
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d",
             _config.entries[bestIndex].hour,
             _config.entries[bestIndex].minute);
    return String(buf);
}

bool Scheduler::_matchesTime(const ScheduleEntry& entry) const {
    return (entry.hour == timeManager.getHour() &&
            entry.minute == timeManager.getMinute());
}

void Scheduler::_startWatering(uint8_t entryIndex) {
    if (entryIndex >= MAX_SCHEDULE_ENTRIES) return;
    
    _currentEntryIndex = entryIndex;
    _wateringDuration = _config.entries[entryIndex].duration;
    _wateringStartTime = millis();
    _isWatering = true;
    
    LOG_INF(MOD_SCHED, "start", "Starting scheduled watering for %ds",
            _wateringDuration);
    
    // Turn on pump via callback
    if (_pumpCb) {
        _pumpCb(true, _wateringDuration);
    }
}

void Scheduler::_stopWatering() {
    LOG_INF(MOD_SCHED, "stop", "Scheduled watering complete");
    
    _isWatering = false;
    
    // Turn off pump via callback
    if (_pumpCb) {
        _pumpCb(false, 0);
    }
}

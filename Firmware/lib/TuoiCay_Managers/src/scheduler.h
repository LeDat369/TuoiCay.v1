/**
 * @file scheduler.h
 * @brief Watering Schedule Manager
 * 
 * LOGIC:
 * - Schedule watering at specific times
 * - Skip if soil is already wet enough
 * - Multiple schedule entries support
 * - Integrates with TimeManager for time checks
 * 
 * RULES: #TIME(12) #ACTUATOR(15)
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include <storage_manager.h>
#include <time_manager.h>

//=============================================================================
// SCHEDULER CALLBACKS
//=============================================================================
typedef bool (*SchedulerMoistureCallback)();    // Returns true if soil needs water
typedef void (*SchedulerPumpCallback)(bool on, uint16_t duration);  // Control pump

//=============================================================================
// SCHEDULER CLASS
//=============================================================================

/**
 * @class Scheduler
 * @brief Manages scheduled watering events
 */
class Scheduler {
public:
    /**
     * @brief Initialize scheduler
     * @return true if successful
     */
    bool begin();
    
    /**
     * @brief Update scheduler (call in loop)
     * Checks if it's time to water
     */
    void update();
    
    /**
     * @brief Load schedule from storage
     */
    bool loadSchedule();
    
    /**
     * @brief Save schedule to storage
     */
    bool saveSchedule();
    
    /**
     * @brief Enable/disable scheduler
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Check if scheduler is enabled
     */
    bool isEnabled() const { return _config.enabled; }
    
    /**
     * @brief Get schedule entry
     * @param index Entry index (0 to MAX_SCHEDULE_ENTRIES-1)
     */
    ScheduleEntry* getEntry(uint8_t index);
    
    /**
     * @brief Set schedule entry
     * @param index Entry index
     * @param hour Hour (0-23)
     * @param minute Minute (0-59)
     * @param duration Duration in seconds
     * @param enabled Enable this entry
     */
    void setEntry(uint8_t index, uint8_t hour, uint8_t minute, 
                  uint16_t duration, bool enabled);
    
    /**
     * @brief Get schedule config reference
     */
    ScheduleConfig& getConfig() { return _config; }
    
    /**
     * @brief Set callback to check if watering is needed
     */
    void setMoistureCallback(SchedulerMoistureCallback cb) { _moistureCb = cb; }
    
    /**
     * @brief Set callback to control pump
     */
    void setPumpCallback(SchedulerPumpCallback cb) { _pumpCb = cb; }
    
    /**
     * @brief Get number of enabled entries
     */
    uint8_t getEnabledCount() const;
    
    /**
     * @brief Get next scheduled time string
     */
    String getNextScheduleString() const;
    
    /**
     * @brief Check if currently in scheduled watering
     */
    bool isWatering() const { return _isWatering; }

private:
    ScheduleConfig _config;
    SchedulerMoistureCallback _moistureCb;
    SchedulerPumpCallback _pumpCb;
    
    bool _initialized;
    bool _isWatering;
    uint8_t _lastCheckedMinute;     // To prevent multiple triggers per minute
    uint8_t _currentEntryIndex;     // Currently running schedule entry
    unsigned long _wateringStartTime;
    uint16_t _wateringDuration;
    
    /**
     * @brief Check if current time matches a schedule entry
     * @param entry Schedule entry to check
     * @return true if matches
     */
    bool _matchesTime(const ScheduleEntry& entry) const;
    
    /**
     * @brief Start scheduled watering
     */
    void _startWatering(uint8_t entryIndex);
    
    /**
     * @brief Stop scheduled watering
     */
    void _stopWatering();
};

// Global instance
extern Scheduler scheduler;

#endif // SCHEDULER_H

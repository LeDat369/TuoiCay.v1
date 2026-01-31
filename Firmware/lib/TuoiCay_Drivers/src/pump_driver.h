/**
 * @file pump_driver.h
 * @brief Pump Control Driver with safety features
 * 
 * LOGIC:
 * - Control pump via MOSFET (HIGH = ON, LOW = OFF)
 * - Safety: auto-off after configurable max runtime
 * - Safety: minimum off time to prevent rapid cycling
 * - Track runtime and state
 * 
 * HARDWARE:
 * - D6 (GPIO12) → MOSFET Gate
 * - MOSFET: N-Channel, LOW-side switch
 * - Logic: HIGH = pump ON, LOW = pump OFF
 * 
 * RULES: #ACTUATOR(15) #SAFETY(2)
 */

#ifndef PUMP_DRIVER_H
#define PUMP_DRIVER_H

#include <Arduino.h>
#include <config.h>

//=============================================================================
// PUMP STATE ENUM
//=============================================================================
enum class PumpState : uint8_t {
    OFF = 0,        // Pump is off
    ON = 1,         // Pump is running
    COOLDOWN = 2    // Pump off, in minimum off-time cooldown
};

//=============================================================================
// PUMP REASON ENUM (why pump turned on)
//=============================================================================
enum class PumpReason : uint8_t {
    NONE = 0,       // Not running
    MANUAL = 1,     // Manual control (web/mqtt)
    AUTO = 2,       // Auto mode (threshold)
    SCHEDULE = 3    // Scheduled watering
};

//=============================================================================
// PUMP CONTROLLER CLASS
//=============================================================================

/**
 * @class PumpController
 * @brief Controls pump with safety features
 */
class PumpController {
public:
    /**
     * @brief Constructor
     * @param pin GPIO pin connected to MOSFET gate
     */
    PumpController(uint8_t pin);
    
    /**
     * @brief Initialize pump controller
     * CRITICAL: Sets pump OFF immediately (safe state)
     * @return true if successful
     */
    bool begin();
    
    /**
     * @brief Turn pump ON
     * @param reason Why pump is being turned on
     * @param duration Optional duration in seconds (0 = use max runtime)
     * @return true if pump was turned on, false if in cooldown
     */
    bool turnOn(PumpReason reason = PumpReason::MANUAL, uint16_t duration = 0);
    
    /**
     * @brief Turn pump OFF
     * @param startCooldown Whether to start cooldown timer
     */
    void turnOff(bool startCooldown = true);
    
    /**
     * @brief Toggle pump state
     * @param reason Reason if turning on
     * @return New state (true = on)
     */
    bool toggle(PumpReason reason = PumpReason::MANUAL);
    
    /**
     * @brief Update pump state (call in loop)
     * Handles:
     * - Auto-off after max runtime
     * - Cooldown timer expiry
     */
    void update();
    
    /**
     * @brief Check if pump is running
     * @return true if pump is ON
     */
    bool isRunning() const { return _state == PumpState::ON; }
    
    /**
     * @brief Check if pump is in cooldown
     * @return true if in cooldown period
     */
    bool isInCooldown() const { return _state == PumpState::COOLDOWN; }
    
    /**
     * @brief Get current pump state
     */
    PumpState getState() const { return _state; }
    
    /**
     * @brief Get reason pump is/was running
     */
    PumpReason getReason() const { return _reason; }
    
    /**
     * @brief Get reason as string
     */
    const char* getReasonString() const;
    
    /**
     * @brief Get current runtime in seconds
     * @return Seconds pump has been running (0 if off)
     */
    uint16_t getRuntime() const;
    
    /**
     * @brief Get remaining runtime before auto-off
     * @return Seconds until auto-off
     */
    uint16_t getRemainingTime() const;
    
    /**
     * @brief Get cooldown remaining in seconds
     * @return Seconds until cooldown ends (0 if not in cooldown)
     */
    uint16_t getCooldownRemaining() const;
    
    /**
     * @brief Set maximum runtime
     * @param seconds Max runtime before auto-off
     */
    void setMaxRuntime(uint16_t seconds);
    
    /**
     * @brief Set minimum off time (cooldown)
     * @param ms Minimum milliseconds between runs
     */
    void setMinOffTime(uint32_t ms);
    
    /**
     * @brief Emergency stop - immediately turn off pump
     * Does NOT start cooldown
     */
    void emergencyStop();
    
    /**
     * @brief Get last turn-on timestamp
     */
    unsigned long getLastOnTime() const { return _onTime; }
    
    /**
     * @brief Get last turn-off timestamp
     */
    unsigned long getLastOffTime() const { return _offTime; }

private:
    uint8_t _pin;                           // GPIO pin
    PumpState _state;                       // Current state
    PumpReason _reason;                     // Why pump is on
    
    unsigned long _onTime;                  // When pump turned on
    unsigned long _offTime;                 // When pump turned off
    
    uint16_t _maxRuntimeSec;                // Max runtime before auto-off
    uint16_t _requestedDuration;            // Requested duration for this run
    uint32_t _minOffTimeMs;                 // Minimum off time (cooldown)
    
    bool _initialized;
    
    /**
     * @brief Set physical pin state
     * @param on true = pump on
     */
    void _setPin(bool on);
};

#endif // PUMP_DRIVER_H

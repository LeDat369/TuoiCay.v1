/**
 * @file pump_driver.cpp
 * @brief Implementation of Pump Control Driver
 * 
 * LOGIC:
 * - SAFETY FIRST: Always default to OFF
 * - Auto-off: Turns off after max runtime (safety)
 * - Cooldown: Minimum off time prevents rapid cycling
 * - State machine: OFF -> ON -> OFF/COOLDOWN -> OFF
 * 
 * RULES: #ACTUATOR(15) #SAFETY(2)
 */

#include "pump_driver.h"
#include <pins.h>
#include <logger.h>

//=============================================================================
// PUMP CONTROLLER IMPLEMENTATION
//=============================================================================

PumpController::PumpController(uint8_t pin)
    : _pin(pin)
    , _state(PumpState::OFF)
    , _reason(PumpReason::NONE)
    , _onTime(0)
    , _offTime(0)
    , _maxRuntimeSec(PUMP_MAX_RUNTIME_SEC)
    , _requestedDuration(0)
    , _minOffTimeMs(PUMP_MIN_OFF_TIME_MS)
    , _speedPercent(PUMP_SPEED_DEFAULT)
    , _initialized(false)
{
}

bool PumpController::begin() {
    // CRITICAL: Set pin mode and OFF state FIRST
    pinMode(_pin, OUTPUT);
    
    // Configure PWM
    analogWriteFreq(PUMP_PWM_FREQ);
    analogWriteRange(PUMP_PWM_RANGE);
    
    _setPin(false);  // Pump OFF
    
    _state = PumpState::OFF;
    _reason = PumpReason::NONE;
    // Set _offTime far in the past so no cooldown at boot
    _offTime = millis() - _minOffTimeMs - 1000;
    _initialized = true;
    
    LOG_INF(MOD_PUMP, "init", "Ready (pin=%d, maxRun=%ds, cooldown=%lums, speed=%d%%)",
            _pin, _maxRuntimeSec, _minOffTimeMs, _speedPercent);
    
    return true;
}

bool PumpController::turnOn(PumpReason reason, uint16_t duration) {
    if (!_initialized) {
        LOG_ERR(MOD_PUMP, "on", "Not initialized!");
        return false;
    }
    
    // Check if already on
    if (_state == PumpState::ON) {
        LOG_WRN(MOD_PUMP, "on", "Already running");
        return true;
    }
    
    // Check cooldown - ONLY for AUTO mode
    // Manual and Schedule bypass cooldown for immediate control
    if (_state == PumpState::COOLDOWN && reason == PumpReason::AUTO) {
        uint16_t remaining = getCooldownRemaining();
        LOG_WRN(MOD_PUMP, "on", "Auto mode in cooldown, %ds remaining", remaining);
        return false;
    }
    
    // If in cooldown but not AUTO, clear cooldown state
    if (_state == PumpState::COOLDOWN) {
        _state = PumpState::OFF;
        LOG_INF(MOD_PUMP, "on", "Cooldown bypassed for %s mode", 
                reason == PumpReason::MANUAL ? "manual" : "schedule");
    }
    
    // Set duration
    if (duration > 0 && duration < _maxRuntimeSec) {
        _requestedDuration = duration;
    } else {
        _requestedDuration = _maxRuntimeSec;
    }
    
    // Turn on
    _setPin(true);
    _state = PumpState::ON;
    _reason = reason;
    _onTime = millis();
    
    LOG_INF(MOD_PUMP, "on", "Started (reason=%s, duration=%ds)",
            getReasonString(), _requestedDuration);
    
    return true;
}

void PumpController::turnOff(bool startCooldown) {
    if (!_initialized) return;
    
    // Get runtime before turning off
    uint16_t runtime = getRuntime();
    
    // Turn off
    _setPin(false);
    _offTime = millis();
    
    if (startCooldown && _minOffTimeMs > 0) {
        _state = PumpState::COOLDOWN;
        LOG_INF(MOD_PUMP, "off", "Stopped after %ds, cooldown=%lus",
                runtime, _minOffTimeMs / 1000);
    } else {
        _state = PumpState::OFF;
        LOG_INF(MOD_PUMP, "off", "Stopped after %ds", runtime);
    }
    
    _reason = PumpReason::NONE;
    _requestedDuration = 0;
}

bool PumpController::toggle(PumpReason reason) {
    if (_state == PumpState::ON) {
        turnOff();
        return false;
    } else {
        return turnOn(reason);
    }
}

void PumpController::update() {
    if (!_initialized) return;
    
    unsigned long now = millis();
    
    switch (_state) {
        case PumpState::ON: {
            // Check for auto-off (max runtime exceeded)
            uint16_t runtime = getRuntime();
            if (runtime >= _requestedDuration) {
                LOG_WRN(MOD_PUMP, "safety", "Auto-off: max runtime %ds reached", 
                        _requestedDuration);
                turnOff(true);  // Start cooldown
            }
            break;
        }
        
        case PumpState::COOLDOWN: {
            // Check if cooldown expired
            if (now - _offTime >= _minOffTimeMs) {
                _state = PumpState::OFF;
                LOG_INF(MOD_PUMP, "cooldown", "Cooldown complete");
            }
            break;
        }
        
        case PumpState::OFF:
        default:
            // Nothing to do
            break;
    }
}

const char* PumpController::getReasonString() const {
    switch (_reason) {
        case PumpReason::MANUAL:   return "manual";
        case PumpReason::AUTO:     return "auto";
        case PumpReason::SCHEDULE: return "schedule";
        default:                   return "none";
    }
}

uint16_t PumpController::getRuntime() const {
    if (_state != PumpState::ON) {
        return 0;
    }
    return (uint16_t)((millis() - _onTime) / 1000);
}

uint16_t PumpController::getRemainingTime() const {
    if (_state != PumpState::ON) {
        return 0;
    }
    uint16_t runtime = getRuntime();
    if (runtime >= _requestedDuration) {
        return 0;
    }
    return _requestedDuration - runtime;
}

uint16_t PumpController::getCooldownRemaining() const {
    if (_state != PumpState::COOLDOWN) {
        return 0;
    }
    unsigned long elapsed = millis() - _offTime;
    if (elapsed >= _minOffTimeMs) {
        return 0;
    }
    return (uint16_t)((_minOffTimeMs - elapsed) / 1000);
}

void PumpController::setMaxRuntime(uint16_t seconds) {
    _maxRuntimeSec = seconds;
    LOG_INF(MOD_PUMP, "config", "Max runtime set to %ds", seconds);
}

void PumpController::setMinOffTime(uint32_t ms) {
    _minOffTimeMs = ms;
    LOG_INF(MOD_PUMP, "config", "Min off time set to %lums", ms);
}

void PumpController::setSpeed(uint8_t percent) {
    // Clamp to valid range
    if (percent < PUMP_SPEED_MIN) percent = PUMP_SPEED_MIN;
    if (percent > PUMP_SPEED_MAX) percent = PUMP_SPEED_MAX;
    
    _speedPercent = percent;
    LOG_INF(MOD_PUMP, "config", "Speed set to %d%%", percent);
    
    // If pump is running, apply new speed immediately
    if (_state == PumpState::ON) {
        _applyPWM();
    }
}

void PumpController::emergencyStop() {
    LOG_ERR(MOD_PUMP, "ESTOP", "EMERGENCY STOP!");
    _setPin(false);
    _state = PumpState::OFF;
    _reason = PumpReason::NONE;
    _offTime = millis();
    // No cooldown on emergency stop - allow immediate restart if needed
}

void PumpController::_setPin(bool on) {
    if (on) {
        _applyPWM();
    } else {
        analogWrite(_pin, 0);  // PWM off
    }
}

void PumpController::_applyPWM() {
    // Convert percent to PWM value (0-1023)
    uint16_t pwmValue = (_speedPercent * PUMP_PWM_RANGE) / 100;
    analogWrite(_pin, pwmValue);
}

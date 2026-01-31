/**
 * @file sensor_driver.cpp
 * @brief Implementation of Soil Moisture Sensor Driver
 * 
 * LOGIC:
 * - SoilSensor: Individual sensor handling with filtering
 * - SensorManager: Coordinates both sensors
 * - Moving average filter smooths noisy readings
 * - Calibration maps ADC to moisture %
 * 
 * RULES: #SENSOR(13) #CORE(1.3)
 */

#include "sensor_driver.h"
#include <pins.h>
#include <logger.h>

//=============================================================================
// SOIL SENSOR IMPLEMENTATION
//=============================================================================

SoilSensor::SoilSensor(int8_t digitalPin, int8_t analogPin, uint8_t id)
    : _digitalPin(digitalPin)
    , _analogPin(analogPin)
    , _id(id)
    , _digitalValue(true)       // Default: dry (safe assumption)
    , _analogValue(ADC_DRY_VALUE)
    , _moisturePercent(0)
    , _filterIndex(0)
    , _filterFilled(false)
    , _calDry(ADC_DRY_VALUE)
    , _calWet(ADC_WET_VALUE)
    , _lastReadTime(0)
    , _initialized(false)
{
    // Initialize filter buffer to dry value
    for (uint8_t i = 0; i < SENSOR_FILTER_SIZE; i++) {
        _filterBuffer[i] = ADC_DRY_VALUE;
    }
}

bool SoilSensor::begin() {
    // Configure digital pin as input with pullup
    if (_digitalPin >= 0) {
        pinMode(_digitalPin, INPUT_PULLUP);
    }
    
    // Note: A0 on ESP8266 doesn't need pinMode configuration
    // It's always analog input
    
    _initialized = true;
    LOG_INF(MOD_SENSOR, "init", "Sensor %d ready (D=%d, A=%d)", 
            _id, _digitalPin, _analogPin);
    
    // Do initial reading
    update();
    
    return true;
}

bool SoilSensor::readDigital() {
    if (_digitalPin < 0) {
        return true;  // No digital pin, assume dry
    }
    
    // Sensor logic: LOW = wet (moisture detected), HIGH = dry
    return digitalRead(_digitalPin) == HIGH;
}

uint16_t SoilSensor::readAnalogRaw() {
    if (_analogPin < 0) {
        return 0;
    }
    
    return analogRead(_analogPin);
}

uint16_t SoilSensor::readAnalogFiltered() {
    if (_analogPin < 0) {
        return 0;
    }
    
    return _analogValue;
}

uint8_t SoilSensor::getMoisturePercent() {
    if (_analogPin < 0) {
        return SENSOR_INVALID_VALUE;
    }
    
    return _moisturePercent;
}

bool SoilSensor::isValid() {
    // Check if reading is within reasonable range
    if (_analogPin >= 0) {
        // For analog: check if value is in calibration range
        // Allow some margin (±10%) for sensor variation
        uint16_t margin = (_calDry - _calWet) / 10;
        return (_analogValue >= (_calWet - margin)) && 
               (_analogValue <= (_calDry + margin));
    }
    
    // Digital-only sensor is always valid
    return true;
}

void SoilSensor::update() {
    // Read digital value
    _digitalValue = readDigital();
    
    // Read and filter analog value if available
    if (_analogPin >= 0) {
        uint16_t rawValue = readAnalogRaw();
        _analogValue = _addToFilter(rawValue);
        _moisturePercent = _adcToPercent(_analogValue);
    }
    
    _lastReadTime = millis();
}

void SoilSensor::setCalibration(uint16_t dryValue, uint16_t wetValue) {
    _calDry = dryValue;
    _calWet = wetValue;
    LOG_INF(MOD_SENSOR, "cal", "Sensor %d calibrated: dry=%u, wet=%u", 
            _id, dryValue, wetValue);
}

uint16_t SoilSensor::_addToFilter(uint16_t value) {
    // Add new value to buffer
    _filterBuffer[_filterIndex] = value;
    _filterIndex = (_filterIndex + 1) % SENSOR_FILTER_SIZE;
    
    if (_filterIndex == 0) {
        _filterFilled = true;
    }
    
    // Calculate moving average
    uint32_t sum = 0;
    uint8_t count = _filterFilled ? SENSOR_FILTER_SIZE : _filterIndex;
    
    for (uint8_t i = 0; i < count; i++) {
        sum += _filterBuffer[i];
    }
    
    return (uint16_t)(sum / count);
}

uint8_t SoilSensor::_adcToPercent(uint16_t adcValue) {
    // Constrain ADC value to calibration range
    if (adcValue >= _calDry) return 0;    // Completely dry
    if (adcValue <= _calWet) return 100;  // Completely wet
    
    // Map ADC to moisture %
    // Note: Inverted because higher ADC = drier
    // Formula: (dry - adc) * 100 / (dry - wet)
    uint32_t range = _calDry - _calWet;
    uint32_t value = _calDry - adcValue;
    
    return (uint8_t)((value * 100) / range);
}

//=============================================================================
// SENSOR MANAGER IMPLEMENTATION
//=============================================================================

SensorManager::SensorManager()
    : _sensor1(PIN_SENSOR1_DIGITAL, -1, 1)                     // Digital only
    , _sensor2(PIN_SENSOR2_DIGITAL, PIN_SENSOR2_ANALOG, 2)     // Digital + Analog
    , _lastUpdateTime(0)
{
}

bool SensorManager::begin() {
    LOG_INF(MOD_SENSOR, "init", "Initializing sensors...");
    
    bool ok1 = _sensor1.begin();
    bool ok2 = _sensor2.begin();
    
    if (ok1 && ok2) {
        LOG_INF(MOD_SENSOR, "init", "All sensors initialized successfully");
        return true;
    } else {
        LOG_ERR(MOD_SENSOR, "init", "Sensor init failed: S1=%d, S2=%d", ok1, ok2);
        return false;
    }
}

void SensorManager::update() {
    _sensor1.update();
    _sensor2.update();
    _lastUpdateTime = millis();
}

uint8_t SensorManager::getAverageMoisture() {
    // Get moisture from sensors with analog capability
    uint8_t m2 = _sensor2.getMoisturePercent();
    
    if (m2 != SENSOR_INVALID_VALUE) {
        return m2;  // Only sensor 2 has analog
    }
    
    // Fallback: use digital reading (rough estimate)
    // If digital says wet, assume ~70%, if dry assume ~20%
    if (!_sensor1.readDigital() || !_sensor2.readDigital()) {
        return 70;  // At least one sensor wet
    }
    return 20;  // Both dry
}

bool SensorManager::isAnyDry() {
    return _sensor1.readDigital() || _sensor2.readDigital();
}

bool SensorManager::isAllWet() {
    return !_sensor1.readDigital() && !_sensor2.readDigital();
}

void SensorManager::logReadings() {
    const char* s1Str = _sensor1.readDigital() ? "DRY" : "WET";
    const char* s2Str = _sensor2.readDigital() ? "DRY" : "WET";
    
    if (_sensor2.hasAnalog()) {
        LOG_INF(MOD_SENSOR, "read", "S1=%s, S2=%s, M=%d%%, raw=%u",
                s1Str, s2Str, 
                _sensor2.getMoisturePercent(),
                _sensor2.readAnalogFiltered());
    } else {
        LOG_INF(MOD_SENSOR, "read", "S1=%s, S2=%s", s1Str, s2Str);
    }
}

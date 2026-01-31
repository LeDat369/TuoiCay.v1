/**
 * @file sensor_driver.h
 * @brief Soil Moisture Sensor Driver for capacitive sensors
 * 
 * LOGIC:
 * - Support 2 sensors: Sensor1 (digital only), Sensor2 (digital + analog)
 * - Digital: LOW = wet (moisture detected), HIGH = dry
 * - Analog: 0-1023, higher = drier (capacitive sensor characteristic)
 * - Moving average filter for stable readings
 * - Moisture % calculation with calibration
 * 
 * HARDWARE:
 * - Sensor 1: D5 (GPIO14) - Digital output
 * - Sensor 2: D1 (GPIO5) - Digital output, A0 - Analog output
 * 
 * RULES: #SENSOR(13) #CORE(1.3)
 */

#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

#include <Arduino.h>
#include <config.h>

//=============================================================================
// CONSTANTS
//=============================================================================
#define SENSOR_FILTER_SIZE      SENSOR_FILTER_SAMPLES   // From config.h (5)
#define SENSOR_INVALID_VALUE    255                     // Invalid reading marker

//=============================================================================
// SENSOR CLASS
//=============================================================================

/**
 * @class SoilSensor
 * @brief Class to handle soil moisture sensor reading with filtering
 */
class SoilSensor {
public:
    /**
     * @brief Constructor
     * @param digitalPin GPIO pin for digital output
     * @param analogPin Analog pin (use -1 if no analog)
     * @param id Sensor identifier (1 or 2)
     */
    SoilSensor(int8_t digitalPin, int8_t analogPin = -1, uint8_t id = 1);
    
    /**
     * @brief Initialize sensor pins
     * @return true if successful
     */
    bool begin();
    
    /**
     * @brief Read digital value (dry/wet)
     * @return true = dry (no moisture), false = wet (moisture detected)
     */
    bool readDigital();
    
    /**
     * @brief Read raw analog value (0-1023)
     * @return Raw ADC value, or 0 if no analog pin configured
     */
    uint16_t readAnalogRaw();
    
    /**
     * @brief Read analog value with moving average filter
     * @return Filtered ADC value
     */
    uint16_t readAnalogFiltered();
    
    /**
     * @brief Get moisture percentage (0-100%)
     * @return Moisture %, or SENSOR_INVALID_VALUE if no analog available
     * 
     * LOGIC:
     * - Uses calibration values ADC_DRY_VALUE and ADC_WET_VALUE from config.h
     * - Maps analog reading to 0-100% range
     * - Inverted because higher ADC = drier
     */
    uint8_t getMoisturePercent();
    
    /**
     * @brief Check if sensor reading is valid
     * @return true if reading is within valid range
     */
    bool isValid();
    
    /**
     * @brief Update sensor reading (call periodically)
     * Reads sensor and updates internal state
     */
    void update();
    
    /**
     * @brief Get sensor ID
     * @return Sensor identifier
     */
    uint8_t getId() const { return _id; }
    
    /**
     * @brief Check if sensor has analog capability
     * @return true if analog pin is configured
     */
    bool hasAnalog() const { return _analogPin >= 0; }
    
    /**
     * @brief Set calibration values
     * @param dryValue ADC value when dry
     * @param wetValue ADC value when wet
     */
    void setCalibration(uint16_t dryValue, uint16_t wetValue);
    
    /**
     * @brief Get last read timestamp
     * @return millis() of last reading
     */
    unsigned long getLastReadTime() const { return _lastReadTime; }

private:
    int8_t _digitalPin;                     // Digital input pin
    int8_t _analogPin;                      // Analog input pin (-1 if none)
    uint8_t _id;                            // Sensor identifier
    
    bool _digitalValue;                     // Current digital reading
    uint16_t _analogValue;                  // Current filtered analog reading
    uint8_t _moisturePercent;               // Current moisture %
    
    uint16_t _filterBuffer[SENSOR_FILTER_SIZE]; // Moving average buffer
    uint8_t _filterIndex;                   // Current buffer index
    bool _filterFilled;                     // Buffer fully populated?
    
    uint16_t _calDry;                       // Calibration: dry ADC value
    uint16_t _calWet;                       // Calibration: wet ADC value
    
    unsigned long _lastReadTime;            // Last read timestamp
    bool _initialized;                      // Initialization flag
    
    /**
     * @brief Add value to filter and get average
     * @param value New value to add
     * @return Moving average
     */
    uint16_t _addToFilter(uint16_t value);
    
    /**
     * @brief Map ADC value to moisture percentage
     * @param adcValue Raw or filtered ADC value
     * @return Moisture % (0-100)
     */
    uint8_t _adcToPercent(uint16_t adcValue);
};

//=============================================================================
// SENSOR MANAGER (handles both sensors)
//=============================================================================

/**
 * @class SensorManager
 * @brief Manages multiple soil moisture sensors
 */
class SensorManager {
public:
    /**
     * @brief Constructor - creates sensors based on pins.h configuration
     */
    SensorManager();
    
    /**
     * @brief Initialize all sensors
     * @return true if all sensors initialized successfully
     */
    bool begin();
    
    /**
     * @brief Update all sensors (call periodically, e.g., every 5s)
     */
    void update();
    
    /**
     * @brief Get sensor 1 reference
     */
    SoilSensor& getSensor1() { return _sensor1; }
    
    /**
     * @brief Get sensor 2 reference
     */
    SoilSensor& getSensor2() { return _sensor2; }
    
    /**
     * @brief Get average moisture from all sensors with analog
     * @return Average moisture %, or first valid reading
     */
    uint8_t getAverageMoisture();
    
    /**
     * @brief Check if any sensor detects dry condition
     * @return true if any digital sensor reads dry
     */
    bool isAnyDry();
    
    /**
     * @brief Check if all sensors detect wet condition
     * @return true if all digital sensors read wet
     */
    bool isAllWet();
    
    /**
     * @brief Log current sensor readings
     */
    void logReadings();

private:
    SoilSensor _sensor1;
    SoilSensor _sensor2;
    unsigned long _lastUpdateTime;
};

#endif // SENSOR_DRIVER_H

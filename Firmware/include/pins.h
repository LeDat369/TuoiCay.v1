/**
 * @file pins.h
 * @brief GPIO pin definitions for ESP8266 NodeMCU
 * 
 * LOGIC:
 * - Map NodeMCU D-pins to GPIO numbers
 * - Define all hardware connections
 * - Use descriptive names for each pin function
 * 
 * HARDWARE MAPPING (from Pinout.md):
 * - D6 (GPIO12) → MOSFET Gate (pump control)
 * - D5 (GPIO14) → Sensor 1 Digital
 * - D1 (GPIO5)  → Sensor 2 Digital
 * - A0 (ADC)    → Sensor 2 Analog
 * - LED_BUILTIN → Status indicator
 * 
 * RULES: #GPIO(11) - Pin definitions
 */

#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

//=============================================================================
// NodeMCU PIN MAPPING (D-pin to GPIO)
//=============================================================================
// Reference: NodeMCU ESP8266 pinout
// D0 = GPIO16 (no PWM, no interrupt)
// D1 = GPIO5  (I2C SCL)
// D2 = GPIO4  (I2C SDA)
// D3 = GPIO0  (FLASH button, boot mode)
// D4 = GPIO2  (LED_BUILTIN, boot mode)
// D5 = GPIO14 (HSPI CLK)
// D6 = GPIO12 (HSPI MISO)
// D7 = GPIO13 (HSPI MOSI)
// D8 = GPIO15 (HSPI CS, boot mode - must be LOW)
// A0 = ADC0   (Analog input, 0-1V)

//=============================================================================
// PUMP CONTROL
//=============================================================================
#define PIN_PUMP            12          // D6 (GPIO12) - MOSFET Gate
#define PIN_PUMP_D          D6          // NodeMCU alias

// MOSFET Logic: HIGH = Pump ON, LOW = Pump OFF
#define PUMP_ON             HIGH
#define PUMP_OFF            LOW

//=============================================================================
// SOIL MOISTURE SENSORS
//=============================================================================
// Sensor 1 - Digital only
#define PIN_SENSOR1_DIGITAL 14          // D5 (GPIO14)
#define PIN_SENSOR1_D       D5          // NodeMCU alias

// Sensor 2 - Digital + Analog
#define PIN_SENSOR2_DIGITAL 5           // D1 (GPIO5)
#define PIN_SENSOR2_D       D1          // NodeMCU alias
#define PIN_SENSOR2_ANALOG  A0          // ADC input

// Sensor Logic: LOW = Wet (detected moisture), HIGH = Dry (no moisture)
#define SENSOR_WET          LOW
#define SENSOR_DRY          HIGH

//=============================================================================
// STATUS LED
//=============================================================================
#define PIN_LED_STATUS      LED_BUILTIN // Built-in LED (GPIO2, active LOW)

// LED Logic: Active LOW on NodeMCU
#define LED_ON              LOW
#define LED_OFF             HIGH

//=============================================================================
// RESERVED PINS (DO NOT USE)
//=============================================================================
// D3 (GPIO0)  - FLASH button, affects boot mode
// D4 (GPIO2)  - LED_BUILTIN, affects boot mode (must be HIGH at boot)
// D8 (GPIO15) - Must be LOW at boot
// TX (GPIO1)  - Serial TX
// RX (GPIO3)  - Serial RX

//=============================================================================
// PIN INITIALIZATION FUNCTION
//=============================================================================
/**
 * @brief Initialize all GPIO pins to safe state
 * 
 * LOGIC:
 * 1. Set pump pin as OUTPUT, initially OFF (SAFETY FIRST!)
 * 2. Set sensor digital pins as INPUT
 * 3. Set LED as OUTPUT
 * 
 * MUST be called at the beginning of setup() before any other initialization
 */
inline void pins_init_safe() {
    // CRITICAL: Pump OFF first (safety)
    pinMode(PIN_PUMP, OUTPUT);
    digitalWrite(PIN_PUMP, PUMP_OFF);
    
    // Sensor digital inputs (internal pullup)
    pinMode(PIN_SENSOR1_DIGITAL, INPUT_PULLUP);
    pinMode(PIN_SENSOR2_DIGITAL, INPUT_PULLUP);
    
    // Status LED
    pinMode(PIN_LED_STATUS, OUTPUT);
    digitalWrite(PIN_LED_STATUS, LED_OFF);
}

/**
 * @brief Set all actuators to safe state (emergency stop)
 * 
 * Call this on:
 * - Boot
 * - Watchdog reset
 * - Error conditions
 * - Before entering deep sleep
 */
inline void gpio_set_safe() {
    digitalWrite(PIN_PUMP, PUMP_OFF);
    digitalWrite(PIN_LED_STATUS, LED_OFF);
}

#endif // PINS_H

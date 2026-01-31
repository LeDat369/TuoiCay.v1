/**
 * @file config.h
 * @brief Main configuration file - Version, constants, timeouts
 * 
 * LOGIC:
 * - Định nghĩa version firmware theo SemVer
 * - Các timeout cho WiFi, MQTT, sensors
 * - Các threshold mặc định cho auto watering
 * - Device identification
 * 
 * RULES: #CORE(1.2) - Project configuration
 */

#ifndef CONFIG_H
#define CONFIG_H

//=============================================================================
// FIRMWARE VERSION (SemVer)
//=============================================================================
#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    0
#define FW_VERSION          "1.0.0"
#define FW_NAME             "TuoiCay"

//=============================================================================
// DEVICE IDENTIFICATION
//=============================================================================
#define DEVICE_TYPE         "TUOICAY_V1"
#define DEVICE_PREFIX       "TC"        // Prefix for device ID

//=============================================================================
// TIMING CONSTANTS (milliseconds)
//=============================================================================
// Watchdog
#define WDT_TIMEOUT_SEC         30      // Watchdog timeout in seconds

// WiFi
#define WIFI_CONNECT_TIMEOUT_MS 30000   // 30s WiFi connection timeout
#define WIFI_RECONNECT_MIN_MS   2000    // Min reconnect delay
#define WIFI_RECONNECT_MAX_MS   30000   // Max reconnect delay (exponential backoff)

// MQTT
#define MQTT_CONNECT_TIMEOUT_MS 10000   // 10s MQTT connection timeout
#define MQTT_RECONNECT_MIN_MS   2000    // Min reconnect delay
#define MQTT_RECONNECT_MAX_MS   30000   // Max reconnect delay
#define MQTT_KEEPALIVE_SEC      60      // MQTT keepalive interval
#define MQTT_OFFLINE_QUEUE_SIZE 10      // Max messages in offline queue

// Sensors
#define SENSOR_READ_INTERVAL_MS 5000    // Read sensors every 5s
#define SENSOR_FILTER_SAMPLES   5       // Moving average samples

// Pump
#define PUMP_MAX_RUNTIME_SEC    60      // Auto-off after 60s (safety)
#define PUMP_MIN_OFF_TIME_MS    30000   // 30 seconds minimum off time (anti-cycling)

// NTP
#define NTP_SYNC_INTERVAL_MS    21600000 // 6 hours
#define NTP_TIMEZONE_OFFSET     7       // UTC+7 Vietnam

//=============================================================================
// DEFAULT THRESHOLDS
//=============================================================================
#define DEFAULT_THRESHOLD_DRY   30      // % moisture to start watering
#define DEFAULT_THRESHOLD_WET   50      // % moisture to stop watering
#define MOISTURE_MIN_VALID      0       // Minimum valid moisture %
#define MOISTURE_MAX_VALID      100     // Maximum valid moisture %

//=============================================================================
// ANALOG CALIBRATION (ESP8266 ADC 0-1023)
//=============================================================================
#define ADC_DRY_VALUE           1023    // ADC value when sensor is dry
#define ADC_WET_VALUE           300     // ADC value when sensor is wet

//=============================================================================
// SERIAL CONFIGURATION
//=============================================================================
#define SERIAL_BAUD_RATE        115200

//=============================================================================
// BUFFER SIZES
//=============================================================================
#define JSON_BUFFER_SIZE        256     // JSON document buffer
#define TOPIC_BUFFER_SIZE       64      // MQTT topic buffer
#define LOG_BUFFER_SIZE         128     // Log message buffer

#endif // CONFIG_H

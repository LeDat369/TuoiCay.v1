/**
 * @file error_codes.h
 * @brief Error code definitions for all modules
 * 
 * LOGIC:
 * - Error codes grouped by module (1xxx, 2xxx, etc.)
 * - 0 = Success (OK)
 * - Consistent naming: TC_ERR_MODULE_DESCRIPTION (TC = TuoiCay prefix to avoid conflicts)
 * 
 * RULES: #ERROR(6) - Error handling
 */

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

//=============================================================================
// SUCCESS
// Note: Using TC_ prefix to avoid conflict with lwip ERR_OK
//=============================================================================
#define TC_ERR_OK               0       // Success, no error

//=============================================================================
// WIFI ERRORS (1xxx)
//=============================================================================
#define TC_ERR_WIFI_CONNECT_FAIL   1001    // Failed to connect to WiFi
#define TC_ERR_WIFI_TIMEOUT        1002    // Connection timeout
#define TC_ERR_WIFI_WRONG_PASS     1003    // Wrong password
#define TC_ERR_WIFI_NO_SSID        1004    // SSID not found
#define TC_ERR_WIFI_DISCONNECTED   1005    // Unexpectedly disconnected

//=============================================================================
// MQTT ERRORS (2xxx)
//=============================================================================
#define TC_ERR_MQTT_CONNECT_FAIL   2001    // Failed to connect to broker
#define TC_ERR_MQTT_PUBLISH_FAIL   2002    // Failed to publish message
#define TC_ERR_MQTT_SUBSCRIBE_FAIL 2003    // Failed to subscribe to topic
#define TC_ERR_MQTT_TIMEOUT        2004    // Operation timeout
#define TC_ERR_MQTT_DISCONNECTED   2005    // Unexpectedly disconnected

//=============================================================================
// SENSOR ERRORS (3xxx)
//=============================================================================
#define TC_ERR_SENSOR_NOT_FOUND    3001    // Sensor not detected
#define TC_ERR_SENSOR_READ_FAIL    3002    // Failed to read sensor
#define TC_ERR_SENSOR_OUT_OF_RANGE 3003    // Value out of valid range
#define TC_ERR_SENSOR_TIMEOUT      3004    // Sensor read timeout

//=============================================================================
// STORAGE ERRORS (4xxx)
//=============================================================================
#define TC_ERR_STORAGE_INIT_FAIL   4001    // Failed to initialize storage
#define TC_ERR_STORAGE_READ_FAIL   4002    // Failed to read from storage
#define TC_ERR_STORAGE_WRITE_FAIL  4003    // Failed to write to storage
#define TC_ERR_STORAGE_CRC_FAIL    4004    // CRC verification failed
#define TC_ERR_STORAGE_FULL        4005    // Storage full

//=============================================================================
// OTA ERRORS (5xxx)
//=============================================================================
#define TC_ERR_OTA_DOWNLOAD_FAIL   5001    // Failed to download firmware
#define TC_ERR_OTA_VERIFY_FAIL     5002    // Firmware verification failed
#define TC_ERR_OTA_FLASH_FAIL      5003    // Failed to flash firmware
#define TC_ERR_OTA_NO_SPACE        5004    // Not enough space for OTA

//=============================================================================
// PUMP/ACTUATOR ERRORS (6xxx)
//=============================================================================
#define TC_ERR_PUMP_TIMEOUT        6001    // Pump ran too long (safety cutoff)
#define TC_ERR_PUMP_OVERCURRENT    6002    // Overcurrent detected (future)
#define TC_ERR_PUMP_SAFETY_TRIP    6003    // Safety mechanism triggered

//=============================================================================
// SYSTEM ERRORS (9xxx)
//=============================================================================
#define TC_ERR_SYSTEM_INIT_FAIL    9001    // System initialization failed
#define TC_ERR_SYSTEM_OUT_OF_MEM   9002    // Out of memory
#define TC_ERR_SYSTEM_WDT_RESET    9003    // Watchdog timer reset
#define TC_ERR_SYSTEM_INVALID_ARG  9004    // Invalid argument

//=============================================================================
// ERROR CODE TO STRING (for logging)
//=============================================================================
inline const char* error_to_string(int err) {
    switch (err) {
        case TC_ERR_OK:                    return "OK";
        case TC_ERR_WIFI_CONNECT_FAIL:     return "WIFI_CONNECT_FAIL";
        case TC_ERR_WIFI_TIMEOUT:          return "WIFI_TIMEOUT";
        case TC_ERR_MQTT_CONNECT_FAIL:     return "MQTT_CONNECT_FAIL";
        case TC_ERR_MQTT_PUBLISH_FAIL:     return "MQTT_PUBLISH_FAIL";
        case TC_ERR_SENSOR_NOT_FOUND:      return "SENSOR_NOT_FOUND";
        case TC_ERR_SENSOR_READ_FAIL:      return "SENSOR_READ_FAIL";
        case TC_ERR_STORAGE_INIT_FAIL:     return "STORAGE_INIT_FAIL";
        case TC_ERR_STORAGE_CRC_FAIL:      return "STORAGE_CRC_FAIL";
        case TC_ERR_PUMP_TIMEOUT:          return "PUMP_TIMEOUT";
        case TC_ERR_PUMP_SAFETY_TRIP:      return "PUMP_SAFETY_TRIP";
        default:                           return "UNKNOWN_ERROR";
    }
}

#endif // ERROR_CODES_H

/**
 * @file logger.h
 * @brief Logging macros with module and function tags
 * 
 * LOGIC:
 * - Format: [LEVEL][MODULE][func] message
 * - Levels: INF (Info), WRN (Warning), ERR (Error), DBG (Debug)
 * - Module names: SYSTEM, WIFI, MQTT, SENSOR, PUMP, STORAGE, WEB, TIME, OTA
 * - Uses Serial output with configurable baud rate
 * 
 * USAGE:
 *   LOG_INF("WIFI", "connect", "Connected to %s", ssid);
 *   // Output: [INF][WIFI][connect] Connected to MyNetwork
 * 
 * RULES: #LOG(7) - Logging standards
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

//=============================================================================
// LOG LEVEL CONFIGURATION
//=============================================================================
#define LOG_LEVEL_NONE      0
#define LOG_LEVEL_ERROR     1
#define LOG_LEVEL_WARNING   2
#define LOG_LEVEL_INFO      3
#define LOG_LEVEL_DEBUG     4

// Set current log level (compile-time)
#ifndef LOG_LEVEL
#define LOG_LEVEL           LOG_LEVEL_INFO
#endif

//=============================================================================
// COLOR CODES (for terminals that support ANSI)
//=============================================================================
#ifdef LOG_USE_COLORS
    #define LOG_COLOR_RESET     "\033[0m"
    #define LOG_COLOR_RED       "\033[31m"
    #define LOG_COLOR_YELLOW    "\033[33m"
    #define LOG_COLOR_GREEN     "\033[32m"
    #define LOG_COLOR_CYAN      "\033[36m"
#else
    #define LOG_COLOR_RESET     ""
    #define LOG_COLOR_RED       ""
    #define LOG_COLOR_YELLOW    ""
    #define LOG_COLOR_GREEN     ""
    #define LOG_COLOR_CYAN      ""
#endif

//=============================================================================
// INTERNAL LOGGING FUNCTION
//=============================================================================
/**
 * @brief Internal printf-style logging function
 * @param level Log level string (INF, WRN, ERR, DBG)
 * @param module Module name (WIFI, MQTT, etc.)
 * @param func Function/action name
 * @param format Printf format string
 * @param ... Variable arguments
 */
inline void _log_printf(const char* color, const char* level, const char* module, 
                        const char* func, const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.printf("%s[%s][%s][%s] %s%s\r\n", 
                  color, level, module, func, buffer, LOG_COLOR_RESET);
}

//=============================================================================
// LOGGING MACROS
//=============================================================================

// ERROR level - Always enabled unless LOG_LEVEL_NONE
#if LOG_LEVEL >= LOG_LEVEL_ERROR
    #define LOG_ERR(module, func, fmt, ...) \
        _log_printf(LOG_COLOR_RED, "ERR", module, func, fmt, ##__VA_ARGS__)
#else
    #define LOG_ERR(module, func, fmt, ...) ((void)0)
#endif

// WARNING level
#if LOG_LEVEL >= LOG_LEVEL_WARNING
    #define LOG_WRN(module, func, fmt, ...) \
        _log_printf(LOG_COLOR_YELLOW, "WRN", module, func, fmt, ##__VA_ARGS__)
#else
    #define LOG_WRN(module, func, fmt, ...) ((void)0)
#endif

// INFO level
#if LOG_LEVEL >= LOG_LEVEL_INFO
    #define LOG_INF(module, func, fmt, ...) \
        _log_printf(LOG_COLOR_GREEN, "INF", module, func, fmt, ##__VA_ARGS__)
#else
    #define LOG_INF(module, func, fmt, ...) ((void)0)
#endif

// DEBUG level
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    #define LOG_DBG(module, func, fmt, ...) \
        _log_printf(LOG_COLOR_CYAN, "DBG", module, func, fmt, ##__VA_ARGS__)
#else
    #define LOG_DBG(module, func, fmt, ...) ((void)0)
#endif

//=============================================================================
// MODULE NAME CONSTANTS (for consistency)
//=============================================================================
#define MOD_SYSTEM      "SYSTEM"
#define MOD_WIFI        "WIFI"
#define MOD_MQTT        "MQTT"
#define MOD_SENSOR      "SENSOR"
#define MOD_PUMP        "PUMP"
#define MOD_STORAGE     "STORAGE"
#define MOD_WEB         "WEB"
#define MOD_TIME        "TIME"
#define MOD_OTA         "OTA"
#define MOD_SCHED       "SCHED"

//=============================================================================
// LOGGER INITIALIZATION
//=============================================================================
/**
 * @brief Initialize Serial for logging
 * @param baud Baud rate (default 115200)
 */
inline void logger_init(unsigned long baud = 115200) {
    Serial.begin(baud);
    while (!Serial && millis() < 3000) {
        // Wait for Serial to be ready, max 3 seconds
        delay(10);
    }
    Serial.println();
    Serial.println(F("====================================="));
    Serial.println(F("    TuoiCay Firmware Logger Ready"));
    Serial.println(F("====================================="));
}

//=============================================================================
// UTILITY MACROS
//=============================================================================

// Print free heap (useful for memory debugging)
#define LOG_HEAP() \
    LOG_DBG(MOD_SYSTEM, "heap", "Free: %u bytes", ESP.getFreeHeap())

// Print uptime in seconds
#define LOG_UPTIME() \
    LOG_INF(MOD_SYSTEM, "uptime", "%lu seconds", millis() / 1000)

#endif // LOGGER_H

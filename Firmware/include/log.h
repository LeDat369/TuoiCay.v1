/* log.h - macro logging don gian (su dung Serial khi DEBUG) */
#ifndef LOG_H
#define LOG_H

#include "config.h"

#include <Arduino.h>
#include <stdio.h>
#include <stdarg.h>

static inline void _log_print_prefix(const char* p) { Serial.print(p); }
static inline void _log_vprintf(const char* fmt, ...) {
    char buf[LOG_BUF_SIZE];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    Serial.println(buf);
}

#ifdef DEBUG
#define LOG_DEBUG(...) do { _log_print_prefix("[DBG] "); _log_vprintf(__VA_ARGS__); } while(0)
#else
#define LOG_DEBUG(...) do {} while(0)
#endif

/* INFO/WARN/ERROR van hoat dong ca khi khong DEBUG de luu thong tin quan trong */
#define LOG_INFO(...)  do { _log_print_prefix("[INF] "); _log_vprintf(__VA_ARGS__); } while(0)
#define LOG_WARN(...)  do { _log_print_prefix("[WRN] "); _log_vprintf(__VA_ARGS__); } while(0)
#define LOG_ERROR(...) do { _log_print_prefix("[ERR] "); _log_vprintf(__VA_ARGS__); } while(0)

#endif /* LOG_H */

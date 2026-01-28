#include "storage.h"
#include <Arduino.h>
#include <LittleFS.h>
#include "log.h"

static const char* k_log_path = "/pump_log.txt";

bool storage_init(void) {
  if (!LittleFS.begin()) {
    LOG_ERROR("LittleFS begin failed");
    return false;
  }
  return true;
}

bool storage_append_pump_event(uint32_t start_ms, uint32_t duration_s, const char* reason) {
  File f = LittleFS.open(k_log_path, "a");
  if (!f) return false;
  // Dong JSON don gian
  char buf[LOG_BUF_SIZE];
  int n = snprintf(buf, sizeof(buf), "{\"start_ms\":%lu,\"duration_s\":%lu,\"reason\":\"%s\"}\n",
                   (unsigned long)start_ms, (unsigned long)duration_s, reason ? reason : "");
  if (n > 0) f.write((const uint8_t*)buf, n);
  f.close();
  return true;
}

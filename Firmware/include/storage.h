/* storage.h - ghi su kien don gian voi LittleFS (dong JSON)
 * Luu su kien bom: start_ms (millis tu khi khoi dong), duration_s, reason
 */
#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool storage_init(void);
bool storage_append_pump_event(uint32_t start_ms, uint32_t duration_s, const char* reason);

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_H */

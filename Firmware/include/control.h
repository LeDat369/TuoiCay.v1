/* control.h - API dieu khien bom va an toan */
#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CONTROL_STATE_IDLE = 0,
    CONTROL_STATE_WATERING,
    CONTROL_STATE_COOLDOWN,
    CONTROL_STATE_ERROR
} control_state_t;

void control_init(void);
void pump_on(void);
void pump_off(void);
void pump_on_for(uint32_t seconds);
bool pump_is_on(void);
void control_update(void); /* goi dinh ky tu main loop */
void control_set_max_duration(uint32_t seconds);

/* Dieu khien che do tu dong */
void control_start_auto(void);
void control_stop_auto(void);
bool control_is_auto_enabled(void);

/* Cau hinh (phan tram 0..100) */
void control_set_thresholds_percent(uint8_t low_percent, uint8_t high_percent);
void control_set_cooldown_seconds(uint32_t seconds);

/* Ham tro giup dieu khien thu cong (khong chan) */
void control_manual_on(uint32_t seconds);

/* Ham truy van trang thai */
control_state_t control_get_state(void);
uint8_t control_get_last_moisture(void);

/* Ep bom bat/tat (ghi de che do tu dong). Khi ep ON, chan bom ON den khi ep OFF */
void control_force_on(void);
void control_force_off(void);
bool control_is_forced(void);

#ifdef __cplusplus
}
#endif

#endif /* CONTROL_H */

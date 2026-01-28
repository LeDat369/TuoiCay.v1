/* hw_io.h - truu tuong hoa phan cung */
#ifndef HW_IO_H
#define HW_IO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void hw_init(void);
void hw_set_pump(bool on);
void hw_set_pwm(uint16_t duty);
int hw_read_adc_raw(void);
float hw_adc_raw_to_voltage(int raw);
float hw_adc_voltage_to_sensor_vin(float v);
void hw_calibrate_adc(float scale);
/* Tra ve duty PWM cuoi cung da viet vao chan bom (0..PWM_MAX_DUTY). */
uint16_t hw_get_pwm_duty(void);

#ifdef __cplusplus
}
#endif

#endif /* HW_IO_H */

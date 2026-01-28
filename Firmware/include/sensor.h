/* sensor.h - truu tuong sensor + loc va debounce cho firmware */
#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int raw_adc;
  float voltage;
  float vin;
  int moisture_percent; /* 0..100 */
  bool sensor1_digital;
  bool sensor2_digital;
} sensor_data_t;

void sensor_init(void);
void sensor_read_all(sensor_data_t* out);
int sensor_get_moisture_percent(uint8_t sensor_id);

/* Return debounced digital state for sensor: 1 or 2. */
bool sensor_get_digital(uint8_t sensor_id);

/* Test helper: override ADC reading (use -1 to disable) */
void sensor_sim_set_adc(int raw);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_H */

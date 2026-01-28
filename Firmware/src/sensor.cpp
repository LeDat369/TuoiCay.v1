#include "Arduino.h"
#include "sensor.h"
#include "hw_io.h"
#include "config.h"

/* Bo loc trung binh dong cho ADC va debounce cho input digital */
static const int k_window_size = FILTER_WINDOW_SIZE;
static int s_buf[FILTER_WINDOW_SIZE];
static int s_idx = 0;
static long s_sum = 0;
static int s_count = 0;

static int s_sim_adc = -1;

/* trang thai debounce cho input digital */
static bool s_sensor1_state = false;
static bool s_sensor2_state = false;
static bool s_sensor1_last = false;
static bool s_sensor2_last = false;
static unsigned long s_sensor1_last_change = 0;
static unsigned long s_sensor2_last_change = 0;
static const unsigned long k_debounce_ms = DEBOUNCE_MS;

static sensor_data_t s_latest;

void sensor_init(void) {
  for (int i = 0; i < k_window_size; ++i) s_buf[i] = 0;
  s_idx = 0; s_sum = 0; s_count = 0;
  s_sim_adc = -1;
  s_sensor1_state = s_sensor2_state = false;
  s_sensor1_last = digitalRead(PIN_SENSOR1_DIGITAL);
  s_sensor2_last = digitalRead(PIN_SENSOR2_DIGITAL);
  s_sensor1_last_change = s_sensor2_last_change = millis();
  memset(&s_latest, 0, sizeof(s_latest));
}

static void feed_adc_sample(int raw) {
  if (s_count < k_window_size) {
    s_buf[s_idx] = raw;
    s_sum += raw;
    s_count++;
    s_idx = (s_idx + 1) % k_window_size;
  } else {
    s_sum -= s_buf[s_idx];
    s_buf[s_idx] = raw;
    s_sum += raw;
    s_idx = (s_idx + 1) % k_window_size;
  }
}

void sensor_read_all(sensor_data_t* out) {
  int raw = (s_sim_adc >= 0) ? s_sim_adc : hw_read_adc_raw();
  feed_adc_sample(raw);
  int avg_raw = (s_count == 0) ? raw : (int)(s_sum / s_count);

  float v = hw_adc_raw_to_voltage(avg_raw);
  float vin = hw_adc_voltage_to_sensor_vin(v);

  /* Map raw -> moisture percent. We assume higher raw means drier; invert. */
  int percent = (int)((1.0f - ((float)avg_raw / (float)ADC_MAX_RAW)) * 100.0f);
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;

  /* Thuc hien debounce cho cac input digital */
  bool s1 = digitalRead(PIN_SENSOR1_DIGITAL);
  if (s1 != s_sensor1_last) {
    s_sensor1_last_change = millis();
    s_sensor1_last = s1;
  } else if ((millis() - s_sensor1_last_change) >= k_debounce_ms) {
    s_sensor1_state = s1;
  }

  bool s2 = digitalRead(PIN_SENSOR2_DIGITAL);
  if (s2 != s_sensor2_last) {
    s_sensor2_last_change = millis();
    s_sensor2_last = s2;
  } else if ((millis() - s_sensor2_last_change) >= k_debounce_ms) {
    s_sensor2_state = s2;
  }

  s_latest.raw_adc = avg_raw;
  s_latest.voltage = v;
  s_latest.vin = vin;
  s_latest.moisture_percent = percent;
  s_latest.sensor1_digital = s_sensor1_state;
  s_latest.sensor2_digital = s_sensor2_state;

  if (out) {
    memcpy(out, &s_latest, sizeof(s_latest));
  }
}

int sensor_get_moisture_percent(uint8_t sensor_id) {
  /* sensor_id 0 -> sensor analog (Sensor2/A0). Cac id khac chua duoc implement */
  (void)sensor_id;
  return s_latest.moisture_percent;
}

void sensor_sim_set_adc(int raw) {
  s_sim_adc = raw;
}

bool sensor_get_digital(uint8_t sensor_id) {
  if (sensor_id == 1) return s_latest.sensor1_digital;
  if (sensor_id == 2) return s_latest.sensor2_digital;
  return false;
}

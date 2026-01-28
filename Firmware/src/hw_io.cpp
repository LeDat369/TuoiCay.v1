#include "Arduino.h"
#include "config.h"
#include "hw_io.h"

static float s_adc_scale = 1.0f;
static uint16_t s_last_pwm_duty = 0;

void hw_init(void) {
  /* Chan dieu khien bom (D6 / GPIO12) */
  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, LOW);

  /* Chan input digital cho sensor */
  pinMode(PIN_SENSOR1_DIGITAL, INPUT);
  pinMode(PIN_SENSOR2_DIGITAL, INPUT);
  /* Cau hinh tan so va pham vi PWM cho ESP8266 Arduino core */
#ifdef ESP8266
  analogWriteFreq(PWM_FREQ_HZ); /* tan so PWM */
  analogWriteRange(PWM_MAX_DUTY);
#endif
}

void hw_set_pump(bool on) {
  digitalWrite(PIN_PUMP, on ? HIGH : LOW);
  /* phan bo gia tri duty tuong ung voi on/off de bao cao trang thai */
  s_last_pwm_duty = on ? PWM_MAX_DUTY : 0;
}

void hw_set_pwm(uint16_t duty) {
  if (duty > PWM_MAX_DUTY) duty = PWM_MAX_DUTY;
  analogWrite(PIN_PUMP, duty);
  s_last_pwm_duty = duty;
}

int hw_read_adc_raw(void) {
  return analogRead(PIN_ADC);
}

float hw_adc_raw_to_voltage(int raw) {
  const float max_raw = (float)ADC_MAX_RAW;
#if ADC_HAS_DIVIDER
  const float vref = 3.3f;
#else
  /* Trên board ESP-12 thuong, A0 doc den ~1.0V neu khong co chia tro ngoai */
  const float vref = 1.0f;
#endif
  float v = (raw / max_raw) * vref;
  v *= s_adc_scale;
  return v;
}

float hw_adc_voltage_to_sensor_vin(float v) {
#if ADC_HAS_DIVIDER
  return v; /* NodeMCU already scales to 0..3.3V */
#else
  float r1 = (float)ADC_DIVIDER_R1;
  float r2 = (float)ADC_DIVIDER_R2;
  return v * ((r1 + r2) / r2);
#endif
}

void hw_calibrate_adc(float scale) {
  if (scale > 0.0f) s_adc_scale = scale;
}

uint16_t hw_get_pwm_duty(void) {
  return s_last_pwm_duty;
}

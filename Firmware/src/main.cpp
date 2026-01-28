#include <Arduino.h>
#include "config.h"
#include "hw_io.h"
#include "sensor.h"
#include "control.h"
#include "storage.h"
#include "log.h"

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(100);
  LOG_INFO("TuoiCay Firmware - Phase 0: setup");

#ifdef BOARD_NODEMCU
  LOG_INFO("Board: NodeMCU (A0 has built-in divider)");
#else
  LOG_INFO("Board: ESP-12 (A0 may need external divider)");
#endif
  hw_init();
  sensor_init();
  if (!storage_init()) {
    LOG_WARN("Warning: storage init failed");
  }

  control_init();

  LOG_INFO("PIN_PUMP=%d", PIN_PUMP);
  LOG_INFO("PIN_SENSOR1_DIGITAL=%d", PIN_SENSOR1_DIGITAL);
  LOG_INFO("PIN_SENSOR2_DIGITAL=%d", PIN_SENSOR2_DIGITAL);
  LOG_INFO("PWM=%u", hw_get_pwm_duty());

  LOG_INFO("Commands: pump on | pump off | pwm <0-1023> | adccal <scale> | adcread");
  LOG_INFO("         sensor read | sensor sim <raw>| sensor sim off");
  LOG_INFO("         pump onfor <s> | pump status | pump setmax <s>");
  LOG_INFO("         auto on | auto off | auto status");

  LOG_INFO("Initial ADC read: %d", hw_read_adc_raw());

  float v = hw_adc_raw_to_voltage(hw_read_adc_raw());
  float vin = hw_adc_voltage_to_sensor_vin(v);
  LOG_INFO("ADC voltage (on-module) = %f", v);
  LOG_INFO("Estimated sensor VIN = %f", vin);
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length()) {
      if (line.equalsIgnoreCase("pump on")) {
        hw_set_pump(true);
        LOG_INFO("OK: pump on");
      } else if (line.equalsIgnoreCase("pump off")) {
        hw_set_pump(false);
        LOG_INFO("OK: pump off");
      } else if (line.startsWith("pwm ")) {
        int v = line.substring(4).toInt();
        hw_set_pwm(v);
        LOG_INFO("OK: pwm %d", v);
      } else if (line.equalsIgnoreCase("pump test")) {
#ifdef DEBUG
        LOG_INFO("Pump test: toggling digital output 5 times");
        for (int i = 0; i < 5; ++i) {
          hw_set_pump(true);
          LOG_INFO("pump HIGH");
          delay(500);
          hw_set_pump(false);
          LOG_INFO("pump LOW");
          delay(500);
        }
        LOG_INFO("Pump test done");
#else
        LOG_WARN("pump test disabled in non-DEBUG build");
#endif
      } else if (line.equalsIgnoreCase("pin check")) {
#ifdef DEBUG
        LOG_INFO("PIN CHECK: set D6 HIGH briefly and read back");
        hw_set_pump(true);
        delay(200);
        int v = digitalRead(PIN_PUMP);
        LOG_INFO("digitalRead(PIN_PUMP)=%d", v);
        hw_set_pump(false);
        LOG_INFO("PIN CHECK done");
#else
        LOG_WARN("pin check disabled in non-DEBUG build");
#endif
      } else if (line.equalsIgnoreCase("force on")) {
        control_force_on();
      } else if (line.equalsIgnoreCase("force off")) {
        control_force_off();
      } else if (line.equalsIgnoreCase("pwm test")) {
#ifdef DEBUG
        LOG_INFO("PWM test: ramping duty 0..1023");
        for (int d = 0; d <= 1023; d += 128) {
          hw_set_pwm(d);
          LOG_INFO("pwm set %d", d);
          delay(300);
        }
        hw_set_pwm(0);
        LOG_INFO("PWM test done");
#else
        LOG_WARN("pwm test disabled in non-DEBUG build");
#endif
      } else if (line.startsWith("pump onfor ")) {
        int s = line.substring(11).toInt();
        pump_on_for((uint32_t)s);
        LOG_INFO("OK: pump onfor %d", s);
      } else if (line.equalsIgnoreCase("pump status")) {
        LOG_INFO("pump_is_on=%d", pump_is_on());
      } else if (line.startsWith("pump setmax ")) {
        int s = line.substring(12).toInt();
        control_set_max_duration((uint32_t)s);
        LOG_INFO("OK: pump setmax %d", s);
      } else if (line.equalsIgnoreCase("auto on")) {
        control_start_auto();
        LOG_INFO("OK: auto on");
      } else if (line.equalsIgnoreCase("auto off")) {
        control_stop_auto();
        LOG_INFO("OK: auto off");
      } else if (line.equalsIgnoreCase("auto status")) {
        LOG_INFO("auto=%d", control_is_auto_enabled());
      } else if (line.startsWith("adccal ")) {
        float s = line.substring(7).toFloat();
        hw_calibrate_adc(s);
        LOG_INFO("OK: adccal %f", s);
      } else if (line.equalsIgnoreCase("sensor read")) {
        sensor_data_t d;
        sensor_read_all(&d);
        LOG_INFO("SENSOR RAW=%d V=%f VIN=%f %%=%d D1=%d D2=%d", d.raw_adc, d.voltage, d.vin, d.moisture_percent, d.sensor1_digital, d.sensor2_digital);
      } else if (line.startsWith("sensor sim ")) {
        String arg = line.substring(11);
        arg.trim();
        if (arg.equalsIgnoreCase("off")) {
          sensor_sim_set_adc(-1);
          LOG_INFO("OK: sensor sim off");
        } else {
          int v = arg.toInt();
          sensor_sim_set_adc(v);
          LOG_INFO("OK: sensor sim %d", v);
        }
      } else if (line.equalsIgnoreCase("adcread")) {
        int raw = hw_read_adc_raw();
        float v = hw_adc_raw_to_voltage(raw);
        float vin = hw_adc_voltage_to_sensor_vin(v);
        LOG_INFO("RAW=%d V=%f VIN=%f", raw, v, vin);
      } else {
        LOG_WARN("Unknown command");
      }
    }
  }
  control_update();
  static unsigned long last_sensor_ms = 0;
  unsigned long now = millis();

  /* phat hien thay doi PWM: log ngay khi duty thay doi */
  static int _last_pwm_duty = -1;
  int _cur_pwm = (int)hw_get_pwm_duty();
  if (_cur_pwm != _last_pwm_duty) {
    _last_pwm_duty = _cur_pwm;
    LOG_INFO("PWM=%u", (unsigned)_cur_pwm);
  }

  /* doc chan digital sensor moi 1s */
  if ((now - last_sensor_ms) >= 1000UL) {
    last_sensor_ms = now;
    sensor_data_t d;
    sensor_read_all(&d);
    int dig1 = digitalRead(PIN_SENSOR1_DIGITAL);
    int dig2 = digitalRead(PIN_SENSOR2_DIGITAL);
    LOG_DEBUG("[SENSOR] RAW=%d V=%f VIN=%f %%=%d D1=%d D2=%d", d.raw_adc, d.voltage, d.vin, d.moisture_percent, d.sensor1_digital, d.sensor2_digital);
    LOG_INFO("DIGITALS: PIN_SENSOR1=%d PIN_SENSOR2=%d PUMP_PIN=%d", dig1, dig2, digitalRead(PIN_PUMP));
    /* canh bao neu hai chan digital khong giong nhau (log lien tuc neu khac nhau) */
    bool _cur_mismatch = (dig1 != dig2);
    if (_cur_mismatch) {
      LOG_WARN("SENSOR_MISMATCH: PIN_SENSOR1=%d PIN_SENSOR2=%d", dig1, dig2);
    }
    /* in trang thai bom / PWM o muc debug */
    LOG_DEBUG("PUMP_is_on=%d PWM_DUTY=%u", pump_is_on(), hw_get_pwm_duty());
  }

  yield();
}

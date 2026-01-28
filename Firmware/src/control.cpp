#include "Arduino.h"
#include "control.h"
#include "hw_io.h"
#include "storage.h"
#include "sensor.h"
#include "log.h"
#include "sync.h"

/* Trien khai may trang thai cho dieu khien tu dong
 * Trang thai: IDLE -> WATERING -> COOLDOWN -> IDLE
 * Trang thai ERROR duoc de trong cho su dung sau
 * */

static bool s_pump_on = false;
static unsigned long s_on_until_ms = 0; /* 0 = khong co hen tat (manual) */
static unsigned long s_started_at = 0;
static uint32_t s_max_duration_s = DEFAULT_MAX_DURATION_S; /* default safety max */
static bool s_auto_enabled = false;
static const char* s_last_reason = "manual";

static control_state_t s_state = CONTROL_STATE_IDLE;
static uint8_t s_thresh_low_pct = DEFAULT_THRESH_LOW_PCT;   /* default: start watering below */
static uint8_t s_thresh_high_pct = DEFAULT_THRESH_HIGH_PCT;  /* stop watering at or above */
static uint32_t s_cooldown_s = DEFAULT_COOLDOWN_S;      /* cooldown after watering */
static unsigned long s_cooldown_until_ms = 0;
static bool s_manual_override = false;
static uint8_t s_last_moisture = 0;
static bool s_pwm_active = false;
static bool s_forced = false;

void control_init(void) {
  s_pump_on = false;
  s_on_until_ms = 0;
  s_started_at = 0;
  s_state = CONTROL_STATE_IDLE;
  s_auto_enabled = true; /* che do tu dong mac dinh */
  s_manual_override = false;
  hw_set_pump(false);
  /* mac dinh khong ep bom bat */
  s_forced = false;
  s_pump_on = false;
  LOG_INFO("control: initialized (auto ON by default)");
}

void control_force_on(void) {
  sync_lock();
  s_forced = true;
  hw_set_pump(true);
  s_pump_on = true;
  sync_unlock();
  LOG_INFO("control: FORCE ON (D6 HIGH)");
}

void control_force_off(void) {
  sync_lock();
  s_forced = false;
  hw_set_pump(false);
  s_pump_on = false;
  sync_unlock();
  LOG_INFO("control: FORCE OFF (D6 LOW)");
}

bool control_is_forced(void) {
  return s_forced;
}

void pump_on(void) {
  if (!s_pump_on) {
    hw_set_pump(true);
    s_pump_on = true;
    s_started_at = millis();
    if (s_on_until_ms == 0) s_on_until_ms = 0; /* ro rang: khong co hen tat */
    s_last_reason = "manual";
  }
}

void pump_off(void) {
  if (s_pump_on) {
    if (s_pwm_active) {
      hw_set_pwm(0);
      LOG_DEBUG("[PWM STOP] duty=0");
      s_pwm_active = false;
    }
    hw_set_pump(false);
    s_pump_on = false;
    s_on_until_ms = 0;
    unsigned long now = millis();
    unsigned long elapsed_ms = now - s_started_at;
    uint32_t elapsed_s = (uint32_t)(elapsed_ms / 1000UL);
    storage_append_pump_event((uint32_t)s_started_at, elapsed_s, s_last_reason);
    s_started_at = 0;
  }
}

void pump_on_for(uint32_t seconds) {
  if (seconds == 0) {
    pump_on();
    return;
  }
  /* gioi han khong vuot qua thoi gian toi da an toan */
  if (seconds > s_max_duration_s) seconds = s_max_duration_s;
  hw_set_pump(true);
  s_pump_on = true;
  s_started_at = millis();
  s_on_until_ms = s_started_at + (unsigned long)seconds * 1000UL;
  s_last_reason = "manual";
}

bool pump_is_on(void) {
  return s_pump_on;
}

void control_set_max_duration(uint32_t seconds) {
  if (seconds == 0) return;
  s_max_duration_s = seconds;
}

void control_set_thresholds_percent(uint8_t low_percent, uint8_t high_percent) {
  if (low_percent > high_percent) return;
  s_thresh_low_pct = low_percent;
  s_thresh_high_pct = high_percent;
}

void control_set_cooldown_seconds(uint32_t seconds) {
  s_cooldown_s = seconds;
}

void control_manual_on(uint32_t seconds) {
  /* Manual override: start watering for given seconds, override auto while active */
  s_manual_override = true;
  pump_on_for(seconds);
  s_state = CONTROL_STATE_WATERING;
  s_last_reason = "manual";
}

control_state_t control_get_state(void) {
  return s_state;
}

uint8_t control_get_last_moisture(void) {
  return s_last_moisture;
}

void control_update(void) {
  unsigned long now = millis();

  /* Doc sensor o dau vong cap nhat de dong bo PWM va cac quyet dinh.
   * Dam bao PWM su dung gia tri ADC/digital moi nhat moi vong.
   */
  sensor_data_t sd;
  sensor_read_all(&sd);
  s_last_moisture = (uint8_t)sd.moisture_percent;

  /* Chuyen trang thai */
  switch (s_state) {
    case CONTROL_STATE_IDLE: {
      if (s_auto_enabled && !s_manual_override) {
        /* kiem tra sensor digital: ca hai phai la 1 de cho phep tuoi bang PWM */
        bool d1 = sd.sensor1_digital;
        bool d2 = sd.sensor2_digital;
        if (d1 && d2) {
          /* bat tuoi bang PWM; duty ti le voi gia tri ADC moi nhat */
          int raw = sd.raw_adc;
          if (raw < 0) raw = 0;
          if (raw > ADC_MAX_RAW) raw = ADC_MAX_RAW;
          uint16_t duty = (uint16_t)raw; /* 0..ADC_MAX_RAW */
          hw_set_pwm(duty);
          LOG_DEBUG("[PWM START] D6 duty=%u", duty);
          s_pwm_active = true;
          s_pump_on = true;
          s_started_at = now;
          s_on_until_ms = 0;
          s_last_reason = "auto";
          s_state = CONTROL_STATE_WATERING;
        }
      }
      break;
    }
    case CONTROL_STATE_WATERING: {
      /* Neu sensor digital khong deu la 1, dung tuoi ngay va tro ve IDLE
       * (khong vao cooldown) de PWM co the bat lai khi sensor tro lai 1
       */
      {
        bool d1 = sensor_get_digital(1);
        bool d2 = sensor_get_digital(2);
        if (!(d1 && d2) && !s_manual_override && !s_forced) {
          /* stop without cooldown so a later re-assertion of sensors restarts watering */
          pump_off();
          s_state = CONTROL_STATE_IDLE;
          break;
        }
      }
      /* Neu PWM dang hoat dong, cap nhat duty lien tuc theo ADC */
      if (s_pwm_active) {
        int raw = sd.raw_adc;
        if (raw < 0) raw = 0;
        if (raw > ADC_MAX_RAW) raw = ADC_MAX_RAW;
        uint16_t duty = (uint16_t)raw;
        hw_set_pwm(duty);
        LOG_DEBUG("[PWM UPDATE] duty=%u", duty);
      } else {
        if (!s_pump_on) {
          hw_set_pump(true);
          s_pump_on = true;
          s_started_at = now;
        }
      }

      /* kiem tra dieu kien dung: muc do analog hoac het thoi gian */
      if (s_last_moisture >= s_thresh_high_pct) {
        pump_off();
        s_state = CONTROL_STATE_COOLDOWN;
        s_cooldown_until_ms = now + (unsigned long)s_cooldown_s * 1000UL;
        s_manual_override = false;
        break;
      }

      /* hen tat da dat (manual on_for) */
      if (s_on_until_ms != 0) {
        if ((long)(now - s_on_until_ms) >= 0) {
          pump_off();
          s_state = CONTROL_STATE_COOLDOWN;
          s_cooldown_until_ms = now + (unsigned long)s_cooldown_s * 1000UL;
          s_manual_override = false;
          break;
        }
      }

      /* kiem tra va thuc thi gioi han thoi gian toi da an toan */
      if (s_max_duration_s != 0) {
        unsigned long elapsed = now - s_started_at;
        if (elapsed >= (unsigned long)s_max_duration_s * 1000UL) {
          pump_off();
          s_state = CONTROL_STATE_COOLDOWN;
          s_cooldown_until_ms = now + (unsigned long)s_cooldown_s * 1000UL;
          s_manual_override = false;
          break;
        }
      }
      break;
    }
    case CONTROL_STATE_COOLDOWN: {
      /* o trang thai cooldown den khi het thoi gian, sau do tro ve IDLE */
      if ((long)(now - s_cooldown_until_ms) >= 0) {
        s_state = CONTROL_STATE_IDLE;
      }
      break;
    }
    case CONTROL_STATE_ERROR: {
      /* reserved: could implement retries or safe shutdown */
      break;
    }
  }
}

void control_start_auto(void) {
  s_auto_enabled = true;
}

void control_stop_auto(void) {
  s_auto_enabled = false;
}

bool control_is_auto_enabled(void) {
  return s_auto_enabled;
}

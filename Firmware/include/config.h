/* config.h - Map chan va tuy chinh build cho firmware TuoiCay Phase 0 */
#ifndef CONFIG_H
#define CONFIG_H

/* Board selection:
 * Uncomment if using NodeMCU (which provides a built-in ADC divider for A0).
 * If not defined, code assumes a raw ESP-12 style board (external divider may be required).
 */
// #define BOARD_NODEMCU

/* Pin assignments (use GPIO numbers). Adjust if your board mapping differs. */
#define PIN_PUMP 12                /* D6 (GPIO12) - MOSFET gate */
#define PIN_SENSOR1_DIGITAL 14     /* D5 (GPIO14) - Sensor1 digital out */
#define PIN_SENSOR2_DIGITAL 5      /* D1 (GPIO5)  - Sensor2 digital out */
#define PIN_ADC A0                 /* ADC pin (A0) */

/* ADC configuration */
#ifdef BOARD_NODEMCU
#define ADC_HAS_DIVIDER 1          /* NodeMCU's module-level divider allows 0-3.3V on A0 */
#else
#define ADC_HAS_DIVIDER 0          /* Raw boards may need an external divider */
/* If using an external divider, assumed example: R1=24k, R2=10k (see Docs/Pinout.md) */
#define ADC_DIVIDER_R1 24000
#define ADC_DIVIDER_R2 10000
#endif

/* Serial settings */
#define SERIAL_BAUD 57600

/* Common magic numbers consolidated */
/* Maximum ADC raw value (10-bit ADC) */
#define ADC_MAX_RAW 1023

/* PWM configuration */
#define PWM_MAX_DUTY 1023
#define PWM_FREQ_HZ 1000

/* Control defaults */
#define DEFAULT_MAX_DURATION_S 300  /* safety max pump on duration */
#define DEFAULT_COOLDOWN_S 60       /* cooldown after watering (s) */

/* Sensor/filter settings */
#define FILTER_WINDOW_SIZE 8
#define DEBOUNCE_MS 50

/* Logging buffer sizes */
#define LOG_BUF_SIZE 192
#define LOG_OUT_SIZE 256

/* Default thresholds for watering (percent) */
#ifndef DEFAULT_THRESH_LOW_PCT
#define DEFAULT_THRESH_LOW_PCT 30
#endif
#ifndef DEFAULT_THRESH_HIGH_PCT
#define DEFAULT_THRESH_HIGH_PCT 60
#endif

#endif /* CONFIG_H */

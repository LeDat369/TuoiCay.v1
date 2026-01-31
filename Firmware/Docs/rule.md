# ESP32 FIRMWARE DEVELOPMENT RULES v3.0
================================================================================
MỤC ĐÍCH: Quy tắc phát triển firmware ESP32 thương mại
ĐỐI TƯỢNG: AI Assistant + Developer
CẬP NHẬT: 2026-01-30
================================================================================

## QUICK REFERENCE (TRA CỨU NHANH)

### Error Code Ranges:
```
0=OK | 1xxx=WiFi | 2xxx=MQTT | 3xxx=Sensor | 4xxx=Storage | 5xxx=OTA | 6xxx=Hardware | 7xxx=Comm | 8xxx=Logic | 9xxx=System
```

### Naming Convention:
```
Class=PascalCase | Method=camelCase | Constant=UPPER_SNAKE | Private=trailing_ | File=snake_case
```

### Critical Timeouts:
```
Watchdog=5-30s | WiFi_Connect=30s | MQTT_Connect=10s | I2C=100ms | Flash_Debounce=5-10s
```

### Memory Thresholds:
```
Heap_Warning<30KB | Stack_Min>500bytes | PSRAM_for>10KB | No_malloc_in_loop
```

### Common Code Patterns:
```cpp
// === ERROR HANDLING ===
if (result != OK) { LOG_ERR("MOD","func","err=%d",result); return result; }

// === MUTEX ===
if (xSemaphoreTake(mtx, pdMS_TO_TICKS(1000)) == pdTRUE) { /*code*/ xSemaphoreGive(mtx); }

// === NON-BLOCKING DELAY ===
vTaskDelay(pdMS_TO_TICKS(100));

// === SAFE NVS READ ===
if (nvs_get_blob(h,k,&data,&len) != ESP_OK) data = DEFAULT;

// === JSON PARSE ===
StaticJsonDocument<256> doc; if (deserializeJson(doc,payload)) return ERR;

// === WIFI RECONNECT ===
WiFi.begin(ssid, pass, savedChannel, savedBssid, true);  // Fast reconnect

// === DEEP SLEEP ===
esp_sleep_enable_timer_wakeup(us); esp_deep_sleep_start();
```

### Section Index (Ctrl+F):
```
#CORE=1 #SAFETY=2 #MEMORY=3 #THREAD=4 #SECURITY=5 #ERROR=6 #LOG=7 #WIFI=8 #MQTT=9 #OTA=10
#GPIO=11 #TIME=12 #SENSOR=13 #PROTOCOL=14 #ACTUATOR=15 #LED=16 #POWER=17 #NVS=18
#TEST=19 #AIDEV=20 #OPTIMIZE=21 #DIAGNOSTIC=22 #JSON=23 #HTTP=24 #FS=25 #BLE=26 #BUTTON=27
#EXPERT=28 #FIELD=29 #TROUBLESHOOT=30
```

================================================================================
# PHẦN 1: NGUYÊN TẮC CƠ BẢN
================================================================================
TAGS: #CORE #PRINCIPLES #SOLID #OOP #STRUCTURE #NAMING
WHEN: Bắt đầu project mới | Review code | Refactor
SEE ALSO: #AIDEV(20) for code organization

## 1.1 CORE PRINCIPLES
| Principle | Rule | Violation Example |
|-----------|------|-------------------|
| KISS | Simple, readable code | Complex templates unnecessarily |
| YAGNI | Implement only when needed | Pre-building unused features |
| DRY | No code duplication | Copy-paste same logic |
| OOP | Object-oriented required | Procedural spaghetti code |

**FORBIDDEN:** Markdown in code, Emoji in commits

## 1.2 PROJECT STRUCTURE (PlatformIO Standard)
```
project/
├── include/              # Project-wide headers
│   ├── config.h          # Main configuration (timeouts, sizes, versions)
│   ├── pins.h            # GPIO pin definitions
│   ├── secrets.h         # Credentials (WiFi, MQTT) - KHÔNG COMMIT!
│   ├── device_state.h    # Device state model
│   └── sensor_data.h     # Sensor data model
│
├── lib/                  # Reusable libraries (có library.json)
│   ├── ESP32_Drivers/    # Hardware drivers
│   │   └── src/
│   │       ├── gpio/     # button_driver, led_driver, pwm_led_driver
│   │       ├── sensors/  # temp_driver, hall_driver, touch_driver
│   │       ├── storage/  # nvs_storage, file_system
│   │       ├── wifi/     # wifi_driver, wifi_ap, wifi_scanner
│   │       └── comm/     # ble_server, http_client, web_server
│   │
│   ├── ESP32_Managers/   # High-level managers
│   │   └── src/          # mqtt_manager, ota_manager, task_manager
│   │                     # time_manager, sleep_manager
│   │
│   └── ESP32_Utils/      # Utilities
│       └── src/          # logger.h, crc_utils.h, string_utils.h
│
├── src/                  # Application code
│   └── main.cpp          # Entry point only (setup/loop)
│
├── test/                 # Unit tests
├── data/                 # SPIFFS/LittleFS files
└── platformio.ini        # Build configuration
```

**Library Include Style:**
```cpp
// ✅ CORRECT - Use angle brackets for library headers
#include <config.h>      // From include/
#include <logger.h>      // From lib/ESP32_Utils/
#include <wifi_driver.h> // From lib/ESP32_Drivers/
#include <mqtt_manager.h>// From lib/ESP32_Managers/

// ❌ WRONG - Avoid relative paths
#include "../../utils/logger.h"
#include "../drivers/wifi_driver.h"
```

## 1.3 SOLID FOR ESP32
| S | Single Responsibility | WiFiManager only handles WiFi |
| O | Open/Closed | Add sensor without modifying SensorManager |
| L | Liskov Substitution | All ISensor have read() |
| I | Interface Segregation | Small interfaces: IConnectable, IConfigurable |
| D | Dependency Inversion | Use ISensor*, not DHT22Sensor* |

================================================================================
# PHẦN 2: RELIABILITY & SAFETY
================================================================================
TAGS: #SAFETY #WATCHDOG #SAFEMODE #BROWNOUT #STATEMACHINE
WHEN: Device crash | Reboot loop | Power issue | Unstable behavior
SEE ALSO: #POWER(17) #ERROR(6) #ACTUATOR(15)

## 2.1 WATCHDOG
```
CONFIG: esp_task_wdt_init(timeout_sec, true)
FEED:   esp_task_wdt_reset() in each main task
TIMEOUT: 5-30 seconds depending on application
```

## 2.2 NON-BLOCKING RULES
| FORBIDDEN | USE INSTEAD |
|-----------|-------------|
| delay(ms) | vTaskDelay(pdMS_TO_TICKS(ms)) |
| while(waiting) | Event callbacks |
| Blocking I/O | Async + timeout |

## 2.3 SAFE MODE
```
ENTER WHEN: Boot>3x in 5min | Watchdog reset | Self-test fail
BEHAVIOR:   WiFi AP ON | OTA allowed | Actuators OFF | Special LED
```

## 2.4 STATE MACHINE RULES
- Use enum for all states
- Log every state transition
- Every state MUST have timeout
- NO scattered flag variables

## 2.5 BROWNOUT HANDLING
```cpp
// Check reset reason at boot
esp_reset_reason_t reason = esp_reset_reason();
if (reason == ESP_RST_BROWNOUT) {
    LOG_WRN("SYSTEM", "boot", "Brownout reset detected");
    // Reduce power: lower CPU freq, disable unused peripherals
    // Alert user: low voltage warning
    // Save critical state before potential next brownout
}
```
| Prevention | Method |
|------------|--------|
| Capacitor | 100-1000μF on VCC |
| CPU freq | Reduce to 80MHz under low voltage |
| Peripherals | Disable WiFi/BT if battery critical |
| Detection | ADC monitor VCC, warn at 3.0V |

================================================================================
# PHẦN 3: MEMORY MANAGEMENT
================================================================================
TAGS: #MEMORY #HEAP #STACK #PSRAM #FLASH #ALLOCATION #LEAK
WHEN: Out of memory | Heap decreasing | Need large buffer | Flash wear
SEE ALSO: #NVS(18) #FS(25) #OPTIMIZE(21)

## 3.1 ALLOCATION RULES
| Situation | Method |
|-----------|--------|
| Fixed buffer | Static or once at setup |
| Large >10KB | PSRAM: heap_caps_malloc(size, MALLOC_CAP_SPIRAM) |
| Fixed-size data | Memory Pool pattern |
| In loop() | NEVER malloc/new |

## 3.2 FLASH WRITE
```
DEBOUNCE: 5-10s minimum between writes
CONDITION: Only when value actually changed
BATCH: Combine multiple changes into one write
```

## 3.3 MONITORING THRESHOLDS
| Metric | Warning | Action |
|--------|---------|--------|
| Free Heap | <30KB | Reduce buffers |
| Heap decreasing | >1KB/min | Memory leak investigation |
| Stack High Water | <500 bytes | Increase stack size |

================================================================================
# PHẦN 4: CONCURRENCY
================================================================================
TAGS: #THREAD #MUTEX #ISR #INTERRUPT #QUEUE #DEADLOCK #RTOS #FREERTOS
WHEN: Multi-task | Shared variable | ISR handler | Race condition | Deadlock
SEE ALSO: #BUTTON(27) for ISR example

## 4.1 THREAD SAFETY
| Mechanism | When | API |
|-----------|------|-----|
| Mutex | Shared resource | xSemaphoreCreateMutex() |
| Critical | <10μs code | taskENTER_CRITICAL() |
| Queue | Inter-task data | xQueueSend() |
| Event Group | Multi-event sync | xEventGroupSetBits() |

## 4.2 ISR RULES (FORBIDDEN IN ISR)
```
malloc/free | printf/Serial | delay/vTaskDelay | Non-FromISR functions
```

## 4.3 DEADLOCK PREVENTION
```cpp
// Fixed order: mutex_wifi -> mutex_mqtt -> mutex_sensor
// Always timeout, never portMAX_DELAY in production
if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    // critical section
    xSemaphoreGive(mutex);
} else {
    LOG_ERR("SYSTEM", "mutex", "Timeout");
}
```

================================================================================
# PHẦN 5: SECURITY
================================================================================
TAGS: #SECURITY #TLS #SSL #ENCRYPTION #NVS #CERTIFICATE #AUTH
WHEN: Store credentials | Network communication | Production build | Tamper protection
SEE ALSO: #MQTT(9) #HTTP(24) #BLE(26) #OTA(10)

## 5.1 DATA PROTECTION
| Data | Protection |
|------|------------|
| WiFi password | NVS Encryption |
| API Key | NVS Encryption |
| Certificate | Secure partition |
| Token | RAM only |

## 5.2 COMMUNICATION
```
REQUIRED: TLS/SSL all connections | Certificate verify | MQTT:8883 | HTTPS for OTA
RECOMMENDED: Secure Boot | Flash Encryption | eFuse key storage
```

## 5.3 ANTI-TAMPER
| Threat | Countermeasure |
|--------|----------------|
| Command spam | Rate limit: 10 cmd/min |
| Invalid payload | Validate + sanitize |
| Reset loop | Boot counter + Safe Mode |
| Debug access | Disable JTAG/UART in production |

================================================================================
# PHẦN 6: ERROR HANDLING
================================================================================
TAGS: #ERROR #ERRORCODE #EXCEPTION #FAULT #RETURN #PROPAGATE
WHEN: Function returns error | Debug issue | Create new error | Report to AI
SEE ALSO: #LOG(7) #AIDEV(20) #DIAGNOSTIC(22)

## 6.1 ERROR CODE TABLE
| Range | Module | Examples |
|-------|--------|----------|
| 0 | OK | Success |
| 1xxx | WiFi | 1001=ConnectFail, 1002=Timeout, 1003=WrongPass, 1004=APNotFound, 1005=DHCPFail |
| 2xxx | MQTT | 2001=ConnectFail, 2002=PublishFail, 2003=SubscribeFail, 2004=TLSFail, 2005=AuthFail |
| 3xxx | Sensor | 3001=NotFound, 3002=ReadFail, 3003=OutOfRange, 3004=CRCError, 3005=Timeout |
| 4xxx | Storage | 4001=NVSInitFail, 4002=ReadFail, 4003=WriteFail, 4004=Full, 4005=Corrupted |
| 5xxx | OTA | 5001=DownloadFail, 5002=VerifyFail, 5003=FlashFail, 5004=Rollback, 5005=WrongFW |
| 6xxx | Hardware | 6001=GPIO, 6002=I2C, 6003=SPI, 6004=ADC, 6005=PWM |
| 7xxx | Comm | 7001=UART, 7002=RS485, 7003=CAN, 7004=BLE |
| 8xxx | Logic | 8001=InvalidCmd, 8002=PermissionDenied, 8003=RateLimited, 8004=InvalidState |
| 9xxx | System | 9001=OOM, 9002=WDTReset, 9003=Brownout, 9004=StackOverflow, 9005=AssertFail |

## 6.2 ERROR HANDLING PATTERN
```cpp
int result = some_function();
if (result != OK) {
    LOG_ERR("MODULE", "func", "Failed, error=%d", result);
    return result;  // Propagate error
}
```

================================================================================
# PHẦN 7: LOGGING
================================================================================
TAGS: #LOG #DEBUG #TRACE #SERIAL #PRINT #MONITOR
WHEN: Debug | Monitor behavior | Production logging | Report error
SEE ALSO: #ERROR(6) #AIDEV(20) #DIAGNOSTIC(22)

## 7.1 LOG FORMAT
```
[LEVEL][MODULE][FUNCTION] Message with context
Example: [ERR][WIFI][connect] Failed, error=1002, ssid=MyNetwork, rssi=-85
```

## 7.2 LOG LEVELS
| Level | Tag | When | Production |
|-------|-----|------|------------|
| Error | [ERR] | Critical, needs action | ON |
| Warning | [WRN] | Potential issue | ON |
| Info | [INF] | Important events | ON |
| Debug | [DBG] | Debug details | OFF |
| Trace | [TRC] | Step-by-step | OFF |

## 7.3 LOG MACROS
```cpp
#define LOG_ERR(m,f,fmt,...) log_printf("[ERR][%s][%s] " fmt "\n",m,f,##__VA_ARGS__)
#define LOG_WRN(m,f,fmt,...) log_printf("[WRN][%s][%s] " fmt "\n",m,f,##__VA_ARGS__)
#define LOG_INF(m,f,fmt,...) log_printf("[INF][%s][%s] " fmt "\n",m,f,##__VA_ARGS__)
#define LOG_DBG(m,f,fmt,...) log_printf("[DBG][%s][%s] " fmt "\n",m,f,##__VA_ARGS__)
```

================================================================================
# PHẦN 8: WIFI MANAGEMENT
================================================================================
TAGS: #WIFI #CONNECT #RECONNECT #PROVISION #SOFTAP #BACKOFF #RSSI
WHEN: WiFi setup | Connection lost | Weak signal | Provisioning | Fast reconnect
SEE ALSO: #BLE(26) for BLE provisioning | #SECURITY(5) | #TIME(12)

## 8.1 PROVISIONING
```
PRIORITY: 1.BLE Provisioning | 2.SoftAP+Web | 3.SmartConfig(NOT recommended)
TIMEOUT: 5-10 minutes then auto-disable
STORAGE: NVS (NOT EEPROM) | Trim whitespace | Store 2+ networks
```

## 8.2 CONNECTION STATE MACHINE
```
IDLE -> CONNECTING -> WAITING_IP -> CONNECTED
          |              |              |
          v              v              v
       ERROR <-------- ERROR      DISCONNECTED -> [backoff retry]
```

## 8.3 EXPONENTIAL BACKOFF
```
Formula: delay = min(base * 2^attempt, max_delay) + random_jitter(0-20%)
Example: base=2s, max=300s -> 2,4,8,16,32,64,128,256,300,300...
Reset: After stable connection >60s
Max attempts: 20 -> Fallback to SoftAP
```

## 8.4 FAST RECONNECT
```cpp
// Save on successful connect
WiFiFastConnect_t fc = {WiFi.BSSID(), WiFi.channel(), now()};
saveToNVS(&fc);

// Use on reconnect (saves 1-2s scan time)
WiFi.begin(ssid, pass, fc.channel, fc.bssid, true);
```

================================================================================
# PHẦN 9: MQTT MANAGEMENT
================================================================================
TAGS: #MQTT #PUBLISH #SUBSCRIBE #QOS #LWT #TOPIC #BROKER #OFFLINE
WHEN: Send data | Receive command | Connection lost | Message queue | Last Will
SEE ALSO: #WIFI(8) #JSON(23) #SECURITY(5)

## 9.1 TOPIC STRUCTURE
```
{project}/{device_id}/{direction}/{type}
Examples:
  smartfarm/AABBCCDDEEFF/up/telemetry   <- Send sensor data
  smartfarm/AABBCCDDEEFF/down/command   <- Receive commands
  smartfarm/global/down/broadcast       <- System-wide commands
```

## 9.2 QOS SELECTION
| Message Type | QoS | Reason |
|--------------|-----|--------|
| Telemetry periodic | 0 | Loss acceptable |
| Status Online/Offline | 1 | Must deliver |
| Control commands | 1 | Critical |
| Error reports | 1 | Must not lose |
| QoS 2 | NEVER | Too much overhead |

## 9.3 LWT (LAST WILL)
```cpp
// Before connect
client.setWill("project/DEVICE/up/status", "{\"online\":false}", true, 1);
// After connect
client.publish("project/DEVICE/up/status", "{\"online\":true}", true, 1);
```

## 9.4 OFFLINE QUEUE
```
On disconnect: Save important messages to queue (RAM/SPIFFS)
Limit: 50 messages or 100KB, FIFO when full
On reconnect: Send queued messages with message_id for deduplication
```

================================================================================
# PHẦN 10: OTA MANAGEMENT
================================================================================
TAGS: #OTA #UPDATE #FIRMWARE #ROLLBACK #PARTITION #VERSION #UPGRADE
WHEN: Remote update | Version check | Rollback needed | Partition setup
SEE ALSO: #SECURITY(5) #HTTP(24) #SAFETY(2)

## 10.1 PARTITION SCHEME
```
nvs(0x9000,0x5000) | otadata(0xe000,0x2000) | ota_0(0x10000,0x1E0000) | ota_1(0x1F0000,0x1E0000) | spiffs(0x3D0000,0x30000)
```

## 10.2 ROLLBACK MECHANISM
```
1. Download & Flash to ota_1
2. Reboot to ota_1
3. Trial boot: Self-test 30-60s (WiFi, MQTT, Sensors OK?)
4a. OK: esp_ota_mark_app_valid_cancel_rollback()
4b. FAIL: Watchdog reset -> Auto rollback to ota_0
```

## 10.3 VERSION FORMAT
```cpp
#define FW_VERSION "1.2.3"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__
#define GIT_COMMIT "abc1234"

// Log at boot
[INF][SYSTEM][boot] FW:v1.2.3, Built:Jan 30 2026, Heap:280KB
```

## 10.4 VERSION COMPARE
```cpp
// Semantic versioning: MAJOR.MINOR.PATCH
typedef struct { uint8_t major, minor, patch; } Version_t;

int compareVersion(const char* v1, const char* v2) {
    Version_t a, b;
    sscanf(v1, "%hhu.%hhu.%hhu", &a.major, &a.minor, &a.patch);
    sscanf(v2, "%hhu.%hhu.%hhu", &b.major, &b.minor, &b.patch);
    if (a.major != b.major) return a.major - b.major;
    if (a.minor != b.minor) return a.minor - b.minor;
    return a.patch - b.patch;
}

// OTA decision
if (compareVersion(newVer, FW_VERSION) > 0) doOTA();      // Upgrade
else if (forceDowngrade && compareVersion(newVer, FW_VERSION) < 0) doOTA(); // Downgrade
else skipOTA();  // Same version
```

================================================================================
# PHẦN 11: HARDWARE & GPIO
================================================================================
TAGS: #GPIO #PIN #HARDWARE #INIT #SAFESTATE #PERIPHERAL
WHEN: Pin setup | Init order | Safe state | Hardware config
SEE ALSO: #SENSOR(13) #ACTUATOR(15) #PROTOCOL(14)

## 11.1 PIN DEFINITIONS (config.h)
```cpp
#define PIN_RELAY_1    GPIO_NUM_25
#define PIN_SENSOR_T   GPIO_NUM_4
#define PIN_LED_STATUS GPIO_NUM_2
#define PIN_BTN_RESET  GPIO_NUM_0
```

## 11.2 SAFE STATE
```cpp
const GpioSafe SAFE[] = {{PIN_RELAY_1,LOW}, {PIN_MOTOR,LOW}};
void gpio_set_all_safe() { for(auto& s:SAFE) digitalWrite(s.pin,s.level); }
// Call on: Boot, Error, OTA, Safe Mode
```

## 11.3 INIT ORDER
```
1.Power -> 2.Clock -> 3.GPIO -> 4.I2C/SPI/UART -> 5.Sensors -> 6.Actuators -> 7.WiFi -> 8.MQTT
```

================================================================================
# PHẦN 12: TIME MANAGEMENT
================================================================================
TAGS: #TIME #NTP #RTC #TIMESTAMP #TIMEZONE #SYNC #CLOCK
WHEN: Time sync | Timestamp validation | Timezone config | Schedule task
SEE ALSO: #WIFI(8) #NVS(18)

## 12.1 NTP CONFIG
```
Servers: pool.ntp.org, time.google.com, time.cloudflare.com
Sync: On WiFi connect, then every 6-24h
```

## 12.2 TIMESTAMP VALIDATION
| Check | Condition | Action |
|-------|-----------|--------|
| Year valid | year >= BUILD_YEAR | Mark "time_not_synced" |
| Not future | ts <= now + 60s | Reject |
| Not too old | ts >= now - 24h | Log warning |

```cpp
#define BUILD_YEAR ((__DATE__[7]-'0')*1000 + (__DATE__[8]-'0')*100 + (__DATE__[9]-'0')*10 + (__DATE__[10]-'0'))
bool isTimeValid(time_t ts) { return (year(ts) >= BUILD_YEAR); }
```

## 12.3 TIMEZONE
```cpp
int16_t tzOffset = 420;  // UTC+7 = 7*60 minutes
// Store in NVS, allow remote change, NEVER hardcode
```

================================================================================
# PHẦN 13: SENSOR MANAGEMENT
================================================================================
TAGS: #SENSOR #ADC #SAMPLING #FILTER #CALIBRATION #TEMPERATURE #HUMIDITY
WHEN: Read sensor | Filter noise | Calibrate | ADC issue | Outlier detection
SEE ALSO: #PROTOCOL(14) for I2C/SPI | #GPIO(11)

## 13.1 SAMPLING RATES
| Sensor Type | Rate | Reason |
|-------------|------|--------|
| Temperature/Humidity | 1-10s | Slow change |
| Motion/PIR | 100ms | Fast response needed |
| Current/Power | 100-500ms | Fast change |
| Button/Switch | 10-50ms | User interaction |

## 13.2 FILTERING
```cpp
// Moving Average
float avg = sum(buffer) / size;

// Exponential MA (less RAM)
ema = alpha * newVal + (1-alpha) * ema;  // alpha: 0.1=smooth, 0.5=responsive

// Median (removes spikes)
sort(buffer); return buffer[size/2];
```

## 13.3 OUTLIER DETECTION
```
Reject if: |value-avg| > 3*stddev | value < MIN or > MAX
Suspect if: |value-lastValue| > MAX_CHANGE_RATE
Fault if: consecutive outliers > 5
Action: Log warning, keep old value, report if persistent
```

## 13.4 CALIBRATION
```cpp
typedef struct {
    float offset, scale;     // Factory
    float userOffset;        // Field adjustment
    uint32_t calibDate, crc;
} SensorCal_t;

// Apply: calibrated = (raw - offset) * scale + userOffset
// Store in separate NVS partition, survives Factory Reset
```

## 13.5 ADC CALIBRATION (ESP32-SPECIFIC)
```cpp
#include "esp_adc_cal.h"
esp_adc_cal_characteristics_t adc_chars;

// Initialize with calibration
esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);

// Read with calibration (returns mV)
uint32_t voltage_mv;
esp_adc_cal_get_voltage(ADC_CHANNEL_6, &adc_chars, &voltage_mv);
```
| ADC Issue | Solution |
|-----------|----------|
| Non-linear | Use esp_adc_cal (eFuse calibration) |
| Noisy | Multisampling (8-64 samples) |
| WiFi interference | Read when WiFi idle or use ADC2 carefully |
| Input impedance | Add buffer opamp for high-Z sources |

================================================================================
# PHẦN 14: COMMUNICATION PROTOCOLS
================================================================================
TAGS: #PROTOCOL #I2C #SPI #UART #RS485 #MODBUS #SERIAL #BUS
WHEN: Connect peripheral | Bus error | Multi-device | Protocol timing
SEE ALSO: #SENSOR(13) #GPIO(11) #THREAD(4) for mutex

## 14.1 I2C
```
Pull-ups: 10kΩ(<100kHz), 4.7kΩ(100-400kHz), 2.2kΩ(>400kHz)
Timeout: Wire.setTimeOut(100)
Recovery: 9 clock pulses + STOP condition when SDA stuck
```

## 14.2 SPI
| Device | Clock | Mode |
|--------|-------|------|
| SD Card | 1-25MHz | 0 |
| Display | 40-80MHz | 0 |
| ADC | 1-2MHz | 0 |

```
CS: Separate per device, HIGH when idle, 1μs between transactions
Shared bus: Use mutex, beginTransaction/endTransaction
```

## 14.3 UART/RS485
```cpp
#define UART_BAUD 115200
#define UART_CONFIG SERIAL_8N1

// Protocol framing: START(0xAA) + LEN + PAYLOAD + CRC + END(0x55)
```

## 14.4 MODBUS RTU
```
Timing: Inter-char=1.5 chars, Inter-frame=3.5 chars, Response=100-1000ms
Errors: CRC fail->retransmit, Timeout->retry(max 3), Continuous fail->offline
```

================================================================================
# PHẦN 15: ACTUATOR SAFETY
================================================================================
TAGS: #ACTUATOR #RELAY #MOTOR #PWM #INTERLOCK #ESTOP #OVERCURRENT
WHEN: Control relay/motor | Safety interlock | Emergency stop | Current limit
SEE ALSO: #SAFETY(2) #GPIO(11) #THREAD(4) for ISR

## 15.1 RELAY INTERLOCK
```cpp
// NEVER allow conflicting relays ON simultaneously
const RelayPair INTERLOCKS[] = {
    {MOTOR_FWD, MOTOR_REV},
    {HEATER, COOLER},
    {VALVE_OPEN, VALVE_CLOSE}
};
// Check interlock BEFORE setting relay ON
```

## 15.2 RELAY TIMING
```
After OFF: Wait 50-100ms before ON another (same group)
Contactor: Wait 200-500ms for contact stability
Zero-cross SSR: Max 10ms at 50Hz
```

## 15.3 MOTOR CONTROL
```cpp
// Soft start/stop to avoid current spike
void softStart(uint8_t target, uint16_t rampMs) {
    for(int i=0; i<=rampMs/10; i++) {
        ledcWrite(ch, target*i*10/rampMs);
        vTaskDelay(10);
    }
}
```

## 15.4 OVERCURRENT
```
Monitor: ACS712/INA219 @ 100-500ms
Thresholds: >120%=warn, >150%@5s=reduce, >200%=STOP+alert
Cooldown: 30s before allow restart
```

## 15.5 E-STOP
```cpp
void IRAM_ATTR estopISR() {
    REG_WRITE(GPIO_OUT_W1TC_REG, ACTUATOR_MASK);  // Direct OFF
    estopFlag = true;
}
// Main: Log, Alert, Safe Mode, Wait manual reset
```

================================================================================
# PHẦN 16: LED & DISPLAY
================================================================================
TAGS: #LED #DISPLAY #OLED #LCD #BLINK #PATTERN #STATUS #UI
WHEN: Status indication | User feedback | Display update | OLED burn-in
SEE ALSO: #PROTOCOL(14) for I2C display

## 16.1 LED PATTERNS
| Pattern | Meaning |
|---------|---------|
| Solid ON | Connected OK |
| Blink 1Hz | Connecting |
| Blink 2Hz | Provisioning |
| Blink 0.5Hz | Disconnected |
| Blink 5Hz | Error |
| Double blink | OTA in progress |
| OFF | Deep Sleep |

## 16.2 DISPLAY UPDATES
| Content | Rate | Reason |
|---------|------|--------|
| Sensor values | 1-5s | Avoid flicker |
| Clock | 1s | User expectation |
| Progress | 100-500ms | Smooth |
| Errors | Immediate | Critical |
| Static | On change only | OLED burn-in |

================================================================================
# PHẦN 17: POWER MANAGEMENT
================================================================================
TAGS: #POWER #SLEEP #DEEPSLEEP #BATTERY #WAKEUP #LOWPOWER #MODEM
WHEN: Battery device | Reduce power | Deep sleep | Wake source
SEE ALSO: #SAFETY(2) for brownout | #MEMORY(3) for RTC

## 17.1 SLEEP MODES
| Mode | Current | Wake Time | WiFi | Use Case |
|------|---------|-----------|------|----------|
| Active | ~240mA | - | Yes | Processing |
| Modem Sleep | ~20mA | <3ms | Yes(DTIM) | Idle, plugged |
| Light Sleep | ~0.8mA | <3ms | Optional | Need fast wake |
| Deep Sleep | ~10μA | ~250ms | No | Battery, long interval |

## 17.2 DEEP SLEEP CHECKLIST
```
BEFORE: Save to RTC_DATA_ATTR | Close WiFi/MQTT | Set wake source | gpio_set_all_safe()
AFTER: Check wakeup_cause | Read RTC state | Process | Decide: work or sleep again
```

================================================================================
# PHẦN 18: DATA INTEGRITY
================================================================================
TAGS: #NVS #DATA #CRC #STORAGE #CONFIG #BACKUP #DEFAULT
WHEN: Save config | Load settings | Data corruption | Default values
SEE ALSO: #FS(25) #MEMORY(3) #SECURITY(5)

## 18.1 NVS BEST PRACTICES
```
CRC: Store data+CRC32, verify on read, fallback to default if fail
Double write: config_main + config_backup for critical data
Atomic: nvs_set_*() multiple keys, nvs_commit() once
```

## 18.2 DEFAULT VALUES
```cpp
const Config DEFAULT = {.wifiTimeout=30000, .mqttKeepAlive=60, .sensorInterval=5000};
if (loadFromNVS(&config) != OK) config = DEFAULT;
```

================================================================================
# PHẦN 19: TESTING & MANUFACTURING
================================================================================
TAGS: #TEST #UNITTEST #SMOKE #STRESS #PRODUCTION #FACTORY #QA
WHEN: Test code | Production build | Factory reset | Quality check
SEE ALSO: #DIAGNOSTIC(22) #SAFETY(2)

## 19.1 TEST TYPES
| Type | Purpose | Frequency |
|------|---------|-----------|
| Unit Test | Pure logic | Every commit |
| Smoke Test | Boot/WiFi/MQTT | Every build |
| Integration | Module interaction | Every release |
| Stress | 72h continuous | Before production |
| Boundary | Weak RSSI, max payload | Before production |
| Recovery | Power loss, network loss | Before production |

## 19.2 PRODUCTION VS DEV
| Feature | Dev | Production |
|---------|-----|------------|
| Serial debug | ON | OFF |
| Log level | DEBUG | INFO |
| Assert | ON | OFF |
| JTAG | ON | OFF |
| HTTP OTA | OK | HTTPS only |

## 19.3 FACTORY RESET
```
Trigger: Hold RESET 10s OR remote command
Action: Clear user_config NVS, Clear WiFi, KEEP calibration, KEEP serial number
```

================================================================================
# PHẦN 20: AI-ASSISTED DEVELOPMENT
================================================================================
TAGS: #AIDEV #CODEORG #DOCUMENTATION #ERRORREPORT #GIT #COMMENT
WHEN: Report bug to AI | Organize code | Document function | Git commit
SEE ALSO: #ERROR(6) #LOG(7) #CORE(1)

## 20.1 CODE ORGANIZATION
```
Max per file: 300-500 lines
Max per function: 50-100 lines
One file = One class/feature
Use section markers: // ===== SECTION NAME =====
```

## 20.2 FUNCTION DOCUMENTATION
```cpp
/**
 * @brief Connect WiFi with retry and backoff
 * @param ssid SSID (validated, not null)
 * @param pass Password (8-64 chars)
 * @return WIFI_OK(0), WIFI_ERR_TIMEOUT(1002), WIFI_ERR_WRONG_PASS(1003)
 * @note Non-blocking, result via onStateChange callback
 */
int WiFiManager::connect(const char* ssid, const char* pass);
```

## 20.3 ERROR REPORT TEMPLATE FOR AI
```
1. DESCRIPTION: [One sentence]
2. ERROR CODE: Error XXXX: MESSAGE
3. LOCATION: File: xxx.cpp, Function: xxx(), Line: XXX
4. STATE: Current: XXX, Previous: XXX
5. LOG: [Last 10-20 lines]
6. CONDITION: [When error occurs]
7. TRIED: [What you've tried]
```

## 20.4 GIT COMMIT FORMAT
```
[Module] Short description (max 72 chars)
Examples:
[WiFi] Fix reconnect loop on router restart
[MQTT] Add QoS1 for command topic
[OTA] Implement rollback after 3 failed boots
[Fix] #42 - Message lost on reconnect
```

================================================================================
# PHẦN 21: FIRMWARE OPTIMIZATION
================================================================================
TAGS: #OPTIMIZE #SIZE #SPEED #BOOT #RAM #FLASH #PROGMEM #BINARY
WHEN: Binary too large | Boot slow | RAM full | Performance issue
SEE ALSO: #MEMORY(3) #CORE(1)

## 21.1 BINARY SIZE
```ini
# platformio.ini
build_flags = -Os -ffunction-sections -fdata-sections -Wl,--gc-sections -DCORE_DEBUG_LEVEL=0
```
| Technique | Savings |
|-----------|---------|
| Disable unused libs | 10-50KB |
| F() for strings | RAM |
| PROGMEM for const | RAM |
| Avoid String class | 5-20KB |

## 21.2 BOOT TIME
```
Target: <3s to operational
Optimize: Lazy init | Parallel init (2 cores) | Skip unnecessary checks | Fast WiFi (saved BSSID/channel)
```

## 21.3 RAM VS FLASH
| Data | Location | Reason |
|------|----------|--------|
| Const tables | Flash/PROGMEM | Never changes |
| Working buffers | RAM | Fast access |
| Large buffers | PSRAM | >10KB |
| Fixed strings | Flash | Save RAM |
| Fonts/Images | SPIFFS | Too large for RAM |

================================================================================
# PHẦN 22: REMOTE DIAGNOSTICS
================================================================================
TAGS: #DIAGNOSTIC #HEALTH #REMOTE #MONITOR #COMMAND #TELEMETRY
WHEN: Remote monitoring | Health check | Debug deployed device | Send command
SEE ALSO: #MQTT(9) #JSON(23) #ERROR(6)

## 22.1 HEALTH CHECK PAYLOAD
```json
{"device_id":"AABBCCDDEEFF","fw":"1.2.3","uptime":86400,"heap_kb":120,"rssi":-65,"reconnects":3,"temp_c":45,"errors":[{"code":1002,"count":2}]}
```

## 22.2 REMOTE COMMANDS
| Command | Action |
|---------|--------|
| restart | Reboot device |
| get_config | Return config JSON |
| set_log_level | Change verbosity |
| run_selftest | Execute HW test |
| factory_reset | Reset to defaults |
| get_logs | Send last 100 entries |

================================================================================
# PHẦN 23: JSON PARSING
================================================================================
TAGS: #JSON #PARSE #SERIALIZE #ARDUINOJSON #CJSON #PAYLOAD
WHEN: Parse MQTT payload | Create JSON response | Estimate buffer size
SEE ALSO: #MQTT(9) #HTTP(24) #MEMORY(3)

## 23.1 LIBRARY CHOICE
| Library | Pros | Cons | Use When |
|---------|------|------|----------|
| ArduinoJson | Easy API, safe | RAM heavy | Prototype, small JSON |
| cJSON | Light, ESP-IDF native | Manual memory | Production, large JSON |
| StaticJsonDocument | No heap alloc | Fixed size | Known max size |

## 23.2 SAFE PARSING PATTERN
```cpp
// ArduinoJson - Static allocation (preferred)
StaticJsonDocument<512> doc;
DeserializationError err = deserializeJson(doc, payload);

if (err) {
    LOG_ERR("JSON", "parse", "Failed: %s", err.c_str());
    return ERR_JSON_PARSE;
}

// Safe field access with defaults
const char* cmd = doc["cmd"] | "unknown";
int value = doc["value"] | -1;
bool flag = doc["flag"] | false;

// Check required fields
if (!doc.containsKey("cmd")) return ERR_JSON_MISSING_FIELD;
```

## 23.3 JSON SIZE ESTIMATION
```
Rule: JSON size ≈ 2 * (keys + values + nesting)
Example: {"temp":25.5,"hum":60} = ~30 bytes -> Doc size: 64-128 bytes
Buffer: Add 20% margin for ArduinoJson overhead
```

## 23.4 SERIALIZATION
```cpp
StaticJsonDocument<256> doc;
doc["device"] = DEVICE_ID;
doc["temp"] = roundf(temp * 10) / 10;  // 1 decimal
doc["ts"] = now();

char buffer[256];
size_t len = serializeJson(doc, buffer);
mqttPublish(topic, buffer, len);
```

================================================================================
# PHẦN 24: HTTP CLIENT
================================================================================
TAGS: #HTTP #HTTPS #REST #API #GET #POST #DOWNLOAD #STREAM
WHEN: Call REST API | Download file | OTA from HTTP | Large response
SEE ALSO: #OTA(10) #SECURITY(5) #JSON(23)

## 24.1 HTTPS REQUIRED
```cpp
// Production: ALWAYS use HTTPS with certificate verification
WiFiClientSecure client;
client.setCACert(root_ca);  // Pin CA certificate

// Dev only: Skip verification (NEVER in production)
// client.setInsecure();
```

## 24.2 HTTP REQUEST PATTERN
```cpp
HTTPClient http;
http.setTimeout(10000);  // 10s timeout
http.setConnectTimeout(5000);  // 5s connect timeout

http.begin(client, url);
http.addHeader("Content-Type", "application/json");
http.addHeader("Authorization", "Bearer " + token);

int code = http.POST(payload);

if (code == HTTP_CODE_OK) {
    String response = http.getString();
    // Parse response
} else if (code < 0) {
    LOG_ERR("HTTP", "post", "Connection failed: %s", http.errorToString(code).c_str());
} else {
    LOG_ERR("HTTP", "post", "Server error: %d", code);
}
http.end();  // ALWAYS close
```

## 24.3 RETRY STRATEGY
| HTTP Code | Action |
|-----------|--------|
| 200-299 | Success |
| 400-499 | Client error, don't retry (except 429) |
| 429 | Rate limited, backoff retry |
| 500-599 | Server error, retry with backoff |
| Timeout | Retry with backoff (max 3) |

## 24.4 LARGE RESPONSE STREAMING
```cpp
// For responses > 10KB, use stream
WiFiClient* stream = http.getStreamPtr();
while (stream->available()) {
    char c = stream->read();
    // Process byte-by-byte or chunk
}
```

================================================================================
# PHẦN 25: FILE SYSTEM (SPIFFS/LittleFS)
================================================================================
TAGS: #FS #FILESYSTEM #SPIFFS #LITTLEFS #FILE #STORAGE #LOG
WHEN: Store files | Log to flash | Config file | Large data storage
SEE ALSO: #NVS(18) #MEMORY(3)

## 25.1 COMPARISON
| Feature | SPIFFS | LittleFS |
|---------|--------|----------|
| Wear leveling | Yes | Yes |
| Directories | No | Yes |
| Speed | Slower | Faster |
| Power-safe | Partial | Yes |
| Recommendation | Legacy | **Preferred** |

## 25.2 INITIALIZATION
```cpp
#include <LittleFS.h>

if (!LittleFS.begin(true)) {  // true = format on fail
    LOG_ERR("FS", "init", "Mount failed");
    return ERR_FS_MOUNT;
}
LOG_INF("FS", "init", "Total:%dKB, Used:%dKB", 
    LittleFS.totalBytes()/1024, LittleFS.usedBytes()/1024);
```

## 25.3 SAFE FILE OPERATIONS
```cpp
// Write with temp file (power-safe)
bool safeWrite(const char* path, const uint8_t* data, size_t len) {
    String tempPath = String(path) + ".tmp";
    
    File f = LittleFS.open(tempPath, "w");
    if (!f) return false;
    
    size_t written = f.write(data, len);
    f.close();
    
    if (written != len) {
        LittleFS.remove(tempPath);
        return false;
    }
    
    LittleFS.remove(path);           // Remove old
    LittleFS.rename(tempPath, path); // Atomic rename
    return true;
}
```

## 25.4 FILE SYSTEM RULES
| Rule | Reason |
|------|--------|
| Max 10 open files | Memory limit |
| Close after use | Prevent leaks |
| Check free space before write | Prevent corruption |
| Use subdirectories (LittleFS) | Organization |
| Periodic garbage collection | SPIFFS fragmentation |

================================================================================
# PHẦN 26: BLE MANAGEMENT
================================================================================
TAGS: #BLE #BLUETOOTH #GATT #PROVISION #ADVERTISE #CHARACTERISTIC
WHEN: BLE provisioning | Phone app connection | BLE+WiFi coexist | BLE security
SEE ALSO: #WIFI(8) #SECURITY(5)

## 26.1 BLE VS WIFI COEXISTENCE
```
ESP32 shares antenna: WiFi + BLE can interfere
Recommendation: 
  - Use BLE for provisioning, then disable
  - If need both: reduce BLE TX power, use WiFi channel 1/6/11
```

## 26.2 BLE PROVISIONING FLOW
```
1. Start BLE advertising (30s-5min timeout)
2. Phone connects, discovers services
3. Phone writes WiFi credentials to characteristic
4. ESP32 validates, attempts WiFi connect
5. ESP32 notifies result via characteristic
6. On success: Stop BLE, save credentials
7. On fail: Allow retry (max 3)
```

## 26.3 GATT SERVICE STRUCTURE
```cpp
// Provisioning Service UUID: Custom or use Espressif's
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_SSID_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_PASS_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define CHAR_STATUS_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26aa"

// Characteristics:
// SSID: Write (max 32 bytes)
// Password: Write (max 64 bytes)  
// Status: Read + Notify (0=idle, 1=connecting, 2=success, 3=fail)
```

## 26.4 BLE SECURITY
| Threat | Mitigation |
|--------|------------|
| Sniffing | Use BLE pairing with PIN |
| Unauthorized access | Timeout advertising |
| Replay attack | One-time token |
| MITM | Secure pairing (LESC) |

================================================================================
# PHẦN 27: BUTTON & INPUT HANDLING
================================================================================
TAGS: #BUTTON #INPUT #DEBOUNCE #LONGPRESS #INTERRUPT #SWITCH
WHEN: Handle button | Debounce | Long press | Factory reset button | ISR input
SEE ALSO: #THREAD(4) for ISR rules | #GPIO(11)

## 27.1 HARDWARE DEBOUNCE
```
RC filter: R=10kΩ, C=100nF -> τ=1ms
Schmitt trigger: 74HC14 for noisy environments
```

## 27.2 SOFTWARE DEBOUNCE
```cpp
#define DEBOUNCE_MS 50

class Button {
    uint8_t pin_;
    uint32_t lastChange_ = 0;
    bool lastState_ = HIGH;
    bool stableState_ = HIGH;
    
public:
    bool read() {
        bool current = digitalRead(pin_);
        if (current != lastState_) {
            lastChange_ = millis();
            lastState_ = current;
        }
        if ((millis() - lastChange_) > DEBOUNCE_MS) {
            stableState_ = lastState_;
        }
        return stableState_;
    }
};
```

## 27.3 BUTTON EVENTS
| Event | Detection | Use Case |
|-------|-----------|----------|
| Press | Falling edge | Immediate action |
| Release | Rising edge | End action |
| Short press | <500ms | Normal function |
| Long press | >2s | Secondary function |
| Very long | >10s | Factory reset |
| Double click | 2 presses <300ms | Alternate function |

## 27.4 LONG PRESS PATTERN
```cpp
void buttonTask() {
    static uint32_t pressStart = 0;
    bool pressed = (digitalRead(BTN) == LOW);
    
    if (pressed && pressStart == 0) {
        pressStart = millis();
    } else if (pressed) {
        uint32_t duration = millis() - pressStart;
        if (duration > 10000) {
            factoryReset();  // Very long press
        } else if (duration > 2000) {
            showResetWarning();  // Long press feedback
        }
    } else {
        if (pressStart > 0) {
            uint32_t duration = millis() - pressStart;
            if (duration < 500) onShortPress();
            else if (duration < 2000) onMediumPress();
            pressStart = 0;
        }
    }
}
```

## 27.5 INTERRUPT-BASED INPUT
```cpp
volatile bool buttonPressed = false;

void IRAM_ATTR buttonISR() {
    buttonPressed = true;  // Just set flag, process in main loop
}

// In setup:
attachInterrupt(BTN_PIN, buttonISR, FALLING);

// In loop:
if (buttonPressed) {
    buttonPressed = false;
    // Debounce check here before processing
}
```

================================================================================
# PHẦN 28: EXPERT REVIEW CHECKLIST
================================================================================
TAGS: #EXPERT #REVIEW #AUDIT #COMPLIANCE #CERTIFICATION #SECURITY
WHEN: Before production | Security audit | Certification prep | Scale >1000 units
SEE ALSO: #TEST(19) #SECURITY(5) #SAFETY(2)

## 28.1 HARDWARE REVIEW (Cần EE Expert)
| Item | Check | Pass Criteria |
|------|-------|---------------|
| Schematic | Power supply design | Clean 3.3V, <50mV ripple |
| Schematic | Decoupling caps | 100nF per VCC pin + 10μF bulk |
| Schematic | ESD protection | TVS on all external interfaces |
| Schematic | Antenna clearance | No GND pour under antenna |
| PCB | 4-layer for RF | Signal-GND-PWR-Signal |
| PCB | Impedance matching | 50Ω for RF traces |
| Power | Brownout margin | Operate stable at 2.8V |
| Power | Inrush current | <500mA with soft start |
| Thermal | Max junction temp | Tj < 85°C at max ambient |

## 28.2 FIRMWARE REVIEW
| Category | Checklist |
|----------|-----------|
| Memory | No malloc in loop? Heap stable 72h? Stack high-water OK? |
| Watchdog | All tasks feed WDT? WDT timeout appropriate? |
| Error handling | All errors logged? Graceful degradation? |
| Security | No hardcoded secrets? TLS verified? Debug disabled? |
| OTA | Rollback works? Version check? Signature verify? |
| Power | Sleep mode works? Wake sources correct? RTC data saved? |
| Edge cases | WiFi loss handled? MQTT reconnect? NVS corrupt? |

## 28.3 SECURITY AUDIT (Cần Security Expert)
| Attack Vector | Test Method | Mitigation Verify |
|---------------|-------------|-------------------|
| Firmware dump | JTAG/UART access | Disabled in production? |
| Flash read | esptool read_flash | Flash encryption enabled? |
| Network sniff | Wireshark capture | TLS 1.2+ all connections? |
| MQTT injection | Publish fake commands | Topic ACL + payload validate? |
| Replay attack | Capture & resend | Timestamp/nonce in payload? |
| DoS | Flood commands | Rate limiting works? |
| Physical tamper | Open case, probe | Tamper detect? Secure boot? |

## 28.4 COMPLIANCE CHECKLIST
| Standard | Region | Key Requirements |
|----------|--------|------------------|
| CE (RED) | EU | EMC + Radio + Safety |
| FCC Part 15 | USA | Unintentional radiator limits |
| IC | Canada | Similar to FCC |
| TELEC | Japan | Radio certification |
| RoHS | EU | No hazardous substances |
| REACH | EU | Chemical safety |
| UL/IEC 62368 | Global | Electrical safety |

```
PRE-CERTIFICATION CHECKLIST:
□ Use certified ESP32 module (not bare chip)
□ Follow module manufacturer antenna guidelines
□ Keep RF section unchanged from reference design
□ Label with required certification marks
□ Prepare technical documentation file
```

## 28.5 CODE QUALITY METRICS
| Metric | Target | Tool |
|--------|--------|------|
| Cyclomatic complexity | <15 per function | cppcheck, SonarQube |
| Code coverage | >70% for critical | gcov, Unity |
| Static analysis | 0 high severity | cppcheck -enable=all |
| MISRA compliance | For safety-critical | PC-lint, Polyspace |
| Memory leaks | 0 | Valgrind (simulation), heap monitor |

================================================================================
# PHẦN 29: FIELD TESTING & ITERATION
================================================================================
TAGS: #FIELD #TESTING #PILOT #ITERATION #PRODUCTION #DEPLOYMENT #MONITORING
WHEN: Before mass production | After pilot | Scaling up | Continuous improvement
SEE ALSO: #TEST(19) #DIAGNOSTIC(22) #EXPERT(28)

## 29.1 TESTING PHASES
```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   DEV LAB   │ -> │   ALPHA     │ -> │    BETA     │ -> │ PRODUCTION  │
│   1-5 units │    │  10-20 units│    │  50-200 units│   │   Scale up  │
│   1-2 weeks │    │   1 month   │    │   2-3 months │   │  Continuous │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
     Lab              Internal          Real users         Mass deploy
   conditions        employees          + harsh env         + monitor
```

## 29.2 ALPHA TESTING (Internal)
| Test | Duration | Success Criteria |
|------|----------|------------------|
| Continuous run | 72h non-stop | No crash, heap stable |
| Power cycle | 100 cycles | Boot OK every time |
| WiFi stress | 24h with router restart | Auto reconnect <60s |
| OTA cycle | 20 updates | All success + rollback works |
| Edge temperature | -10°C to +50°C | Function normal |
| Power boundary | 4.5V - 5.5V input | No brownout |

## 29.3 BETA TESTING (Field)
| Environment | What to Test | Duration |
|-------------|--------------|----------|
| Real location | WiFi interference, range | 2-4 weeks |
| Real users | UX, provisioning, errors | 4-8 weeks |
| Harsh conditions | Outdoor, humidity, dust | 4-12 weeks |
| 24/7 operation | Long-term stability | 8-12 weeks |

```
BETA METRICS TO COLLECT:
- Uptime percentage (target: >99%)
- Reconnection count per day
- Error code frequency
- Heap free over time (memory leak detection)
- RSSI distribution
- OTA success rate
- User-reported issues
```

## 29.4 FIELD DATA COLLECTION
```cpp
// Daily health report structure
typedef struct {
    uint32_t uptime_sec;
    uint32_t boot_count;
    uint32_t wifi_reconnects;
    uint32_t mqtt_reconnects;
    uint32_t error_counts[10];  // Top 10 error codes
    int8_t rssi_min, rssi_max, rssi_avg;
    uint32_t heap_min, heap_max;
    int8_t temp_max;
    char fw_version[16];
} DailyReport_t;

// Send daily at midnight or on request
```

## 29.5 ISSUE TRACKING TEMPLATE
```markdown
## FIELD ISSUE #XXX
- **Device ID:** AABBCCDDEEFF
- **FW Version:** 1.2.3
- **Location:** Site A, Building 2
- **Environment:** Indoor, high humidity
- **Symptom:** Device offline every morning ~6AM
- **Error Log:**
  [ERR][WIFI][connect] Failed, error=1002, rssi=-82
  [WRN][SYSTEM][health] Heap: 28KB (low)
- **Frequency:** Daily for 5 days
- **Hypothesis:** WiFi congestion at shift change
- **Action:** Increase backoff, add channel scan
- **Result:** Fixed in v1.2.4, monitoring...
```

## 29.6 ITERATION CYCLE
```
┌──────────────────────────────────────────────────────────────┐
│  1. COLLECT         2. ANALYZE         3. FIX               │
│  Field data    ->   Root cause    ->   Code change          │
│  Error logs         Pattern find       Test locally         │
│  User feedback      Prioritize                              │
└──────────────────────────────────────────────────────────────┘
                              │
                              v
┌──────────────────────────────────────────────────────────────┐
│  6. MONITOR         5. ROLLOUT         4. VALIDATE          │
│  Same metrics  <-   OTA to fleet  <-   Beta group first     │
│  Compare before/    Staged: 10%->      Confirm fix works    │
│  after              50%->100%          No regression        │
└──────────────────────────────────────────────────────────────┘
```

## 29.7 GO/NO-GO CRITERIA FOR PRODUCTION
| Metric | Minimum | Target |
|--------|---------|--------|
| Uptime | >95% | >99% |
| Boot success | >99% | 100% |
| OTA success | >95% | >99% |
| Critical errors/day | <1 | 0 |
| Mean time between failure | >30 days | >90 days |
| User-reported issues | <5% devices | <1% |
| Memory leak | None detected in 30 days | - |
| Security vulnerabilities | 0 critical | 0 any |

## 29.8 PRODUCTION MONITORING DASHBOARD
```
REAL-TIME METRICS:
├── Fleet overview: X devices online, Y offline, Z errors
├── Error heatmap: Which errors, which FW version, which location
├── OTA status: Rollout progress, success rate, rollback count
├── Health trends: Heap, uptime, reconnects over time
└── Alerts: Anomaly detection (sudden error spike, mass offline)

WEEKLY REPORT:
├── New issues discovered
├── Issues resolved
├── FW version distribution
├── Performance trends
└── Action items for next iteration
```

## 29.9 CONTINUOUS IMPROVEMENT LOOP
| Phase | Action | Output |
|-------|--------|--------|
| Week 1-4 | Deploy beta, heavy monitoring | Issue list |
| Week 5-6 | Fix critical issues, OTA | v1.0.1 |
| Week 7-8 | Monitor fix effectiveness | Metrics |
| Week 9-12 | Expand deployment, new issues | v1.0.2 |
| Month 4+ | Stable operation, feature add | v1.1.0 |

```
GOLDEN RULE: 
Never deploy 100% at once.
Always: 10% -> wait 48h -> 50% -> wait 1 week -> 100%
```

================================================================================
# PHẦN 30: TROUBLESHOOTING GUIDE
================================================================================
TAGS: #TROUBLESHOOT #DEBUG #FIX #COMMON #ISSUE #SOLUTION
WHEN: Device not working | Strange behavior | Debug production issue
SEE ALSO: #ERROR(6) #LOG(7) #DIAGNOSTIC(22)

## 30.1 COMMON ISSUES & SOLUTIONS
| Symptom | Likely Cause | Solution |
|---------|--------------|----------|
| Boot loop | WDT timeout in setup | Add wdt feed, check blocking code |
| Boot loop | Exception in setup | Check Serial log, fix crash |
| Boot loop | Brownout | Check power supply, add capacitor |
| Can't connect WiFi | Wrong credentials | Clear NVS, re-provision |
| Can't connect WiFi | Weak signal | Check antenna, move closer |
| Can't connect WiFi | Channel congestion | Enable channel scan |
| WiFi drops frequently | Router issue | Check router logs, try different AP |
| WiFi drops frequently | Heap exhausted | Fix memory leak |
| MQTT won't connect | Wrong broker/port | Verify config, check firewall |
| MQTT won't connect | TLS cert expired | Update CA cert |
| MQTT disconnects | Keep-alive too long | Reduce to 30-60s |
| OTA fails | Not enough space | Check partition size |
| OTA fails | Download interrupted | Add retry, resume support |
| OTA fails | Wrong firmware | Verify project name match |
| Sensor reads wrong | Calibration needed | Run calibration routine |
| Sensor reads wrong | Noise/interference | Add filtering, check wiring |
| High power consumption | Not entering sleep | Check sleep conditions |
| High power consumption | WiFi always on | Use modem sleep |
| Device hot | High CPU usage | Profile, optimize loops |
| Device hot | Short circuit | Check hardware |

## 30.2 DEBUG DECISION TREE
```
Device not working?
├── Does it boot? (LED blink, Serial output)
│   ├── NO -> Check power (3.3V?), check flash, try reflash
│   └── YES -> Continue
├── Does WiFi connect?
│   ├── NO -> Check credentials, RSSI, router
│   └── YES -> Continue
├── Does MQTT connect?
│   ├── NO -> Check broker, port, TLS, credentials
│   └── YES -> Continue
├── Does it send data?
│   ├── NO -> Check topic, payload, QoS
│   └── YES -> Continue
├── Is data correct?
│   ├── NO -> Check sensor, calibration, filtering
│   └── YES -> System OK!
```

## 30.3 EMERGENCY RECOVERY
| Situation | Recovery Method |
|-----------|-----------------|
| Brick after bad OTA | Boot button + reflash via USB |
| Stuck in boot loop | Hold BOOT, release after power on, flash |
| NVS corrupted | Erase NVS partition, reconfigure |
| Total brick | UART bootloader mode, full erase + flash |
| Production device inaccessible | OTA rollback (if enabled) |
| Mass issue in field | Server-side disable, investigate, OTA fix |

```cpp
// Emergency NVS recovery (add to firmware)
if (bootCount > 5 && lastBootTime < 60) {
    LOG_WRN("SYSTEM", "boot", "Multiple rapid reboots, clearing config");
    nvs_flash_erase();
    nvs_flash_init();
    // Will boot with defaults
}
```

================================================================================
# PHỤ LỤC: PROMPT CHO AI
================================================================================
TAGS: #PROMPT #AI #KEHOACH #LEARNING
WHEN: Cần tạo kế hoạch học ESP32 mới

## A.1 TẠO FILE KEHOACH.MD
**Prompt tối ưu để AI tạo kế hoạch học:**
```
Tạo file kehoach.md cho ESP32 WROOM-32 với yêu cầu:
1. Chỉ phần mềm, phần cứng có file riêng (hardware.md)
2. Format cho AI dễ hiểu và thực hiện khi user nói "Làm ngày X"
3. Không cần code mẫu, AI sẽ tự viết theo rule.md
4. Mỗi ngày có format TASK block:
   - TASK: [Tên task]
   - INPUT: [User cần chuẩn bị gì]
   - OUTPUT: [Kết quả mong đợi]
   - VERIFY: [Cách kiểm tra thành công]
   - RULES: [#TAGS trong rule.md cần tuân theo]
5. Có progress tracking table
6. Thời gian: 4 tuần, 2h/ngày
7. Nội dung: GPIO, PWM, Touch, Hall, WiFi, HTTP, BLE, NVS, FileSystem, Sleep, RTOS, OTA, MQTT
```

## A.2 SỬ DỤNG KEHOACH.MD
| Lệnh | AI sẽ làm |
|------|-----------|
| "Làm ngày X" | Tạo project, viết code đầy đủ, hướng dẫn test |
| "Kiểm tra môi trường" | Tạo blink project, verify PlatformIO OK |
| "Review ngày X" | Kiểm tra code đã làm, góp ý cải thiện |
| "Tiếp tục" | Làm ngày tiếp theo chưa hoàn thành |

================================================================================
END OF DOCUMENT
================================================================================

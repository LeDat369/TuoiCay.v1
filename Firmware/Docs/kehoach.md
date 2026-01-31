# KẾ HOẠCH DỰ ÁN: HỆ THỐNG TƯỚI CÂY TỰ ĐỘNG
================================================================================
VERSION: 1.0.0
MCU: ESP8266 (NodeMCU)
FRAMEWORK: Arduino
AI_EXECUTOR: GitHub Copilot / Claude
RULE_VERSION: rule.md v3.0
================================================================================

## TỔNG QUAN DỰ ÁN

### Mục tiêu
Xây dựng hệ thống tưới cây tự động với:
- Đọc độ ẩm đất từ 2 cảm biến
- Điều khiển máy bơm qua MOSFET
- Kết nối WiFi để điều khiển từ xa
- **MQTT để giao tiếp với server/app**
- Lưu cấu hình vào LittleFS (không dùng EEPROM deprecated)
- Hẹn giờ tưới tự động
- OTA update firmware

---

## THÔNG TIN KẾT NỐI (CREDENTIALS)

### WiFi
```
SSID:     WIFI-IOT
PASSWORD: hnh.2025
```

### MQTT Broker
```
BROKER:   192.168.221.5
PORT:     1883 (plain) / 8883 (TLS - production)
USERNAME: (empty)
PASSWORD: (empty)
```

### MQTT Topics Structure
```
DEVICE_ID = MAC address (ví dụ: "AABBCCDDEEFF")

PUBLISH (Device → Server):
├── devices/{deviceId}/sensor/data     [QoS 0] - Sensor readings mỗi 5s
│   Payload: {"moisture1": 45, "moisture2": 52, "moistureRaw": 512, "ts": 1706700000}
│
├── devices/{deviceId}/pump/status     [QoS 1] - Pump state changes
│   Payload: {"running": true, "runtime": 30, "reason": "auto"}
│
└── devices/{deviceId}/mode            [QoS 1] - Mode changes
    Payload: {"mode": "auto", "threshold_dry": 30, "threshold_wet": 50}

SUBSCRIBE (Server → Device):
├── devices/{deviceId}/config          [QoS 1] - Configuration update
│   Payload: {"threshold_dry": 30, "threshold_wet": 50, "max_runtime": 60}
│
├── devices/{deviceId}/pump/control    [QoS 1] - Pump commands
│   Payload: {"action": "on"|"off"|"toggle", "duration": 30}
│
└── devices/{deviceId}/mode/control    [QoS 1] - Mode commands
    Payload: {"mode": "auto"|"manual"}

LWT (Last Will Testament):
└── devices/{deviceId}/status          [QoS 1, Retain]
    Online:  {"online": true, "ip": "192.168.1.100", "fw": "1.0.0"}
    Offline: {"online": false}
```

### QoS Strategy (theo rule.md #MQTT(9.2))
| Topic Type | QoS | Reason |
|------------|-----|--------|
| sensor/data | 0 | Periodic, loss acceptable |
| pump/status | 1 | State change, must deliver |
| mode | 1 | Configuration, must deliver |
| config | 1 | Commands, critical |
| pump/control | 1 | Commands, critical |
| mode/control | 1 | Commands, critical |
| status (LWT) | 1 | Online/offline, must deliver |

---

## ERROR CODES (theo rule.md #ERROR(6))

| Range | Module | Codes |
|-------|--------|-------|
| 0 | OK | 0 = Success |
| 1xxx | WiFi | 1001=ConnectFail, 1002=Timeout, 1003=WrongPass |
| 2xxx | MQTT | 2001=ConnectFail, 2002=PublishFail, 2003=SubscribeFail |
| 3xxx | Sensor | 3001=NotFound, 3002=ReadFail, 3003=OutOfRange |
| 4xxx | Storage | 4001=InitFail, 4002=ReadFail, 4003=WriteFail, 4004=CRCFail |
| 5xxx | OTA | 5001=DownloadFail, 5002=VerifyFail, 5003=FlashFail |
| 6xxx | Pump | 6001=Timeout, 6002=Overcurrent, 6003=SafetyTrip |

---

## PHẦN CỨNG (Từ Pinout.md)

```
MCU: ESP8266 NodeMCU
├── D6 (GPIO12) → MOSFET Gate (điều khiển bơm)
├── D5 (GPIO14) → Sensor 1 Digital
├── D1 (GPIO5)  → Sensor 2 Digital
├── A0 (ADC)    → Sensor 2 Analog
├── LED_BUILTIN → Status indicator
├── 3.3V        → Cấp nguồn cảm biến
└── GND         → Mass chung

MOSFET: N-Channel (low-side switch)
├── Gate   → D6 + 10kΩ pulldown
├── Drain  → Pump (-)
└── Source → GND

Pump: 5V DC Mini
├── (+) → +5V adapter
└── (-) → MOSFET Drain
```

---

## CẤU TRÚC THƯ MỤC ĐÍCH (theo rule.md #CORE(1.2))

```
Firmware/
├── include/                    # Project-wide headers
│   ├── config.h                # Version, constants, timeouts
│   ├── pins.h                  # GPIO mapping
│   ├── secrets.h               # WiFi, MQTT credentials (KHÔNG COMMIT!)
│   ├── secrets.h.example       # Template for secrets.h
│   ├── error_codes.h           # Error code definitions
│   └── device_state.h          # Device state model
│
├── lib/                        # Reusable libraries (có library.json)
│   ├── TuoiCay_Drivers/        # Hardware drivers
│   │   ├── library.json
│   │   └── src/
│   │       ├── sensor_driver.h      # Soil moisture sensor
│   │       ├── sensor_driver.cpp
│   │       ├── pump_driver.h        # Pump control with safety
│   │       └── pump_driver.cpp
│   │
│   ├── TuoiCay_Managers/       # High-level managers
│   │   ├── library.json
│   │   └── src/
│   │       ├── wifi_manager.h       # WiFi connection + provisioning
│   │       ├── wifi_manager.cpp
│   │       ├── mqtt_manager.h       # MQTT client + LWT + queue
│   │       ├── mqtt_manager.cpp
│   │       ├── web_server.h         # HTTP server for control
│   │       ├── web_server.cpp
│   │       ├── storage_manager.h    # LittleFS config storage
│   │       ├── storage_manager.cpp
│   │       ├── scheduler.h          # Watering schedule
│   │       ├── scheduler.cpp
│   │       ├── time_manager.h       # NTP sync
│   │       ├── time_manager.cpp
│   │       ├── ota_manager.h        # OTA update
│   │       └── ota_manager.cpp
│   │
│   └── TuoiCay_Utils/          # Utilities
│       ├── library.json
│       └── src/
│           ├── logger.h             # Logging macros [INF][MOD][func]
│           └── crc_utils.h          # CRC for config verification
│
├── src/                        # Application code
│   └── main.cpp                # Entry point only (setup/loop)
│
├── data/                       # LittleFS files (web UI)
│   └── index.html              # Dashboard HTML
│
├── test/                       # Unit tests
│   └── README
│
└── platformio.ini              # Build configuration
```

### Library Include Style (theo rule.md):
```cpp
// ✅ CORRECT - Use angle brackets for library headers
#include <config.h>           // From include/
#include <logger.h>           // From lib/TuoiCay_Utils/
#include <sensor_driver.h>    // From lib/TuoiCay_Drivers/
#include <mqtt_manager.h>     // From lib/TuoiCay_Managers/

// ❌ WRONG - Avoid relative paths
#include "../../utils/logger.h"
#include "../drivers/sensor_driver.h"
```

================================================================================
## GIAI ĐOẠN THỰC HIỆN
================================================================================

## PHASE 1: CƠ SỞ HẠ TẦNG
**Mục tiêu:** Tạo nền tảng code, định nghĩa pins, logging
**Thời gian:** 1 ngày

### TASK 1.1: Cấu hình dự án
```yaml
COMMAND: "Làm task 1.1"
INPUT:
  - platformio.ini đã có sẵn
  - Pinout.md làm reference
OUTPUT:
  - include/config.h (version, constants, timeouts)
  - include/pins.h (GPIO definitions)
  - include/logger.h (logging macros)
  - src/main.cpp (skeleton với setup/loop)
VERIFY:
  - Build thành công không lỗi
  - Serial output: "[INF][SYSTEM][boot] FW vX.X.X started"
RULES: #CORE(1.2) #LOG(7) #GPIO(11)
```

### TASK 1.2: Safe state và Watchdog
```yaml
COMMAND: "Làm task 1.2"
INPUT:
  - Task 1.1 hoàn thành
OUTPUT:
  - Watchdog timer setup (30s timeout)
  - gpio_set_safe() function (pump OFF on boot/error)
  - Boot reason detection
VERIFY:
  - Pump OFF khi boot
  - Watchdog reset sau 30s nếu không feed
RULES: #SAFETY(2) #GPIO(11.2)
```

---

## PHASE 2: DRIVERS
**Mục tiêu:** Viết drivers cho sensor và pump
**Thời gian:** 2 ngày

### TASK 2.1: Soil Moisture Sensor Driver
```yaml
COMMAND: "Làm task 2.1"
INPUT:
  - 2 sensors: Sensor1 (D5 digital), Sensor2 (D1 digital + A0 analog)
  - Đọc mỗi 5 giây
OUTPUT:
  - include/sensor.h
  - src/sensor.cpp
  - Class SoilSensor với:
    - begin()
    - readDigital() -> bool (true=khô, false=ướt)
    - readAnalog() -> uint16_t (0-1023)
    - getMoisturePercent() -> uint8_t (0-100%)
    - isValid() -> kiểm tra giá trị hợp lệ
  - Moving average filter (5 samples)
VERIFY:
  - Serial log: "[INF][SENSOR][read] S1=DRY, S2=45%, raw=512"
  - Giá trị thay đổi khi chạm/nhúng nước sensor
RULES: #SENSOR(13) #CORE(1.3)
```

### TASK 2.2: Pump Control Driver
```yaml
COMMAND: "Làm task 2.2"
INPUT:
  - D6 (GPIO12) điều khiển MOSFET Gate
  - MOSFET logic: HIGH = pump ON, LOW = pump OFF
OUTPUT:
  - include/pump.h
  - src/pump.cpp
  - Class PumpController với:
    - begin()
    - turnOn() / turnOff()
    - isRunning() -> bool
    - setMaxRuntime(seconds) -> giới hạn thời gian chạy
    - update() -> tự động tắt nếu quá maxRuntime
  - Safety: auto-off sau 60s (configurable)
VERIFY:
  - Pump ON/OFF theo lệnh Serial
  - Auto-off sau timeout
  - Log: "[INF][PUMP][on] Started, max=60s"
RULES: #ACTUATOR(15) #SAFETY(2)
```

### TASK 2.3: Tích hợp Sensor + Pump (Auto mode)
```yaml
COMMAND: "Làm task 2.3"
INPUT:
  - Task 2.1 và 2.2 hoàn thành
OUTPUT:
  - Logic tự động trong main.cpp:
    - Nếu moisture < threshold (30%) -> bật pump
    - Nếu moisture > threshold + hysteresis (50%) -> tắt pump
    - Minimum off time: 5 phút (tránh bật/tắt liên tục)
VERIFY:
  - Pump tự bật khi đất khô
  - Pump tự tắt khi đất đủ ẩm
  - Không bật/tắt liên tục (hysteresis hoạt động)
RULES: #ACTUATOR(15.1) #SENSOR(13)
```

---

## PHASE 3: WIFI & WEB SERVER
**Mục tiêu:** Kết nối WiFi, tạo giao diện web điều khiển
**Thời gian:** 2 ngày

### TASK 3.1: WiFi Manager
```yaml
COMMAND: "Làm task 3.1"
INPUT:
  - SSID/Password hardcode ban đầu (sẽ config sau)
OUTPUT:
  - include/wifi_manager.h
  - src/wifi_manager.cpp
  - Class WiFiManager với:
    - begin(ssid, password)
    - connect() với timeout 30s
    - isConnected()
    - getIP()
    - reconnect() với exponential backoff
  - State machine: IDLE -> CONNECTING -> CONNECTED -> DISCONNECTED
  - LED status indicator (optional, dùng built-in LED)
VERIFY:
  - Kết nối WiFi thành công
  - Auto reconnect khi mất kết nối
  - Log IP address
RULES: #WIFI(8) #ERROR(6)
```

### TASK 3.2: Web Server cơ bản
```yaml
COMMAND: "Làm task 3.2"
INPUT:
  - Task 3.1 hoàn thành
OUTPUT:
  - include/web_server.h
  - src/web_server.cpp
  - Endpoints:
    - GET / -> HTML dashboard
    - GET /api/status -> JSON {moisture, pump, mode, uptime}
    - POST /api/pump -> {"action": "on"|"off"|"toggle"}
    - POST /api/mode -> {"mode": "auto"|"manual"}
    - POST /api/threshold -> {"dry": 30, "wet": 50}
  - Simple HTML trong PROGMEM (không cần LittleFS)
VERIFY:
  - Truy cập http://[IP]/ hiển thị dashboard
  - Bật/tắt pump từ web
  - Chuyển mode auto/manual
RULES: #HTTP(24) #JSON(23)
```

### TASK 3.3: Web Dashboard đẹp
```yaml
COMMAND: "Làm task 3.3"
INPUT:
  - Task 3.2 hoàn thành
OUTPUT:
  - data/index.html với:
    - Hiển thị moisture % (gauge hoặc progress bar)
    - Nút ON/OFF pump
    - Toggle Auto/Manual mode
    - Cài đặt threshold
    - Auto refresh mỗi 5s
  - Mobile responsive
  - Sử dụng LittleFS để serve file
VERIFY:
  - Giao diện đẹp trên mobile
  - Real-time update không cần refresh
RULES: #FS(25) #HTTP(24)
```

---

## PHASE 4: MQTT INTEGRATION
**Mục tiêu:** Kết nối MQTT broker, publish sensor data, subscribe commands
**Thời gian:** 2 ngày

### TASK 4.1: MQTT Manager
```yaml
COMMAND: "Làm task 4.1"
INPUT:
  - WiFi đã connected (Task 3.1)
  - MQTT Broker: 192.168.221.5:1883
  - Topics structure từ kehoach.md
OUTPUT:
  - include/mqtt_manager.h
  - src/mqtt_manager.cpp
  - Class MqttManager với:
    - begin(broker, port, deviceId)
    - connect() với LWT setup
    - publish(topic, payload, qos, retain)
    - subscribe(topic, qos, callback)
    - loop() -> gọi trong main loop
    - isConnected()
  - Auto-reconnect với exponential backoff (2s -> 4s -> 8s -> 16s -> 30s max)
  - Offline queue (lưu max 10 messages khi mất kết nối)
VERIFY:
  - Connect thành công đến broker
  - LWT hoạt động (online/offline status)
  - Log: "[INF][MQTT][conn] Connected to 192.168.221.5"
RULES: #MQTT(9) #ERROR(6)
```

### TASK 4.2: MQTT Publish - Sensor & Status
```yaml
COMMAND: "Làm task 4.2"
INPUT:
  - Task 4.1 hoàn thành
  - Sensor data từ Task 2.1
  - Pump status từ Task 2.2
OUTPUT:
  - Publish sensor data mỗi 5s:
    Topic: devices/{deviceId}/sensor/data
    QoS: 0
    Payload: {"moisture1": 45, "moisture2": 52, "moistureRaw": 512, "ts": epoch}
  - Publish pump status khi thay đổi:
    Topic: devices/{deviceId}/pump/status
    QoS: 1
    Payload: {"running": true, "runtime": 30, "reason": "auto"|"manual"|"schedule"}
  - Publish mode khi thay đổi:
    Topic: devices/{deviceId}/mode
    QoS: 1
    Payload: {"mode": "auto", "threshold_dry": 30, "threshold_wet": 50}
VERIFY:
  - MQTT client (mosquitto_sub) nhận được messages
  - Data chính xác, đúng format JSON
RULES: #MQTT(9.2) #JSON(23)
```

### TASK 4.3: MQTT Subscribe - Commands
```yaml
COMMAND: "Làm task 4.3"
INPUT:
  - Task 4.1 hoàn thành
OUTPUT:
  - Subscribe các topics:
    - devices/{deviceId}/pump/control (QoS 1)
    - devices/{deviceId}/config (QoS 1)
    - devices/{deviceId}/mode/control (QoS 1)
  - Callback handlers:
    - handlePumpControl(): parse {"action": "on"|"off"|"toggle", "duration": 30}
    - handleConfig(): parse và lưu config mới
    - handleModeControl(): chuyển mode auto/manual
  - Validate JSON trước khi apply
  - Respond với status sau khi nhận command
VERIFY:
  - mosquitto_pub gửi command -> device thực hiện
  - Invalid JSON không crash
  - Pump ON/OFF từ MQTT
RULES: #MQTT(9.3) #JSON(23) #SECURITY(5)
```

---

## PHASE 5: LƯU TRỮ & CẤU HÌNH
**Mục tiêu:** Lưu settings vào flash, config WiFi qua web
**Thời gian:** 1 ngày

### TASK 5.1: Storage Manager
```yaml
COMMAND: "Làm task 5.1"
INPUT:
  - Cần lưu: WiFi credentials, MQTT config, thresholds, mode, schedule
OUTPUT:
  - include/storage.h
  - src/storage.cpp
  - Class StorageManager với:
    - begin()
    - saveConfig(Config&) / loadConfig(Config&)
    - saveWiFi(ssid, pass) / loadWiFi()
    - saveMqtt(broker, port) / loadMqtt()
    - factoryReset()
  - Struct Config với CRC verification
  - Sử dụng LittleFS (không dùng EEPROM deprecated)
VERIFY:
  - Settings giữ sau reboot
  - CRC detect corrupted data
  - Factory reset hoạt động
RULES: #NVS(18) #FS(25)
```

### TASK 5.2: WiFi Provisioning
```yaml
COMMAND: "Làm task 5.2"
INPUT:
  - Task 5.1 hoàn thành
OUTPUT:
  - Nếu không có WiFi saved -> bật SoftAP mode
  - SoftAP: SSID="TuoiCay_Setup", no password
  - Captive portal: http://192.168.4.1
  - Form nhập SSID/Password
  - Save và reboot
VERIFY:
  - Lần đầu boot -> AP mode
  - Config WiFi qua điện thoại
  - Reboot -> kết nối WiFi đã config
RULES: #WIFI(8.1) #SECURITY(5)
```

---

## PHASE 6: SCHEDULER & ADVANCED
**Mục tiêu:** Hẹn giờ tưới, NTP time sync
**Thời gian:** 2 ngày

### TASK 6.1: NTP Time Sync
```yaml
COMMAND: "Làm task 6.1"
INPUT:
  - WiFi connected
OUTPUT:
  - NTP sync khi boot và mỗi 6 giờ
  - Timezone Vietnam (UTC+7)
  - getFormattedTime() -> "HH:MM:SS"
  - getHour(), getMinute()
VERIFY:
  - Thời gian chính xác sau boot
  - Log: "[INF][TIME][sync] 14:30:45 UTC+7"
RULES: #TIME(12)
```

### TASK 6.2: Watering Scheduler
```yaml
COMMAND: "Làm task 6.2"
INPUT:
  - Task 6.1 hoàn thành
OUTPUT:
  - include/scheduler.h
  - src/scheduler.cpp
  - Cấu hình qua web:
    - Enable/disable schedule
    - Giờ tưới (ví dụ: 6:00 và 18:00)
    - Thời gian tưới (seconds)
  - Logic: Tới giờ -> bật pump -> tắt sau duration
  - Không tưới nếu đất đủ ẩm (check sensor)
VERIFY:
  - Pump bật đúng giờ đã cài
  - Skip nếu đất còn ướt
  - Log schedule events
RULES: #TIME(12) #ACTUATOR(15)
```

### TASK 6.3: OTA Update
```yaml
COMMAND: "Làm task 6.3"
INPUT:
  - Tất cả tasks trước hoàn thành
OUTPUT:
  - ArduinoOTA hoặc HTTP OTA
  - Password protection
  - Progress callback
  - Rollback support (nếu có dual partition)
VERIFY:
  - Upload firmware qua WiFi
  - Device không brick nếu upload fail
RULES: #OTA(10) #SECURITY(5)
```

---

## PHASE 7: TESTING & POLISH
**Mục tiêu:** Test toàn diện, fix bugs, optimize
**Thời gian:** 1 ngày

### TASK 7.1: Integration Test
```yaml
COMMAND: "Làm task 7.1"
INPUT:
  - Tất cả phases hoàn thành
OUTPUT:
  - Test scenarios:
    1. Boot từ power off -> WiFi connect -> MQTT connect -> Web accessible
    2. Đất khô -> Pump ON -> Đất ướt -> Pump OFF
    3. Schedule trigger -> Pump ON -> Duration -> Pump OFF
    4. WiFi mất -> Reconnect -> MQTT reconnect
    5. MQTT command -> Pump control -> Status publish
    6. Factory reset -> AP mode -> Reconfig
  - Fix any bugs found
VERIFY:
  - Tất cả scenarios pass
  - Không crash sau 24h run
RULES: #TEST(19)
```

### TASK 7.2: Documentation
```yaml
COMMAND: "Làm task 7.2"
INPUT:
  - Code hoàn thiện
OUTPUT:
  - README.md với hướng dẫn sử dụng
  - API documentation (HTTP + MQTT)
  - Troubleshooting guide
VERIFY:
  - Người mới có thể setup theo docs
RULES: #AIDEV(20)
```

================================================================================
## PROGRESS TRACKING
================================================================================

| Phase | Task | Status | Date |
|-------|------|--------|------|
| 1 | 1.1 Cấu hình dự án | ✅ | 2026-01-31 |
| 1 | 1.2 Safe state & Watchdog | ✅ | 2026-01-31 |
| 2 | 2.1 Sensor Driver | ✅ | 2026-01-31 |
| 2 | 2.2 Pump Driver | ✅ | 2026-01-31 |
| 2 | 2.3 Auto mode | ✅ | 2026-01-31 |
| 3 | 3.1 WiFi Manager | ⬜ | |
| 2 | 2.3 Auto mode | ⬜ | |
| 3 | 3.1 WiFi Manager | ⬜ | |
| 3 | 3.2 Web Server | ⬜ | |
| 3 | 3.3 Web Dashboard | ⬜ | |
| 4 | 4.1 MQTT Manager | ⬜ | |
| 4 | 4.2 MQTT Publish | ⬜ | |
| 4 | 4.3 MQTT Subscribe | ⬜ | |
| 5 | 5.1 Storage Manager | ⬜ | |
| 5 | 5.2 WiFi Provisioning | ⬜ | |
| 6 | 6.1 NTP Time | ⬜ | |
| 6 | 6.2 Scheduler | ⬜ | |
| 6 | 6.3 OTA Update | ⬜ | |
| 7 | 7.1 Integration Test | ⬜ | |
| 7 | 7.2 Documentation | ⬜ | |

**Legend:** ⬜ Not started | 🔄 In progress | ✅ Completed | ❌ Blocked

================================================================================
## AI INSTRUCTIONS
================================================================================

### Cách sử dụng file này:

1. **User command:** "Làm task X.Y"
   - AI đọc TASK X.Y trong file này
   - Thực hiện theo OUTPUT specification
   - Verify theo VERIFY criteria
   - Tuân thủ RULES references

2. **User command:** "Review task X.Y"
   - AI kiểm tra code đã viết
   - So sánh với OUTPUT và VERIFY
   - Đề xuất cải thiện

3. **User command:** "Tiếp tục"
   - AI tìm task tiếp theo chưa hoàn thành (⬜)
   - Thực hiện task đó

4. **User command:** "Status"
   - AI đọc PROGRESS TRACKING table
   - Report tiến độ tổng thể

### Code style:
- Tuân thủ rule.md nghiêm ngặt
- Mỗi file có header comment với @file, @brief, logic explanation
- Sử dụng LOG_xxx macros cho tất cả output
- Error handling với error codes
- Non-blocking code (no delay() in loop)

### Khi gặp vấn đề:
- Log chi tiết context
- Đề xuất solution
- Hỏi user nếu cần clarification về hardware

================================================================================
END OF KEHOACH.MD
================================================================================

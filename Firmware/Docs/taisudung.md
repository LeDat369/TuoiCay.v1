# HƯỚNG DẪN TÁI SỬ DỤNG CODE CHO DỰ ÁN MỚI
================================================================================
**Mục đích:** Hướng dẫn tái sử dụng các thành phần từ dự án TuoiCay cho dự án ESP8266/ESP32 mới
**Đối tượng:** Developer
**Cập nhật:** 2026-02-03
================================================================================

## MỤC LỤC
1. [Tổng quan tái sử dụng](#1-tổng-quan-tái-sử-dụng)
2. [Các thư viện có thể tái sử dụng](#2-các-thư-viện-có-thể-tái-sử-dụng)
3. [Các file cấu hình](#3-các-file-cấu-hình)
4. [Quy trình tái sử dụng](#4-quy-trình-tái-sử-dụng)
5. [Checklist tái sử dụng](#5-checklist-tái-sử-dụng)
6. [Ví dụ cụ thể](#6-ví-dụ-cụ-thể)

================================================================================
## 1. TỔNG QUAN TÁI SỬ DỤNG
================================================================================

### 1.1 CẤU TRÚC DỰ ÁN TUOICAY

```
TuoiCay/
├── include/           ❌ PROJECT-SPECIFIC (không tái sử dụng trực tiếp)
│   ├── config.h       → Phải tạo mới cho dự án
│   ├── error_codes.h  → Có thể tái sử dụng với sửa đổi prefix
│   ├── pins.h         → Phải tạo mới theo phần cứng
│   └── secrets.h      → Phải tạo mới (credentials)
│
├── lib/               ✅ REUSABLE LIBRARIES (có thể tái sử dụng)
│   ├── TuoiCay_Drivers/      → Tái sử dụng từng phần (sensor, pump)
│   ├── TuoiCay_Managers/     → Tái sử dụng toàn bộ (wifi, mqtt, ota, storage...)
│   └── TuoiCay_Utils/        → Tái sử dụng 100% (logger, crc)
│
└── src/               ❌ APPLICATION CODE (không tái sử dụng)
    └── main.cpp       → Logic nghiệp vụ riêng của TuoiCay
```

### 1.2 MỨC ĐỘ TÁI SỬ DỤNG

| Thành phần                | Tái sử dụng | Sửa đổi    | Ghi chú                                  |
|---------------------------|-------------|------------|------------------------------------------|
| **TuoiCay_Utils/**        | ✅ 100%     | Không cần  | Logger, CRC utils - universal            |
| **TuoiCay_Managers/**     | ✅ 95%      | Rename     | WiFi, MQTT, OTA, Storage, Time, Web      |
| **TuoiCay_Drivers/**      | ⚠️ 50%      | Tùy HW     | Tùy thuộc phần cứng dự án mới            |
| **include/**              | ❌ 10%      | Tạo mới    | Chỉ copy structure, fill mới             |
| **src/main.cpp**          | ❌ 0%       | Tạo mới    | Logic nghiệp vụ khác nhau                |

================================================================================
## 2. CÁC THƯ VIỆN CÓ THỂ TÁI SỬ DỤNG
================================================================================

### 2.1 ✅ TuoiCay_Utils (100% TÁI SỬ DỤNG - KHÔNG SỬA ĐỔI)

**Nội dung:**
- `logger.h` - Logging macros với format chuẩn
- `crc_utils.h` - CRC32 checksum utilities

**Cách tái sử dụng:**
```bash
# 1. Copy toàn bộ thư mục
cp -r lib/TuoiCay_Utils lib/MyProject_Utils

# 2. Rename trong library.json
# TuoiCay_Utils -> MyProject_Utils

# 3. Include trong code
#include <logger.h>
#include <crc_utils.h>
```

**KHÔNG CẦN SỬA ĐỔI GÌ - DÙNG NGAY!**

---

### 2.2 ✅ TuoiCay_Managers (95% TÁI SỬ DỤNG - CHỈ ĐỔI TÊN)

**Nội dung:**

| File                   | Chức năng                                          | Tái sử dụng |
|------------------------|----------------------------------------------------|-------------|
| `wifi_manager.*`       | Quản lý WiFi (connect, reconnect, backoff)        | ✅ 100%     |
| `mqtt_manager.*`       | Quản lý MQTT (connect, pub/sub, LWT, offline queue) |✅ 100%     |
| `ota_manager.*`        | OTA update (HTTP/HTTPS, rollback)                 | ✅ 100%     |
| `storage_manager.*`    | NVS storage (save/load config với CRC)            | ✅ 100%     |
| `time_manager.*`       | NTP time sync                                      | ✅ 100%     |
| `web_server.*`         | Web server cho config/monitoring                   | ✅ 95%      |
| `captive_portal.*`     | WiFi provisioning via AP                           | ✅ 95%      |
| `scheduler.*`          | Task scheduler                                     | ✅ 90%      |

**Cách tái sử dụng:**
```bash
# 1. Copy toàn bộ thư mục
cp -r lib/TuoiCay_Managers lib/MyProject_Managers

# 2. Rename trong library.json
# name: "TuoiCay_Managers" -> "MyProject_Managers"

# 3. Global find & replace trong toàn bộ file:
# TC_ERR_ -> MYPRJ_ERR_  (nếu dùng error codes riêng)
# Hoặc giữ nguyên TC_ERR_ nếu dùng chung error_codes.h
```

**CÁC FILE CẦN SỬA NHỎ:**

#### web_server.cpp/h
```cpp
// Sửa các API endpoint theo nghiệp vụ mới
// Ví dụ: TuoiCay có /api/pump, /api/moisture
// Dự án mới có thể có /api/led, /api/temp

// TuoiCay:
server.on("/api/pump", HTTP_GET, handleGetPumpStatus);

// Dự án mới:
server.on("/api/device", HTTP_GET, handleGetDeviceStatus);
```

#### scheduler.cpp/h
```cpp
// Sửa logic task scheduling theo nghiệp vụ
// TuoiCay: Check moisture -> auto pump
// Dự án mới: Check temp -> auto fan, etc.

// Chỉ sửa phần task logic, giữ nguyên cơ chế scheduler
```

---

### 2.3 ⚠️ TuoiCay_Drivers (50% TÁI SỬ DỤNG - TÙY PHẦN CỨNG)

**Nội dung:**
| File | Chức năng | Tái sử dụng |
|------|-----------|-------------|
| `pump_driver.*` | Điều khiển relay/pump | ⚠️ Nếu có pump/relay tương tự |
| `sensor_driver.*` | Đọc cảm biến độ ẩm đất | ⚠️ Nếu có sensor analog tương tự |

**Khi nào TÁI SỬ DỤNG:**
- ✅ Dự án có relay/pump → Dùng `pump_driver.*`
- ✅ Dự án có cảm biến analog → Dùng `sensor_driver.*` làm template
- ❌ Dự án không có HW tương tự → KHÔNG tái sử dụng, viết driver mới

**Cách tái sử dụng:**
```bash
# 1. Copy driver cần thiết
mkdir -p lib/MyProject_Drivers/src
cp lib/TuoiCay_Drivers/src/pump_driver.* lib/MyProject_Drivers/src/

# 2. Rename class và file
# PumpDriver -> RelayDriver / MotorDriver / ...
# pump_driver -> relay_driver / motor_driver / ...

# 3. Sửa logic trong driver theo HW mới
```

**QUAN TRỌNG:** Drivers phụ thuộc `pins.h` → Phải tạo mới cho dự án!

---

### 2.4 ❌ Application Code (KHÔNG TÁI SỬ DỤNG)

**Nội dung:**
- `src/main.cpp` - Logic nghiệp vụ TuoiCay (auto watering)

**Lý do KHÔNG tái sử dụng:**
- Logic nghiệp vụ riêng của từng dự án
- Main loop khác nhau
- State machine khác nhau

**Cách làm đúng:**
- Tham khảo cấu trúc tổ chức code
- Tham khảo cách init các managers
- Viết lại logic nghiệp vụ mới

================================================================================
## 3. CÁC FILE CẤU HÌNH
================================================================================

### 3.1 ❌ include/config.h (PHẢI TẠO MỚI)

**Nội dung cần sửa:**

| Thành phần | TuoiCay | Dự án mới |
|------------|---------|-----------|
| FW_NAME | "TuoiCay" | "YourProject" |
| DEVICE_TYPE | "TUOICAY_V1" | "YOURPRJ_V1" |
| DEVICE_PREFIX | "TC" | "YP" |
| Sensor intervals | 2000ms | Tùy dự án |
| Pump settings | `PUMP_MAX_RUNTIME_SEC` | Xóa nếu không có pump |
| Thresholds | `DEFAULT_THRESHOLD_DRY/WET` | Xóa hoặc thay logic mới |
| ADC calibration | `ADC_DRY_VALUE/WET_VALUE` | Calibrate lại theo sensor |

**Template:**
```cpp
// config.h for NEW PROJECT
#ifndef CONFIG_H
#define CONFIG_H

//=============================================================================
// FIRMWARE VERSION (SemVer)
//=============================================================================
#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    0
#define FW_VERSION_PATCH    0
#define FW_VERSION          "1.0.0"
#define FW_NAME             "MyProject"      // ← SỬA

//=============================================================================
// DEVICE IDENTIFICATION
//=============================================================================
#define DEVICE_TYPE         "MYPRJ_V1"       // ← SỬA
#define DEVICE_PREFIX       "MP"             // ← SỬA

//=============================================================================
// TIMING CONSTANTS - GIỮ NGUYÊN (best practices)
//=============================================================================
#define WDT_TIMEOUT_SEC         30
#define WIFI_CONNECT_TIMEOUT_MS 30000
#define MQTT_CONNECT_TIMEOUT_MS 10000
// ... (giữ nguyên các timeout chuẩn)

//=============================================================================
// PROJECT-SPECIFIC SETTINGS - SỬA THEO NGHIỆP VỤ
//=============================================================================
// Ví dụ: Smart Home
#define RELAY_MAX_RUNTIME_SEC   7200        // 2h
#define TEMP_READ_INTERVAL_MS   5000        // 5s
#define MQTT_PUBLISH_INTERVAL_MS 10000      // 10s

#endif // CONFIG_H
```

---

### 3.2 ⚠️ include/error_codes.h (CÓ THỂ TÁI SỬ DỤNG)

**Option 1: Tái sử dụng TOÀN BỘ (khuyến nghị)**
```cpp
// Giữ nguyên prefix TC_ERR_ cho đồng nhất
// Chỉ thêm error codes mới nếu cần

// TuoiCay đã có:
#define TC_ERR_OK               0
#define TC_ERR_WIFI_CONNECT_FAIL 1001
#define TC_ERR_MQTT_CONNECT_FAIL 2001
// ...

// Dự án mới thêm:
#define TC_ERR_TEMP_OUT_OF_RANGE 3010
#define TC_ERR_FAN_CONTROL_FAIL  6010
```

**Option 2: Đổi prefix (nếu muốn độc lập)**
```bash
# Find & replace toàn bộ
TC_ERR_ -> MYPRJ_ERR_

# Kết quả:
MYPRJ_ERR_OK
MYPRJ_ERR_WIFI_CONNECT_FAIL
# ...
```

**Khuyến nghị:** Dùng Option 1 - Giữ TC_ERR_ làm chuẩn chung cho tất cả dự án ESP32.

---

### 3.3 ❌ include/pins.h (PHẢI TẠO MỚI)

**Lý do:** Mỗi dự án có phần cứng khác nhau → GPIO mapping khác nhau

**Template:**
```cpp
// pins.h for NEW PROJECT
#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

//=============================================================================
// ESP8266 / ESP32 PIN MAPPING
//=============================================================================
// Ghi rõ board: NodeMCU / ESP32 DevKit / Custom PCB

// Ví dụ ESP32 DevKit:
// GPIO0-39 available
// Note: GPIO6-11 = Flash (DO NOT USE)
// Note: GPIO34-39 = Input only (no pullup)

//=============================================================================
// PROJECT-SPECIFIC PINS
//=============================================================================
#define PIN_RELAY_1         25          // GPIO25
#define PIN_RELAY_2         26          // GPIO26
#define PIN_LED_STATUS      2           // Built-in LED

#define PIN_TEMP_SENSOR     4           // GPIO4 (I2C SDA)
#define PIN_TEMP_SENSOR_SCL 5           // GPIO5 (I2C SCL)

#define PIN_BUTTON_RESET    0           // GPIO0 (BOOT button)

// Safe states
#define RELAY_ON            HIGH
#define RELAY_OFF           LOW

#endif // PINS_H
```

---

### 3.4 ❌ include/secrets.h (PHẢI TẠO MỚI)

**KHÔNG BAO GIỜ copy file này!** Mỗi dự án có credentials riêng.

**Cách làm:**
```bash
# 1. Copy template
cp include/secrets.h.example include/secrets.h

# 2. Fill thông tin mới
# WiFi SSID/Pass
# MQTT broker/user/pass
# OTA password
# API keys

# 3. Add vào .gitignore
echo "include/secrets.h" >> .gitignore
```

---

### 3.5 ⚠️ platformio.ini (SỬA MỘT PHẦN)

**Cần sửa:**
```ini
[env:nodemcuv2]              ; ← Board của dự án mới
platform = espressif8266     ; ← ESP8266 or espressif32
board = nodemcuv2            ; ← esp32dev, esp32-s3, etc.

upload_port = COM3           ; ← COM port mới

; Upload speed - tùy chip USB-UART
upload_speed = 460800        ; CH340: 460800, CP2102: 921600

lib_deps =                   ; ← Thêm/bớt lib theo dự án
    bblanchon/ArduinoJson@^7.0.0      ; Giữ
    knolleary/PubSubClient@^2.8       ; Giữ nếu dùng MQTT
    DHT sensor library                ; ← Thêm nếu dùng DHT
    Adafruit BMP280 Library           ; ← Thêm nếu dùng BMP280
```

**Giữ nguyên:**
- Build flags: `-I include`
- Logging level: `-D LOG_LEVEL=3`
- ArduinoJson, PubSubClient (nếu dùng MQTT)
- OTA settings

================================================================================
## 4. QUY TRÌNH TÁI SỬ DỤNG
================================================================================

### 4.1 CHUẨN BỊ DỰ ÁN MỚI

```bash
# 1. Tạo project PlatformIO mới
pio project init --board <your-board>

# 2. Tạo cấu trúc thư mục
mkdir -p include lib/MyProject_Utils lib/MyProject_Managers lib/MyProject_Drivers test Docs
```

---

### 4.2 COPY CÁC LIBRARIES (THAO TÁC TỪNG BƯỚC)

#### Bước 1: Copy Utils (100% ready-to-use)
```bash
# Copy toàn bộ
cp -r <TuoiCay>/lib/TuoiCay_Utils lib/MyProject_Utils

# Sửa library.json
# "name": "TuoiCay_Utils" -> "MyProject_Utils"
```

#### Bước 2: Copy Managers (95% ready-to-use)
```bash
# Copy toàn bộ
cp -r <TuoiCay>/lib/TuoiCay_Managers lib/MyProject_Managers

# Sửa library.json
# "name": "TuoiCay_Managers" -> "MyProject_Managers"

# (Optional) Global find & replace error prefix
# TC_ERR_ -> MYPRJ_ERR_  (nếu muốn prefix riêng)
```

#### Bước 3: Copy Drivers (CHỈ KHI CẦN)
```bash
# Chỉ copy drivers phù hợp với HW mới
# Ví dụ: Dự án có relay -> copy pump_driver làm template

cp <TuoiCay>/lib/TuoiCay_Drivers/src/pump_driver.* lib/MyProject_Drivers/src/

# Rename class & file
# PumpDriver -> RelayDriver
# Sửa logic theo HW mới
```

---

### 4.3 TẠO CÁC FILE CẤU HÌNH

#### Bước 1: config.h
```bash
# Copy structure, fill mới
cp <TuoiCay>/include/config.h include/config.h

# SỬA:
# - FW_NAME, DEVICE_TYPE, DEVICE_PREFIX
# - Intervals, timeouts theo dự án
# - Xóa settings không dùng (PUMP_, THRESHOLD_, ADC_)
# - Thêm settings mới (RELAY_, LED_, etc.)
```

#### Bước 2: error_codes.h
```bash
# Option 1: Copy nguyên
cp <TuoiCay>/include/error_codes.h include/error_codes.h

# Option 2: Copy + đổi prefix
cp <TuoiCay>/include/error_codes.h include/error_codes.h
# Find & replace: TC_ERR_ -> MYPRJ_ERR_

# Thêm error codes mới nếu cần
```

#### Bước 3: pins.h
```bash
# Tạo mới từ đầu theo schematic HW
nano include/pins.h

# Tham khảo structure từ TuoiCay:
# - Pin mapping comments
# - Safe state defines
# - Logic level defines (ON/OFF)
```

#### Bước 4: secrets.h
```bash
# Copy template
cp <TuoiCay>/include/secrets.h.example include/secrets.h

# Fill credentials mới
# Add vào .gitignore
```

#### Bước 5: platformio.ini
```bash
# Copy base
cp <TuoiCay>/platformio.ini platformio.ini

# SỬA:
# - [env:xxx] board
# - upload_port COM
# - lib_deps theo dự án
```

---

### 4.4 VIẾT MAIN.CPP MỚI

```cpp
// src/main.cpp - Tham khảo structure TuoiCay, viết lại logic

#include <Arduino.h>
#include <config.h>
#include <error_codes.h>
#include <pins.h>
#include <secrets.h>

// Managers - TÁI SỬ DỤNG
#include <logger.h>
#include <wifi_manager.h>
#include <mqtt_manager.h>
#include <ota_manager.h>
#include <storage_manager.h>
#include <time_manager.h>

// Drivers - VIẾT MỚI hoặc TÁI SỬ DỤNG
#include <your_driver.h>

// Global objects
WiFiManager wifiMgr;
MQTTManager mqttMgr;
// ...

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    LOG_INF("SYSTEM", "setup", "FW:%s v%s", FW_NAME, FW_VERSION);
    
    // Init GPIO
    pinMode(PIN_RELAY_1, OUTPUT);
    digitalWrite(PIN_RELAY_1, RELAY_OFF);
    
    // Init managers (GIỮ NGUYÊN PATTERN)
    wifiMgr.begin();
    mqttMgr.begin();
    
    // Init drivers (SỬA THEO DỰ ÁN)
    // yourDriver.begin();
}

void loop() {
    // Update managers (GIỮ NGUYÊN)
    wifiMgr.loop();
    mqttMgr.loop();
    
    // Business logic (VIẾT MỚI)
    // ...
}
```

---

### 4.5 TEST & VERIFY

```bash
# 1. Compile
pio run

# 2. Upload
pio run -t upload

# 3. Monitor
pio device monitor

# 4. Kiểm tra logs
# [INF][SYSTEM][setup] FW:MyProject v1.0.0
# [INF][WIFI][connect] Connecting to MySSID...
# [INF][WIFI][connect] Connected, IP: 192.168.1.100
# [INF][MQTT][connect] Connected to broker
```

================================================================================
## 5. CHECKLIST TÁI SỬ DỤNG
================================================================================

### ✅ TRƯỚC KHI BẮT ĐẦU

- [ ] Xác định board: ESP8266 hay ESP32?
- [ ] Vẽ schematic/pinout phần cứng mới
- [ ] List các managers cần thiết: WiFi? MQTT? OTA? Storage? Web?
- [ ] List các drivers cần thiết: Relay? Sensor? Motor? LED?
- [ ] Quyết định error code prefix: Giữ TC_ERR_ hay đổi mới?

### ✅ COPY LIBRARIES

- [ ] ✅ Copy `TuoiCay_Utils` → `MyProject_Utils` (100%)
- [ ] ✅ Copy `TuoiCay_Managers` → `MyProject_Managers` (95%)
- [ ] ⚠️ Copy drivers cần thiết từ `TuoiCay_Drivers` (optional)
- [ ] Rename library names trong `library.json`
- [ ] (Optional) Find & replace error prefix toàn bộ libraries

### ✅ TẠO CONFIG FILES

- [ ] ❌ Tạo mới `include/config.h`:
  - [ ] Sửa FW_NAME, DEVICE_TYPE, DEVICE_PREFIX
  - [ ] Sửa intervals, timeouts theo dự án
  - [ ] Xóa settings không dùng
  - [ ] Thêm settings mới
- [ ] ⚠️ Copy `include/error_codes.h` (giữ nguyên hoặc đổi prefix)
- [ ] ❌ Tạo mới `include/pins.h` theo schematic
- [ ] ❌ Tạo mới `include/secrets.h` từ template
- [ ] ⚠️ Copy + sửa `platformio.ini`:
  - [ ] Board, platform
  - [ ] Upload port, speed
  - [ ] lib_deps

### ✅ VIẾT APPLICATION

- [ ] ❌ Viết `src/main.cpp` mới:
  - [ ] Include headers
  - [ ] Init managers
  - [ ] Init drivers
  - [ ] Business logic
- [ ] Tham khảo structure TuoiCay:
  - [ ] Setup pattern
  - [ ] Loop pattern
  - [ ] Error handling
  - [ ] Logging style

### ✅ BUILD & TEST

- [ ] Compile thành công
- [ ] Upload thành công
- [ ] Boot thành công, không crash
- [ ] WiFi connect OK
- [ ] MQTT connect OK (nếu dùng)
- [ ] OTA works (nếu dùng)
- [ ] Drivers hoạt động đúng
- [ ] Logic nghiệp vụ đúng

### ✅ CODE QUALITY

- [ ] Follow naming convention (rule.md #CORE 1.3)
- [ ] Error handling đầy đủ (rule.md #ERROR 6)
- [ ] Logging đầy đủ (rule.md #LOG 7)
- [ ] No memory leak (chạy 1h, heap stable)
- [ ] Watchdog không reset
- [ ] Code review theo rule.md

### ✅ DOCUMENTATION

- [ ] Update README.md cho dự án mới
- [ ] Update hardware schematic/pinout
- [ ] Document API nếu có web server
- [ ] Ghi lại calibration values (ADC, sensor, etc.)

================================================================================
## 6. VÍ DỤ CỤ THỂ
================================================================================

### VÍ DỤ 1: Smart Home (Relay + DHT22)

**Yêu cầu:**
- ESP32 DevKit
- 4 relay điều khiển đèn/quạt
- DHT22 đo nhiệt độ/độ ẩm
- WiFi + MQTT
- Web UI
- OTA

**Tái sử dụng:**
```
✅ 100% TuoiCay_Utils
✅ 100% TuoiCay_Managers (WiFi, MQTT, OTA, Storage, Time, Web)
⚠️ 50% TuoiCay_Drivers:
   - Copy pump_driver → relay_driver (điều khiển relay)
   - Viết mới dht_driver (đọc DHT22)
❌ Viết mới main.cpp với logic smart home
❌ Tạo mới config.h, pins.h, secrets.h
```

**Thời gian ước tính:**
- Copy libraries: 10 phút
- Config files: 20 phút
- Sửa relay_driver: 30 phút
- Viết dht_driver: 1 giờ
- Viết main.cpp: 2 giờ
- Test & debug: 1 giờ
**TỔNG: ~5 giờ** (so với viết từ đầu: ~20 giờ) → **Tiết kiệm 75%**

---

### VÍ DỤ 2: Weather Station (Sensors only, no actuators)

**Yêu cầu:**
- ESP32 DevKit
- BMP280 (nhiệt độ, khí áp)
- DHT22 (độ ẩm)
- Rain sensor
- WiFi + MQTT
- Deep sleep (chạy pin)

**Tái sử dụng:**
```
✅ 100% TuoiCay_Utils
✅ 95% TuoiCay_Managers:
   - WiFi, MQTT, OTA, Storage, Time (100%)
   - Web server: KHÔNG dùng (deep sleep)
   - Scheduler: KHÔNG dùng (deep sleep)
⚠️ 30% TuoiCay_Drivers:
   - Copy sensor_driver làm template
   - Viết mới bmp280_driver, dht_driver, rain_driver
❌ Viết mới main.cpp với deep sleep logic
❌ Tạo mới config.h (thêm deep sleep settings), pins.h, secrets.h
```

**Thời gian ước tính:**
- Copy libraries: 10 phút
- Config files: 20 phút
- Viết 3 drivers: 3 giờ
- Viết main.cpp + deep sleep: 2 giờ
- Test: 1 giờ
**TỔNG: ~6.5 giờ** (viết từ đầu: ~25 giờ) → **Tiết kiệm 74%**

---

### VÍ DỤ 3: BLE Beacon (No WiFi/MQTT)

**Yêu cầu:**
- ESP32 DevKit
- BLE advertising only
- Sensor data via BLE
- Low power

**Tái sử dụng:**
```
✅ 100% TuoiCay_Utils (logger vẫn dùng cho debug)
❌ KHÔNG dùng TuoiCay_Managers:
   - Không WiFi, MQTT, OTA, Web
   - Chỉ dùng StorageManager (NVS)
⚠️ 30% TuoiCay_Drivers (sensor drivers làm template)
❌ Viết mới BLE stack
❌ Viết mới main.cpp
❌ Tạo mới config.h, pins.h
```

**Thời gian ước tính:**
- Copy Utils + StorageManager: 10 phút
- Config files: 15 phút
- Viết BLE stack: 4 giờ
- Sensor drivers: 2 giờ
- Main logic: 2 giờ
**TỔNG: ~8.5 giờ** (viết từ đầu: ~30 giờ) → **Tiết kiệm 72%**

================================================================================
## 7. LƯU Ý QUAN TRỌNG
================================================================================

### 7.1 ĐỌC KỸ rule.md

**Trước khi tái sử dụng, ĐỌC:**
- `rule.md` - Quy tắc phát triển firmware ESP32
- Đảm bảo dự án mới tuân thủ các nguyên tắc:
  - Error handling (#ERROR 6)
  - Logging (#LOG 7)
  - Memory management (#MEMORY 3)
  - Concurrency (#THREAD 4)
  - Security (#SECURITY 5)

### 7.2 KHÔNG COPY MÙ QUÁNG

**CẦN SUY NGHĨ:**
- Dự án mới có cần MQTT không? → Có thể bỏ MQTTManager
- Có cần Web UI không? → Có thể bỏ WebServer
- Có cần OTA không? → Production nên có, prototype có thể bỏ
- Phần cứng có gì khác? → Viết driver mới
- Logic nghiệp vụ khác thế nào? → Viết main.cpp hoàn toàn mới

### 7.3 ƯU TIÊN TÁI SỬ DỤNG

**Thứ tự ưu tiên:**
1. ✅ **Utils** - LUÔN LUÔN tái sử dụng (logger, crc)
2. ✅ **Managers** - Tái sử dụng TỐI ĐA (wifi, mqtt, ota, storage, time)
3. ⚠️ **Drivers** - Chỉ tái sử dụng khi HW tương tự
4. ❌ **Application** - KHÔNG BAO GIỜ copy, viết mới

### 7.4 KHI NÀO KHÔNG NÊN TÁI SỬ DỤNG

**VIẾT LẠI TỪ ĐẦU NẾU:**
- Dự án quá khác biệt (ví dụ: TuoiCay là IoT, dự án mới là BLE mesh)
- Yêu cầu performance/memory khác xa (ví dụ: TuoiCay chạy liên tục, dự án mới chạy pin)
- Platform khác (TuoiCay ESP8266, dự án mới STM32)
- Đội ngũ khác, coding style khác

**NHƯNG:** Vẫn nên tham khảo **architecture pattern** và **best practices** từ TuoiCay!

================================================================================
## 8. TÀI LIỆU THAM KHẢO
================================================================================

**Trong dự án TuoiCay:**
- `Docs/rule.md` - Quy tắc phát triển (BẮT BUỘC đọc)
- `Docs/API.md` - API documentation
- `Docs/HARDWARE.md` - Hardware schematic
- `Docs/Pinout.md` - GPIO pinout
- `README.md` - Project overview

**PlatformIO:**
- https://docs.platformio.org/en/latest/librarymanager/creating.html
- https://docs.platformio.org/en/latest/projectconf/index.html

**ESP32/ESP8266:**
- https://docs.espressif.com/projects/esp-idf/en/latest/
- https://arduino-esp8266.readthedocs.io/

================================================================================
## 9. HỖ TRỢ & FEEDBACK
================================================================================

**Nếu gặp vấn đề khi tái sử dụng:**
1. Đọc lại `rule.md` section tương ứng
2. Check logs: `[ERR]` messages
3. Check error codes trong `error_codes.h`
4. Debug với `LOG_DBG` level

**Cải thiện tài liệu này:**
- Nếu phát hiện bước nào khó hiểu → Bổ sung
- Nếu có case study mới → Thêm vào section 6
- Nếu có pitfall mới → Thêm vào section 7

================================================================================
KẾT LUẬN
================================================================================

**Lợi ích tái sử dụng:**
- ✅ Tiết kiệm 70-80% thời gian phát triển
- ✅ Code đã được test, ổn định
- ✅ Tuân thủ best practices (rule.md)
- ✅ Dễ bảo trì, mở rộng
- ✅ Đồng nhất architecture giữa các dự án

**Nguyên tắc vàng:**
> "Tái sử dụng MANAGERS & UTILS, viết mới DRIVERS & APPLICATION"

**Checklist cuối cùng:**
- [ ] Đã copy đúng libraries cần thiết
- [ ] Đã tạo mới các config files
- [ ] Đã viết application logic mới
- [ ] Đã test đầy đủ
- [ ] Đã tuân thủ rule.md
- [ ] Code review OK

**Chúc bạn tái sử dụng thành công! 🚀**

================================================================================
END OF DOCUMENT
================================================================================

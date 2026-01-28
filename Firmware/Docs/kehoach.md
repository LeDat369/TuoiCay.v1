# Kế Hoạch Viết Firmware — Hệ Thống Tưới Cây Tự Động

## Mục tiêu
Viết firmware cho ESP8266 thực hiện 2 tính năng chính:
- Chế độ tưới tự động (theo độ ẩm đất và ngưỡng cấu hình).
- Chế độ tưới thủ công qua giao diện web (bật/tắt bơm, điều chỉnh thời gian/pwm).

Tài liệu này dựa trên `Docs/rule.md` (quy tắc lập trình) và `Docs/Pinout.md` (phần cứng).

---

## Yêu cầu chức năng (Functional Requirements)
1. Đọc dữ liệu từ 2 cảm biến độ ẩm:
   - Sensor1: digital -> `D5` (GPIO14)
   - Sensor2: digital `D1` (GPIO5) + analog `A0` (ADC) để đo giá trị chi tiết
2. Điều khiển máy bơm 5V thông qua MOSFET gate trên `D6` (GPIO12).
3. Chế độ tự động:
   - Đọc A0 định kỳ (configurable interval).
   - Nếu độ ẩm < ngưỡng thấp → bật bơm đến khi đạt ngưỡng cao hoặc timeout safety.
   - Lưu trạng thái và thống kê (lần tưới, thời gian) vào NVS/LittleFS.
4. Chế độ thủ công qua web:
   - Giao diện web đơn giản cho phép: bật/tắt bơm, đặt thời gian tưới, bật chế độ PWM (nếu cần), xem giá trị cảm biến, xem log/trạng thái.
   - API REST (JSON) cho các lệnh cơ bản (GET trạng thái, POST bật/tắt, POST set thresholds).
5. An toàn:
   - Timeout tối đa cho mỗi lần tưới (configurable).
   - Kiểm tra nguồn (nếu có sensor dòng) / chống quá dòng theo phần cứng.
6. Cấu hình:
   - Ngưỡng ẩm thấp/caothấp, thời gian tưới tối đa, interval đọc sensor, WiFi SSID/password, chế độ auto on/off.
   - Lưu cấu hình trên NVS.

---

## Yêu cầu phi chức năng (Non-functional)
- Tôn trọng quy tắc trong `rule.md`: KISS, DRY, tên hàm/file theo chuẩn, mutex/queue cho chia sẻ dữ liệu.
- Ổn định, tiêu thụ RAM/Flash tối ưu cho ESP8266.
- Khả năng gỡ lỗi qua Serial logs và web status page.
- Khả năng nâng cấp sau (OTA) — optional, nếu có bộ nhớ và thời gian.

---

## Ánh xạ phần cứng (từ `Pinout.md`)
- `D6 (GPIO12)` → MOSFET Gate (10k pulldown)
- `Pump +` → +5V
- `Pump -` → MOSFET Drain
- `MOSFET Source` → GND chung
- `Sensor1 DOUT` → `D5 (GPIO14)` (digital)
- `Sensor2 DOUT` → `D1 (GPIO5)` (digital)
- `Sensor2 AOUT` → `A0 (ADC)` (analog, nếu board là NodeMCU có divider tích hợp cho 3.3V; nếu không, dùng mạch chia R1=24k/R2=10k + 100nF decoupling)
- `3.3V` → Sensor VCC (nếu sensor yêu cầu)
- `GND` → Mass chung

---

## Kiến trúc firmware (modules)
Thiết kế modular theo `rule.md`:

- `main.c`
  - Khởi tạo hệ thống, load config, tạo tasks và khởi động web server.

- `hw_io.c/.h`
  - Abstraction cho GPIO, ADC, MOSFET control, debounce/ISR nếu cần.
  - Public API: `hw_init()`, `hw_set_pump(bool)`, `hw_set_pwm(uint16_t)`, `hw_read_adc()`, `hw_read_digital(pin)`.

- `sensor.c/.h`
  - Đọc và lọc sensor (moving average, low-pass), chuyển đổi ADC->voltage->moisture%.
  - Public API: `sensor_init()`, `sensor_read_all(sensor_data_t*)`.

- `control.c/.h`
  - Logic điều khiển tự động: state machine (IDLE, WATERING, COOLDOWN), thresholds, timeout safety.
  - Public API: `control_start_auto()`, `control_stop_auto()`, `control_manual_on(duration)`.

- `webserver.c/.h`
  - Giao diện web + REST API. Xử lý các route: `/status`, `/sensors`, `/control`, `/config`.
  - Sử dụng `ESP8266WebServer` hoặc `ESPAsyncWebServer` (tùy thư viện); giữ implementation đơn giản.

- `storage.c/.h`
  - Lưu cấu hình và logs vào NVS hoặc LittleFS (tuỳ chọn). API: `storage_load_config()`, `storage_save_config()`.

- `utils.c/.h`
  - Hàm trợ giúp, map/convert, safe math, debounce, timing helpers.

Quy tắc file/hàm theo `rule.md` (snake_case, include guard, extern C,..).

---

## Giao tiếp nội bộ
- Sử dụng FreeRTOS queues để truyền sự kiện sensor → controller → web.
- Shared state (pump status, last_sensor) bảo vệ bằng mutex.
- Event group để đồng bộ hành vi auto/manual khi cần.

---

## API Web (gợi ý)
- GET /status
  - Trả JSON: {"pump": "on"/"off", "mode": "auto"/"manual", "last_moisture": 512}
- GET /sensors
  - Trả chi tiết sensor values
- POST /control
  - Payload {"action":"on","duration_s":30} hoặc {"action":"off"}
- POST /config
  - Payload {"moisture_low":300,"moisture_high":600,"max_duration_s":60}

Bảo mật: nếu cần, thêm basic auth hoặc token đơn giản.

---

## Lộ trình công việc & Milestones (gợi ý thời gian phát triển)
Giả định: 1 dev, làm full-time. Thời gian ước lượng có thể điều chỉnh.

1. Chuẩn bị & thiết lập (0.5 ngày)
   - Kiểm tra phần cứng, xác nhận loại board (NodeMCU hay ESP-12).
   - Thiết lập project PlatformIO/Arduino hoặc ESP-IDF.

2. Module cơ bản & hw abstraction (1 ngày)
   - `hw_io` + đọc ADC, digital, drive MOSFET.
   - Test đơn giản: đọc sensor, bật/tắt pump bằng CLI/serial.

3. Sensor & control logic (1.5 ngày)
   - Implement `sensor.c` và `control.c` (state machine, thresholds, timeout).
   - Test local: mô phỏng giá trị sensor, confirm auto behavior.

4. Webserver & API (1.5 ngày)
   - Implement web pages + REST API, endpoints như trên.
   - Test bằng trình duyệt / curl.

5. Storage & config persistence (0.5 ngày)
   - Lưu cấu hình vào NVS/LittleFS, nạp lại khi khởi động.

6. Tối ưu, safety & tests (0.5–1 ngày)
   - Thêm decoupling, diode protection, validate edge cases.
   - Test on-device, đo dòng bơm, test timeout.

7. Documentation & polish (0.5 ngày)
   - Cập nhật `Docs/Pinout.md`, README, hướng dẫn nối dây và test.

Tổng: ~6–7 ngày làm việc (tùy thuộc hardware và test iterations).

---

## Kịch bản test (Acceptance tests)
- Auto mode: đặt moisture thấp, xác nhận bơm bật đến khi đạt ngưỡng high hoặc timeout.
- Manual mode: bật bơm từ web, xác nhận thời gian chạy chính xác.
- Power cycle: cấu hình lưu và khôi phục sau reboot.
- ADC safety: với board trần, kiểm tra không vượt quá 1.0V vào A0.
- Nhiễu: kiểm tra stability khi bơm chạy (nhiễu lên ADC), đảm bảo lọc bằng RC/tụ.

---

## Công cụ & thư viện gợi ý
- PlatformIO hoặc Arduino IDE (ESP8266 core) — nhanh để phát triển
- `ESP8266WebServer` (đơn giản) hoặc `ESPAsyncWebServer` + `ESPAsyncTCP` (hiệu năng tốt hơn)
- NVS hoặc LittleFS cho storage
- Các lib ADC/driver cơ bản nếu cần

---

## Checklist coding (tuân thủ `rule.md`)
- Tên file/hàm theo chuẩn
- Mỗi module có header + source
- Mutex cho shared state
- Kiểm tra return của mọi hàm hệ thống
- Comment bằng tiếng Việt không dấu giải thích WHY

---

## Bước tiếp theo (tôi có thể làm ngay)
- Tôi có thể tạo skeleton project (cấu trúc thư mục + template module files) và push vào repo.
- Hoặc tôi tạo hết `kehoach.md` này vào `Docs` (đã làm) và tiếp tục scaffold code.


---

## Phases chi tiết cho phần Firmware (không bao gồm hardware setup)

Ghi chú: giả định phần cứng đã được hoàn thiện và nối đúng (bạn đã đảm nhận phần này). Kế hoạch này tập trung hoàn toàn vào phát triển firmware và test phần mềm trên thiết bị đã lắp.

### Phase 0 - Môi trường phát triển & xác nhận assumptions (0.5 ngày)
- Thiết lập project (PlatformIO hoặc ESP-IDF) và template build.
- Xác nhận board target trong code (NodeMCU vs ESP-12) để chọn đúng mapping `A0` trong config.
- Tạo cấu trúc thư mục + `config.h` mẫu (các macro pin và constants).
- Acceptance: project build thành công trên target, serial console hoạt động.

### Phase 1 - `hw_io` module (abstraction) (1 ngày)
- Implement API: `hw_init()`, `hw_set_pump(bool)`, `hw_set_pwm(uint16_t)`, `hw_read_adc_raw()`, `hw_read_digital(gpio)`.
- Giữ implementation độc lập với chi tiết phần cứng (dùng macros từ `config.h`).
- Viết unit-test nhỏ / test script qua serial để gọi từng API thủ công.
- Acceptance: hàm API trả giá trị hợp lệ; build và chạy trên board, serial commands hoạt động.

### Phase 2 - ADC handling & calibration (0.5 ngày)
- Implement hàm `adc_raw_to_voltage()` và `adc_voltage_to_sensor_vin()` (phụ thuộc config divider_ratio).
- Thêm API calibration: `hw_calibrate_adc(scale)` để điều chỉnh offset/scale nếu cần.
- Test bằng cách gửi giá trị demo qua serial hoặc đọc ADC thực (phần cứng chuẩn bị sẵn).
- Acceptance: conversion hoạt động, có thể điều chỉnh hệ số scale từ serial.

### Phase 3 - `sensor` module + filtering (1 ngày)
- Implement: `sensor_init()`, `sensor_read_all(sensor_data_t*)`, áp filter (moving average hoặc median), debounce logic cho digital inputs.
- Cung cấp API đọc: `sensor_get_moisture_percent(sensor_id)`.
- Test: chạy script mô phỏng giá trị ADC, kiểm tra filter giảm nhiễu, digital debounce.
- Acceptance: giá trị ổn định, API trả đúng range và filter configurable.

### Phase 4 - Pump control logic & safety APIs (0.5–1 ngày)
- Implement low-level safe APIs: `pump_on()`, `pump_off()`, `pump_on_for(duration_s)`; đảm bảo non-blocking (sử dụng timer/task).
- Implement safety timeout enforcement at hw layer if pump left on too long.
- Test: script bật/tắt pump, kiểm tra timeout auto-off.
- Acceptance: pump_on_for không block, timeout chính xác.

### Phase 5 - Control state machine (auto mode) (1–1.5 ngày)
- Implement `control.c` state machine: IDLE, WATERING, COOLDOWN, ERROR.
- Logic: đọc sensor data, so sánh thresholds, kích hoạt pump via pump APIs, tính toán retry/cooldown.
- Sử dụng queue/event group cho events; shared state protected bởi mutex.
- Test: unit tests và on-device simulation (inject sensor values via serial) để validate transitions.
- Acceptance: state transitions đúng, timeout + safety hoạt động, logs rõ ràng.

### Phase 6 - Persistence (config + logs) (0.5 ngày)
- Implement `storage.c` để load/save cấu hình (thresholds, max_duration, mode) bằng NVS hoặc LittleFS.
- API: `storage_load_config()`, `storage_save_config()`, `storage_append_event()`.
- Test: change config via serial command, save, reboot simulated (restart app), verify reload.
- Acceptance: config persist and restored.

### Phase 7 - Integration & automated tests (1 ngày)
- Tạo test harness: serial command set để inject sensor values, read state, trigger manual watering.
- Chạy kịch bản tự động: simulate periodic readings, trigger auto watering, verify logs and states.
- Kiểm tra memory usage, stack watermarks, và lỗi runtime.
- Acceptance: hệ thống chạy qua các kịch bản mà không crash, memory leak nhỏ hoặc không có.

### Phase 8 - Documentation & dev checklist (0.5 ngày)
- Cập nhật `Docs/kehoach.md` (phiên bản firmware-only), `README.md` với cách build, serial commands cho test, API list.
- Tạo test checklist để kỹ thuật viên chạy: bước chạy, lệnh serial, expected outputs.
- Acceptance: dev khác có thể build và chạy test theo checklist.

---

**Created**: 27/01/2026

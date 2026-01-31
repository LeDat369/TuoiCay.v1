# 🌱 TuoiCay v1.0 - Hệ thống tưới cây tự động

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP8266-orange)](https://platformio.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-1.0.0-blue.svg)](CHANGELOG.md)

## Tổng quan

**TuoiCay** là hệ thống tưới cây tự động thông minh dựa trên ESP8266 (NodeMCU), hỗ trợ:

- 🌡️ **Đo độ ẩm đất** qua 2 cảm biến
- 💧 **Tự động tưới** theo ngưỡng cấu hình
- 📱 **Web Dashboard** điều khiển qua trình duyệt
- 🌐 **MQTT** giám sát và điều khiển từ xa
- ⏰ **Lập lịch tưới** theo giờ (NTP sync)
- 🔄 **OTA Update** cập nhật firmware qua WiFi
- 🔒 **Captive Portal** cấu hình WiFi dễ dàng

## Tính năng

| Tính năng | Mô tả |
|-----------|-------|
| **Auto Watering** | Tự động bật/tắt bơm theo độ ẩm |
| **Dual Sensors** | 2 cảm biến độ ẩm cho độ chính xác cao |
| **Web Control** | Dashboard đẹp, responsive |
| **MQTT Integration** | Tích hợp Home Assistant, Node-RED |
| **Scheduler** | Lập lịch tưới theo giờ (tối đa 4 lịch) |
| **OTA Updates** | Cập nhật firmware qua WiFi |
| **Safety Features** | Watchdog, pump timeout, boot safe |

## Cài đặt nhanh

### 1. Clone repository

```bash
git clone https://github.com/example/tuoicay.git
cd tuoicay/Firmware
```

### 2. Cấu hình WiFi

Sao chép file secrets:

```bash
cp include/secrets.h.example include/secrets.h
```

Chỉnh sửa `include/secrets.h`:

```cpp
#define WIFI_SSID     "YourWiFiName"
#define WIFI_PASSWORD "YourWiFiPassword"
#define MQTT_SERVER   "192.168.1.100"
#define MQTT_PORT     1883
```

### 3. Build và Upload

```bash
# Cài đặt PlatformIO
pip install platformio

# Build
platformio run

# Upload qua USB
platformio run --target upload

# Upload qua OTA (sau lần đầu)
platformio run --target upload --upload-port <device-ip>
```

## Cấu trúc thư mục

```
Firmware/
├── src/
│   └── main.cpp              # Entry point
├── include/
│   ├── config.h              # Cấu hình hệ thống
│   ├── pins.h                # Định nghĩa chân GPIO
│   ├── error_codes.h         # Mã lỗi
│   └── secrets.h             # WiFi/MQTT credentials
├── lib/
│   ├── TuoiCay_Drivers/      # Hardware drivers
│   │   ├── sensor_driver.*   # Cảm biến độ ẩm
│   │   └── pump_driver.*     # Điều khiển bơm
│   ├── TuoiCay_Managers/     # System managers
│   │   ├── wifi_manager.*    # Quản lý WiFi
│   │   ├── mqtt_manager.*    # MQTT client
│   │   ├── web_server.*      # HTTP server
│   │   ├── storage_manager.* # LittleFS storage
│   │   ├── ota_manager.*     # OTA updates
│   │   ├── time_manager.*    # NTP time sync
│   │   ├── scheduler.*       # Watering scheduler
│   │   └── captive_portal.*  # WiFi provisioning
│   └── TuoiCay_Utils/        # Utilities
│       └── logger.h          # Logging system
├── docs/
│   ├── API.md                # API documentation
│   ├── HARDWARE.md           # Hardware guide
│   └── USER_GUIDE.md         # User manual
├── platformio.ini            # PlatformIO config
└── README.md                 # This file
```

## Phần cứng

### Linh kiện cần thiết

- NodeMCU ESP8266 v2
- Cảm biến độ ẩm đất capacitive (x2)
- Module Relay 5V
- Máy bơm mini DC 5V
- Nguồn 5V 2A

### Sơ đồ kết nối

```
NodeMCU          Peripheral
────────         ──────────
A0        ←───── Sensor 1 (Analog)
D1 (GPIO5)←───── Sensor 2 (Digital)
D2 (GPIO4)────→  Relay IN
Vin       ────→  Relay VCC, Pump +
GND       ────→  Relay GND, Pump -
```

📖 Xem chi tiết: [HARDWARE.md](docs/HARDWARE.md)

## API

### REST API

```bash
# Lấy trạng thái
curl http://<ip>/api/status

# Bật bơm
curl -X POST http://<ip>/api/pump -d '{"action":"on"}'

# Đổi chế độ
curl -X POST http://<ip>/api/mode -d '{"mode":"auto"}'
```

### MQTT Topics

| Topic | Direction | Description |
|-------|-----------|-------------|
| `devices/{id}/sensor/data` | Publish | Dữ liệu cảm biến |
| `devices/{id}/pump/status` | Publish | Trạng thái bơm |
| `devices/{id}/pump/control` | Subscribe | Điều khiển bơm |
| `devices/{id}/mode/control` | Subscribe | Đổi chế độ |

📖 Xem chi tiết: [API.md](docs/API.md)

## Sử dụng

### Web Dashboard

Truy cập `http://<device-ip>/` để mở dashboard:

- Xem độ ẩm realtime
- Bật/tắt bơm thủ công
- Chuyển đổi chế độ AUTO/MANUAL
- Cấu hình ngưỡng tưới

### Captive Portal

Khi cần cấu hình WiFi mới:

1. Reset thiết bị (giữ FLASH 10s)
2. Kết nối WiFi **"TuoiCay-Setup"**
3. Mở trình duyệt, chọn WiFi và nhập mật khẩu
4. Thiết bị tự động kết nối

📖 Xem chi tiết: [USER_GUIDE.md](docs/USER_GUIDE.md)

## Thông số kỹ thuật

| Thông số | Giá trị |
|----------|---------|
| MCU | ESP8266 80MHz |
| Flash | 4MB |
| RAM sử dụng | ~55% (45KB/80KB) |
| Flash sử dụng | ~42% (435KB/1MB) |
| WiFi | 2.4GHz 802.11 b/g/n |
| Giao thức | HTTP, MQTT, mDNS |

## Đóng góp

Mọi đóng góp đều được chào đón! Xem [CONTRIBUTING.md](CONTRIBUTING.md) để biết cách tham gia.

## License

MIT License - xem [LICENSE](LICENSE) để biết chi tiết.

## Tác giả

- **Your Name** - *Initial work*

---

*Made with ❤️ for plants*

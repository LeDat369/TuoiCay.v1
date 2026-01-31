# TuoiCay v1.0 - API Documentation

## Overview

Hệ thống tưới cây tự động TuoiCay v1.0 cung cấp các giao diện điều khiển sau:
- **HTTP REST API**: Điều khiển qua trình duyệt web
- **MQTT API**: Điều khiển từ xa qua broker MQTT
- **Captive Portal**: Cấu hình WiFi lần đầu

---

## 1. HTTP REST API

### Base URL
```
http://<device-ip>/api
```

### 1.1 Lấy trạng thái hệ thống

**Endpoint:** `GET /api/status`

**Response:**
```json
{
  "moisture": 65,
  "pumpRunning": false,
  "pumpReason": "OFF",
  "pumpRuntime": 0,
  "autoMode": true,
  "thresholdDry": 30,
  "thresholdWet": 60,
  "uptime": 3600
}
```

| Field | Type | Description |
|-------|------|-------------|
| moisture | int (0-100) | Độ ẩm đất trung bình (%) |
| pumpRunning | bool | Trạng thái bơm |
| pumpReason | string | Lý do bơm: "OFF", "MANUAL", "AUTO", "SCHEDULED" |
| pumpRuntime | int | Thời gian bơm đã chạy (giây) |
| autoMode | bool | Chế độ tự động |
| thresholdDry | int | Ngưỡng đất khô (%) |
| thresholdWet | int | Ngưỡng đất ướt (%) |
| uptime | int | Thời gian hoạt động (giây) |

---

### 1.2 Điều khiển máy bơm

**Endpoint:** `POST /api/pump`

**Request Body:**
```json
{
  "action": "on",
  "duration": 30
}
```

| Field | Type | Description |
|-------|------|-------------|
| action | string | "on", "off", "toggle" |
| duration | int | Thời gian chạy tối đa (giây, chỉ khi action="on") |

**Response:**
```json
{
  "success": true,
  "pumpRunning": true,
  "message": "Pump turned on"
}
```

---

### 1.3 Đổi chế độ hoạt động

**Endpoint:** `POST /api/mode`

**Request Body:**
```json
{
  "mode": "auto"
}
```

| Field | Type | Description |
|-------|------|-------------|
| mode | string | "auto" hoặc "manual" |

**Response:**
```json
{
  "success": true,
  "mode": "auto"
}
```

---

### 1.4 Cấu hình ngưỡng tưới

**Endpoint:** `POST /api/config`

**Request Body:**
```json
{
  "threshold_dry": 25,
  "threshold_wet": 55
}
```

| Field | Type | Description |
|-------|------|-------------|
| threshold_dry | int (0-100) | Ngưỡng bắt đầu tưới |
| threshold_wet | int (0-100) | Ngưỡng dừng tưới |

**Response:**
```json
{
  "success": true,
  "thresholdDry": 25,
  "thresholdWet": 55
}
```

---

### 1.5 Dashboard HTML

**Endpoint:** `GET /`

Trả về trang web dashboard để điều khiển trực quan.

---

## 2. MQTT API

### 2.1 Cấu hình MQTT

| Parameter | Default Value |
|-----------|---------------|
| Broker | 192.168.221.5 |
| Port | 1883 |
| Client ID | tuoicay-001 |
| Base Topic | devices/tuoicay-001 |

### 2.2 Topics xuất dữ liệu (Publish)

#### Dữ liệu cảm biến
**Topic:** `devices/{deviceId}/sensor/data`
**QoS:** 0
**Retain:** false
**Interval:** 5 giây

```json
{
  "moisture1": 62,
  "moisture2": 68,
  "moistureAvg": 65,
  "moistureRaw": 2456,
  "ts": 1234567890
}
```

#### Trạng thái máy bơm
**Topic:** `devices/{deviceId}/pump/status`
**QoS:** 1
**Retain:** false

```json
{
  "running": false,
  "runtime": 0,
  "reason": "OFF",
  "ts": 1234567890
}
```

#### Trạng thái chế độ
**Topic:** `devices/{deviceId}/mode`
**QoS:** 1
**Retain:** true

```json
{
  "mode": "auto",
  "threshold_dry": 30,
  "threshold_wet": 60,
  "ts": 1234567890
}
```

#### Last Will Testament (LWT)
**Topic:** `devices/{deviceId}/status`
**Payload:** `offline`
**QoS:** 1
**Retain:** true

Khi thiết bị kết nối thành công sẽ publish:
```
online
```

---

### 2.3 Topics điều khiển (Subscribe)

#### Điều khiển máy bơm
**Topic:** `devices/{deviceId}/pump/control`

```json
{
  "action": "on",
  "duration": 30
}
```

#### Đổi chế độ
**Topic:** `devices/{deviceId}/mode/control`

```json
{
  "mode": "auto"
}
```

#### Cấu hình
**Topic:** `devices/{deviceId}/config`

```json
{
  "threshold_dry": 30,
  "threshold_wet": 60,
  "max_runtime": 120
}
```

---

## 3. Captive Portal (WiFi Provisioning)

Khi thiết bị không kết nối được WiFi hoặc được reset về factory mode:

1. Thiết bị tạo Access Point: **TuoiCay-Setup**
2. Kết nối WiFi từ điện thoại/laptop vào AP này
3. Trình duyệt tự động mở trang cấu hình (hoặc truy cập http://192.168.4.1)
4. Chọn WiFi và nhập mật khẩu
5. Tùy chọn: Cấu hình MQTT server
6. Nhấn "Lưu cấu hình"
7. Thiết bị khởi động lại và kết nối WiFi mới

---

## 4. OTA Updates

### 4.1 Arduino OTA

Thiết bị hỗ trợ OTA update qua Arduino IDE hoặc PlatformIO:

```bash
# PlatformIO
platformio run --target upload --upload-port <device-ip>

# Arduino IDE
Tools > Port > Network > TuoiCay-001 at <device-ip>
```

**Hostname:** TuoiCay-001
**Port:** 8266 (default)

---

## 5. Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | TC_ERR_OK | Thành công |
| 1 | TC_ERR_TIMEOUT | Hết thời gian chờ |
| 2 | TC_ERR_INVALID_PARAM | Tham số không hợp lệ |
| 3 | TC_ERR_WIFI_FAILED | Kết nối WiFi thất bại |
| 4 | TC_ERR_MQTT_FAILED | Kết nối MQTT thất bại |
| 5 | TC_ERR_SENSOR_FAILED | Đọc cảm biến thất bại |
| 6 | TC_ERR_PUMP_BLOCKED | Bơm bị block (an toàn) |
| 7 | TC_ERR_STORAGE_FAILED | Lỗi đọc/ghi flash |

---

## 6. Safety Features

### 6.1 Pump Safety

- **Max Runtime:** 120 giây (mặc định), có thể cấu hình
- **Min Off Time:** 30 giây giữa các lần bật
- **Auto Timeout:** Tự tắt sau khi hết thời gian chạy
- **Boot Safe:** Bơm luôn OFF khi khởi động

### 6.2 Watchdog Timer

- **Timeout:** 10 giây
- **Action:** Tự động restart nếu firmware bị treo

---

## 7. LED Indicators

| LED | State | Meaning |
|-----|-------|---------|
| Built-in LED | Blink 500ms | WiFi đang kết nối |
| Built-in LED | Solid ON | WiFi đã kết nối |
| Built-in LED | Blink 100ms | OTA đang cập nhật |

---

## 8. Example Code

### 8.1 Python MQTT Client

```python
import paho.mqtt.client as mqtt
import json

BROKER = "192.168.221.5"
DEVICE_ID = "tuoicay-001"
BASE_TOPIC = f"devices/{DEVICE_ID}"

def on_connect(client, userdata, flags, rc):
    print("Connected!")
    client.subscribe(f"{BASE_TOPIC}/sensor/data")
    client.subscribe(f"{BASE_TOPIC}/pump/status")

def on_message(client, userdata, msg):
    data = json.loads(msg.payload)
    print(f"{msg.topic}: {data}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, 1883, 60)

# Turn pump on for 30 seconds
client.publish(f"{BASE_TOPIC}/pump/control", 
               json.dumps({"action": "on", "duration": 30}))

client.loop_forever()
```

### 8.2 cURL HTTP Examples

```bash
# Get status
curl http://192.168.1.100/api/status

# Turn pump on
curl -X POST http://192.168.1.100/api/pump \
  -H "Content-Type: application/json" \
  -d '{"action":"on","duration":30}'

# Set auto mode
curl -X POST http://192.168.1.100/api/mode \
  -H "Content-Type: application/json" \
  -d '{"mode":"auto"}'

# Configure thresholds
curl -X POST http://192.168.1.100/api/config \
  -H "Content-Type: application/json" \
  -d '{"threshold_dry":25,"threshold_wet":55}'
```

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2025-01 | Initial release |

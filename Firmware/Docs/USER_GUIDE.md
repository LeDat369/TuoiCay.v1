# TuoiCay v1.0 - Hướng dẫn sử dụng

## 1. Giới thiệu

**TuoiCay v1.0** là hệ thống tưới cây tự động sử dụng ESP8266 (NodeMCU), có khả năng:

- 🌱 Đo độ ẩm đất qua 2 cảm biến
- 💧 Tự động bật/tắt máy bơm theo ngưỡng cấu hình
- 📱 Điều khiển qua Web Dashboard
- 🌐 Giám sát từ xa qua MQTT
- ⏰ Lập lịch tưới theo giờ
- 🔄 Cập nhật OTA không cần dây

---

## 2. Thông số kỹ thuật

| Thông số | Giá trị |
|----------|---------|
| MCU | ESP8266 (NodeMCU v2) |
| Flash | 4MB |
| RAM | 80KB |
| WiFi | 2.4GHz 802.11 b/g/n |
| Nguồn | 5V USB hoặc 5-12V DC |

### Chân kết nối (Pinout)

| Pin | Chức năng | Ghi chú |
|-----|-----------|---------|
| A0 | Cảm biến độ ẩm 1 | Analog input |
| D1 (GPIO5) | Cảm biến độ ẩm 2 | Digital/Analog |
| D2 (GPIO4) | Relay máy bơm | Active LOW |
| D4 (GPIO2) | LED trạng thái | Built-in LED |

---

## 3. Cài đặt lần đầu

### 3.1 Cấp nguồn

1. Kết nối nguồn 5V qua cổng USB hoặc pin Vin
2. LED trạng thái sẽ nhấp nháy (đang chờ cấu hình WiFi)

### 3.2 Cấu hình WiFi qua Captive Portal

1. Mở WiFi trên điện thoại/laptop
2. Kết nối vào mạng **"TuoiCay-Setup"**
3. Trang cấu hình sẽ tự động mở (hoặc truy cập http://192.168.4.1)
4. Chọn mạng WiFi nhà bạn từ danh sách
5. Nhập mật khẩu WiFi
6. (Tùy chọn) Nhập địa chỉ MQTT server nếu có
7. Nhấn **"Lưu cấu hình"**
8. Thiết bị sẽ khởi động lại và kết nối WiFi

### 3.3 Xác nhận kết nối

- LED sáng liên tục = đã kết nối WiFi
- Mở trình duyệt, truy cập IP của thiết bị (xem trong router)

---

## 4. Sử dụng Web Dashboard

### 4.1 Truy cập Dashboard

Mở trình duyệt và truy cập:
```
http://<địa-chỉ-ip-thiết-bị>/
```

### 4.2 Giao diện Dashboard

```
┌─────────────────────────────────────┐
│         🌱 TuoiCay v1.0             │
├─────────────────────────────────────┤
│  ┌─────────────────────────────┐    │
│  │      Độ ẩm đất              │    │
│  │         65%                 │    │
│  └─────────────────────────────┘    │
│                                     │
│  ┌─────────┐    ┌─────────────┐     │
│  │ Máy bơm │    │ Chế độ      │     │
│  │   OFF   │    │    AUTO     │     │
│  │[BẬT/TẮT]│    │ [ĐỔI CHẾ ĐỘ]│     │
│  └─────────┘    └─────────────┘     │
│                                     │
│  Cài đặt ngưỡng:                    │
│  Khô: [30]%    Ướt: [60]%           │
│  [LƯU CẤU HÌNH]                     │
└─────────────────────────────────────┘
```

### 4.3 Điều khiển

- **BẬT/TẮT BƠM**: Bật hoặc tắt máy bơm ngay lập tức
- **ĐỔI CHẾ ĐỘ**: Chuyển giữa AUTO và MANUAL
- **Ngưỡng KHÔ**: Độ ẩm dưới mức này sẽ bắt đầu tưới
- **Ngưỡng ƯỚT**: Độ ẩm trên mức này sẽ dừng tưới

---

## 5. Chế độ hoạt động

### 5.1 Chế độ AUTO (Tự động)

Khi ở chế độ AUTO:
1. Hệ thống đọc độ ẩm mỗi 5 giây
2. Nếu độ ẩm < ngưỡng KHÔ → Bật bơm
3. Khi độ ẩm > ngưỡng ƯỚT → Tắt bơm
4. Bơm tự động tắt sau 120 giây (an toàn)

**Ví dụ:**
- Ngưỡng KHÔ = 30%
- Ngưỡng ƯỚT = 60%
- Độ ẩm hiện tại = 25% → Bơm BẬT
- Độ ẩm tăng lên 62% → Bơm TẮT

### 5.2 Chế độ MANUAL (Thủ công)

Khi ở chế độ MANUAL:
- Bơm chỉ bật/tắt khi người dùng điều khiển
- Vẫn áp dụng giới hạn an toàn (max 120 giây)

---

## 6. Lập lịch tưới

Hệ thống hỗ trợ tối đa 4 lịch tưới:

### 6.1 Cấu hình qua MQTT

```json
{
  "enabled": true,
  "entries": [
    {"hour": 6, "minute": 0, "duration": 30, "enabled": true},
    {"hour": 18, "minute": 0, "duration": 30, "enabled": true}
  ]
}
```

### 6.2 Logic lịch tưới

1. Khi đến giờ đã cài đặt
2. Kiểm tra độ ẩm hiện tại
3. Nếu đất KHÔ → Bật bơm trong thời gian `duration`
4. Nếu đất đủ ẩm → Bỏ qua lịch này

---

## 7. Kết nối MQTT (Nâng cao)

### 7.1 Cấu hình MQTT Server

- **Broker**: Địa chỉ IP server MQTT (VD: 192.168.1.100)
- **Port**: 1883 (mặc định)
- **Username/Password**: Tùy chọn

### 7.2 Ví dụ với Home Assistant

```yaml
# configuration.yaml

mqtt:
  sensor:
    - name: "Độ ẩm đất TuoiCay"
      state_topic: "devices/tuoicay-001/sensor/data"
      value_template: "{{ value_json.moistureAvg }}"
      unit_of_measurement: "%"
      
  switch:
    - name: "Máy bơm TuoiCay"
      command_topic: "devices/tuoicay-001/pump/control"
      state_topic: "devices/tuoicay-001/pump/status"
      payload_on: '{"action":"on"}'
      payload_off: '{"action":"off"}'
      value_template: "{{ value_json.running }}"
```

---

## 8. Cập nhật firmware OTA

### 8.1 Cập nhật qua PlatformIO

```bash
# Upload firmware qua WiFi
platformio run --target upload --upload-port <ip-thiết-bị>
```

### 8.2 Cập nhật qua Arduino IDE

1. Mở Arduino IDE
2. Tools > Port > Network Ports
3. Chọn "TuoiCay-001 at <ip>"
4. Upload Sketch như bình thường

---

## 9. Khắc phục sự cố

### 9.1 Không kết nối được WiFi

**Triệu chứng:** LED nhấp nháy liên tục

**Giải pháp:**
1. Kiểm tra mật khẩu WiFi
2. Đưa thiết bị gần router hơn
3. Reset về factory: Giữ nút FLASH 10 giây

### 9.2 Bơm không hoạt động

**Triệu chứng:** Không có nước khi bật bơm

**Kiểm tra:**
1. Nguồn cấp cho relay (5V đủ?)
2. Relay có click không?
3. Dây nối bơm đúng cực?

### 9.3 Độ ẩm đọc sai

**Triệu chứng:** Hiển thị 0% hoặc 100%

**Kiểm tra:**
1. Cảm biến cắm đúng chân
2. Cảm biến có tiếp xúc đất không
3. Hiệu chuẩn lại giá trị DRY/WET trong config.h

### 9.4 Reset về cài đặt gốc

1. Tắt nguồn
2. Giữ nút FLASH
3. Bật nguồn (vẫn giữ FLASH)
4. Đợi 10 giây rồi nhả
5. Thiết bị sẽ vào chế độ cấu hình WiFi

---

## 10. Bảo trì

### 10.1 Hàng tuần
- Kiểm tra cảm biến không bị rỉ sét
- Vệ sinh đầu cảm biến nếu có bụi/đất bám

### 10.2 Hàng tháng
- Kiểm tra ống dẫn nước không tắc
- Test thủ công bơm hoạt động tốt
- Kiểm tra nguồn điện ổn định

---

## 11. Thông số cấu hình mặc định

| Thông số | Giá trị | Mô tả |
|----------|---------|-------|
| THRESHOLD_DRY | 30% | Ngưỡng đất khô |
| THRESHOLD_WET | 60% | Ngưỡng đất ướt |
| PUMP_MAX_RUNTIME | 120s | Thời gian bơm tối đa |
| PUMP_MIN_OFF_TIME | 30s | Thời gian nghỉ tối thiểu |
| SENSOR_READ_INTERVAL | 5000ms | Chu kỳ đọc cảm biến |
| WDT_TIMEOUT | 10s | Timeout watchdog |

---

## 12. Liên hệ hỗ trợ

- **GitHub**: https://github.com/example/tuoicay
- **Email**: support@example.com

---

*Phiên bản tài liệu: 1.0.0 - Cập nhật: 01/2025*

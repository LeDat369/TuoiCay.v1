# TuoiCay v1.0 - Hướng dẫn lắp đặt phần cứng

## 1. Danh sách linh kiện

| STT | Linh kiện | Số lượng | Ghi chú |
|-----|-----------|----------|---------|
| 1 | NodeMCU ESP8266 v2 | 1 | CH340 hoặc CP2102 |
| 2 | Cảm biến độ ẩm đất | 2 | Capacitive (khuyến nghị) |
| 3 | Module Relay 5V | 1 | 1 kênh, Active LOW |
| 4 | Máy bơm mini | 1 | DC 3-6V hoặc 12V |
| 5 | Nguồn 5V 2A | 1 | Adapter USB hoặc DC |
| 6 | Dây jumper | 10+ | Male-Female, Male-Male |
| 7 | Breadboard | 1 | (tùy chọn) |
| 8 | Hộp điện | 1 | Chống nước IP65 |

---

## 2. Sơ đồ kết nối

```
                    NodeMCU ESP8266
                    ┌─────────────┐
                    │             │
    Sensor 1  ──────│ A0      Vin │────── +5V
    (Analog)        │             │
                    │ D1      GND │────── GND
    Sensor 2  ──────│ (GPIO5)     │
    (Digital)       │             │
                    │ D2      3V3 │
    Relay IN  ──────│ (GPIO4)     │
                    │             │
                    │ D4      D0  │
    (LED Status)    │ (GPIO2)     │
                    │             │
                    └─────────────┘


    ┌──────────────────────────────────────────────────────┐
    │                   POWER & RELAY                       │
    │                                                       │
    │   5V ────┬──── VCC NodeMCU                           │
    │         │                                             │
    │         └──── VCC Relay Module                       │
    │                                                       │
    │   GND ───┬──── GND NodeMCU                           │
    │         │                                             │
    │         └──── GND Relay Module                       │
    │                                                       │
    │   NodeMCU D2 ──── IN Relay                           │
    │                                                       │
    │   Relay COM ──── Pump (+)                            │
    │   Relay NO ───── +5V (hoặc +12V cho bơm 12V)         │
    │   Pump (-) ───── GND                                  │
    └──────────────────────────────────────────────────────┘
```

---

## 3. Kết nối chi tiết

### 3.1 Cảm biến độ ẩm đất #1 (Analog)

| Cảm biến | NodeMCU | Ghi chú |
|----------|---------|---------|
| VCC | 3.3V | **KHÔNG dùng 5V** |
| GND | GND | |
| AO | A0 | Analog output |

### 3.2 Cảm biến độ ẩm đất #2 (Digital)

| Cảm biến | NodeMCU | Ghi chú |
|----------|---------|---------|
| VCC | 3.3V | |
| GND | GND | |
| DO | D1 (GPIO5) | Digital output |

### 3.3 Module Relay

| Relay | NodeMCU | Ghi chú |
|-------|---------|---------|
| VCC | Vin (5V) | Lấy từ nguồn 5V |
| GND | GND | |
| IN | D2 (GPIO4) | Tín hiệu điều khiển |

### 3.4 Máy bơm

| Bơm | Kết nối | Ghi chú |
|-----|---------|---------|
| + (Red) | Relay COM | |
| - (Black) | GND nguồn | |

Relay NO nối với nguồn (+) của bơm:
- Bơm 5V: Nối với 5V
- Bơm 12V: Nối với nguồn 12V riêng

---

## 4. Hình ảnh minh họa

### 4.1 Sơ đồ Fritzing

```
    [Nguồn 5V]
        │
        ├───[+]──────┐
        │            │
    [NodeMCU]   [Relay Module]
        │            │
        ├───[D2]─────[IN]
        │            │
        ├───[A0]─────[Sensor 1 AO]
        │            │
        ├───[D1]─────[Sensor 2 DO]
        │            │
        │       [COM]────[Pump +]
        │       [NO]─────[5V/12V]
        │            │
        └───[GND]────┴───[Pump -]
```

### 4.2 Lưu ý quan trọng

⚠️ **CẢNH BÁO AN TOÀN:**

1. **Không để nước dính vào mạch điện**
2. **Ngắt nguồn trước khi đấu dây**
3. **Kiểm tra kỹ cực tính trước khi cấp nguồn**
4. **Sử dụng hộp chống nước cho mạch**

---

## 5. Kiểm tra sau khi lắp đặt

### 5.1 Checklist

- [ ] Nguồn 5V cấp đúng Vin và GND
- [ ] LED Power trên NodeMCU sáng
- [ ] Sensor 1 nối đúng A0
- [ ] Sensor 2 nối đúng D1
- [ ] Relay nối đúng D2
- [ ] Bơm nối qua relay (COM + NO)

### 5.2 Test từng phần

**Test 1: Kiểm tra nguồn**
```
1. Cấp nguồn 5V
2. LED Power trên NodeMCU phải sáng
3. Đo điện áp: Vin = 5V, 3V3 = 3.3V
```

**Test 2: Kiểm tra cảm biến**
```
1. Upload firmware test đọc analog
2. Nhúng sensor vào nước: giá trị giảm
3. Để sensor khô: giá trị tăng
```

**Test 3: Kiểm tra relay**
```
1. Upload firmware test toggle relay
2. Nghe tiếng "click" từ relay
3. LED trên relay bật/tắt
```

**Test 4: Kiểm tra bơm**
```
1. Bật relay
2. Bơm phải chạy (có tiếng motor)
3. Nước phải được bơm ra
```

---

## 6. Lắp đặt thực tế

### 6.1 Vị trí đặt cảm biến

```
    ┌─────────────────────────────────┐
    │           Chậu cây              │
    │                                 │
    │     [Sensor 1]    [Sensor 2]    │
    │         │              │        │
    │         ▼              ▼        │
    │    ┌─────────────────────┐      │
    │    │                     │      │
    │    │    Đất trồng cây    │      │
    │    │                     │      │
    │    │   ●───── rễ cây     │      │
    │    │                     │      │
    │    └─────────────────────┘      │
    │                                 │
    │    ════════════════════════     │
    │         Ống nước từ bơm         │
    └─────────────────────────────────┘
```

**Lưu ý:**
- Cắm sensor sâu 2-3cm dưới mặt đất
- Đặt 2 sensor ở 2 vị trí khác nhau trong chậu
- Ống nước đặt phía đối diện sensor

### 6.2 Hộp điều khiển

```
    ┌──────────────────────────────┐
    │                              │
    │  ┌──────┐    ┌──────────┐   │
    │  │NodeMCU│    │  Relay   │   │
    │  │      │    │          │   │
    │  └──────┘    └──────────┘   │
    │                              │
    │  [USB]  [Dây sensor]  [Bơm] │
    └──────────────────────────────┘
         │         │          │
         │         │          │
      Nguồn     Sensor      Bơm
```

---

## 7. Xử lý sự cố phần cứng

### 7.1 NodeMCU không nhận nguồn

| Nguyên nhân | Giải pháp |
|-------------|-----------|
| Cáp USB hỏng | Thử cáp khác |
| Nguồn yếu | Dùng nguồn 5V 2A |
| IC nguồn hỏng | Thay NodeMCU mới |

### 7.2 Sensor đọc sai giá trị

| Nguyên nhân | Giải pháp |
|-------------|-----------|
| Sensor hỏng | Thử sensor khác |
| Dây lỏng | Kiểm tra mối nối |
| Nhiễu | Thêm tụ 100nF |

### 7.3 Relay không đóng

| Nguyên nhân | Giải pháp |
|-------------|-----------|
| Thiếu nguồn | Cấp 5V riêng cho relay |
| Tín hiệu sai | Kiểm tra mức logic |
| Relay hỏng | Thay relay mới |

### 7.4 Bơm không chạy

| Nguyên nhân | Giải pháp |
|-------------|-----------|
| Nguồn yếu | Cấp nguồn riêng cho bơm |
| Relay không đóng | Xem mục 7.3 |
| Bơm hỏng | Test bơm trực tiếp |
| Kẹt cặn | Vệ sinh bơm |

---

## 8. Nâng cấp phần cứng (Tùy chọn)

### 8.1 Thêm màn hình OLED

```
OLED 0.96" I2C:
- VCC → 3.3V
- GND → GND
- SDA → D2 (GPIO4) *cần đổi pin relay
- SCL → D1 (GPIO5) *cần đổi pin sensor
```

### 8.2 Thêm nút nhấn

```
Button:
- Chân 1 → D3 (GPIO0)
- Chân 2 → GND
(Dùng internal pull-up)
```

### 8.3 Thêm cảm biến nhiệt độ

```
DS18B20:
- VCC → 3.3V
- GND → GND
- DATA → D5 (GPIO14)
- Điện trở 4.7K giữa VCC và DATA
```

---

## 9. Bill of Materials (BOM)

| # | Item | Qty | Price (VND) | Link |
|---|------|-----|-------------|------|
| 1 | NodeMCU ESP8266 | 1 | 65,000 | [Link] |
| 2 | Cảm biến độ ẩm capacitive | 2 | 30,000 | [Link] |
| 3 | Module Relay 5V 1CH | 1 | 15,000 | [Link] |
| 4 | Bơm mini 5V | 1 | 35,000 | [Link] |
| 5 | Adapter 5V 2A | 1 | 35,000 | [Link] |
| 6 | Dây jumper (bộ) | 1 | 20,000 | [Link] |
| 7 | Hộp nhựa IP65 | 1 | 40,000 | [Link] |
| **Tổng** | | | **~240,000** | |

---

*Phiên bản: 1.0.0 - Cập nhật: 01/2025*

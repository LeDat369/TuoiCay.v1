# Hệ Thống Tưới Cây Tự Động - Sơ Đồ Phần Cứng (Hardware Pinout)

## 1. Tổng Quan Hệ Thống

Hệ thống tưới cây tự động bao gồm:
- **Bộ Điều Khiển**: ESP8266 (WiFi Microcontroller)
- **Thiết Bị Điều Khiển**: MOSFET N-Channel (2N7000 hoặc IRF520)
- **Bơm**: Máy Bơm Mini 5V DC
- **Nguồn Điện**: Adapter USB 5V/2A
- **Cảm Biến**: Soil Moisture Sensor (tuỳ chọn)

---

## 2. Sơ Đồ Khối (Block Diagram)

```
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│  ┌──────────────┐      ┌──────────────┐     ┌───────────┐  │
│  │  ESP8266     │      │  MOSFET      │     │ Pump 5V   │  │
│  │              │      │  N-Channel   │     │           │  │
│  │  D6/GPIO12   ├─────►│  Gate        │     │    (+)    │  │
│  │  GND         ├─────►│  Source      │     │           │  │
│  │  3.3V        │      │  Drain ──────┼────►│    (-)    │  │
│  │  A0 (ADC)    ◄──────┤ Moisture     │     └─────┬─────┘  │
│  │              │      │  Sensor      │           │        │
│  └──────────────┘      └──────┬───────┘           │        │
│                               │                   │        │
│                        ┌──────┴───────┐       ┌───┴────┐   │
│                        │  5V Power    │       │  5V    │   │
│                        │   Supply     │◄──────│ Source │   │
│                        └──────────────┘       └────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. Bảng Kết Nối Chi Tiết (Pinout Table)

### 3.1 ESP8266 Pinout

| GPIO Pin | Chức Năng | Kết Nối | Ghi Chú |
|----------|-----------|--------|--------|
| D1 (GPIO5) | Digital Input | Cảm biến ẩm số 2 (digital) | Đọc tín hiệu digital cảm biến 2 |
| D5 (GPIO14) | Digital Input | Cảm biến ẩm số 1 (digital) | Đọc tín hiệu digital cảm biến 1 |
| D6 (GPIO12) | PWM Output / MOSFET Gate | Điều khiển máy bơm (PWM) / Gate MOSFET | Xuất PWM để điều chỉnh bơm, hoặc điều khiển ON/OFF |
| A0 (ADC0) | Analog Input | Cảm biến ẩm số 2 (analog) | Đọc tín hiệu analog cảm biến 2 |
| D0 (GPIO16) | - | - | Không sử dụng |
| D2 (GPIO4) | - | - | Dự phòng |
# Pinout phần cứng — Hệ Thống Tưới Cây Tự Động

## Tổng quan phần cứng

Thành phần chính:
- Bộ điều khiển: ESP8266 (NodeMCU hoặc tương tự)
- MOSFET N-Channel (ví dụ: 2N7000, IRL-series; chọn loại có Rds(on) thấp ở VGS=3.3V)
- Máy bơm mini 5V DC
- Nguồn: Adapter 5V (USB) đủ dòng (tối thiểu 2A)
- Cảm biến độ ẩm đất (2 x, tuỳ chọn)

---

## Sơ đồ khối (tóm tắt)

```
  +5V (Power) ------+             +-----------+
                     |             | Pump 5V   |
                     |             | (+)       |
               +-----+------+      +----+------+
               |            |           |
           +---+  MOSFET    +-----------+   Pump (-) -> MOSFET Drain
           |   |  N-Channel |           |
           |   +-----+------+           |
           |         |                  
  ESP8266  |         | Gate <- D6 (GPIO12)
  (3.3V)   |         +-- R 10k (pulldown)
   D6 ---->+                      GND Common
   A0 ---->+ (Analog sensor)
   D1/D5 --+ (Digital sensors)
```

---

## Bảng kết nối (các chân phần cứng chính)

### ESP8266 (chọn chân đã dùng)

- `D6 (GPIO12)` — MOSFET Gate (đi qua resistor pulldown 10k tới GND)
- `A0 (ADC)` — Analog out từ cảm biến ẩm 2
- `D1 (GPIO5)` — Digital out cảm biến ẩm 2
- `D5 (GPIO14)` — Digital out cảm biến ẩm 1
- `3.3V` — Cấp nguồn cho cảm biến (nếu yêu cầu 3.3V)
- `GND` — Mass chung (kết nối tất cả GND)

### MOSFET N-Channel (low-side switch)

- Gate (G): nối tới `D6` qua dây và có pulldown 10kΩ tới GND
- Drain (D): nối tới cực âm của bơm (Pump -)
- Source (S): nối tới GND common

Ghi chú: cấu hình MOSFET là low-side switch — Pump (+) nối trực tiếp tới +5V, Pump (-) về Drain.

### Máy bơm 5V DC

- Pump (+): nối tới +5V từ adapter
- Pump (-): nối tới Drain của MOSFET


### Cảm biến độ ẩm (chi tiết cho 2 cảm biến)

#### Cảm biến 1 (Digital)

- VCC → `3.3V`
- GND → `GND`
- DOUT → `D5 (GPIO14)`

#### Cảm biến 2 (Digital + Analog)

- VCC → `3.3V`
- GND → `GND`
- DOUT → `D1 (GPIO5)`
- AOUT → `A0 (ADC)`

---

## Sơ đồ nối dây chi tiết

1) Nguồn

- Adapter USB 5V → +5V rail
- Adapter GND → GND common

2) Bơm và MOSFET

- +5V rail → Pump (+)
- Pump (-) → MOSFET Drain
- MOSFET Source → GND common
- MOSFET Gate → ESP8266 `D6` và kèm 10kΩ pulldown tới GND
- (Tùy chọn) Diode bảo vệ (flyback diode) nối ngược qua bơm (cathode về +5V, anode về Drain)

3) ESP8266 và cảm biến

- ESP8266 `3.3V` cấp nguồn cho cảm biến (nếu cần)
- ESP8266 `GND` nối chung với GND của nguồn và MOSFET
- Cảm biến digital/analog nối tới chân `D1`/`D5`/`A0` tương ứng

---

## Vật tư (phần cứng)

- ESP8266 NodeMCU hoặc module tương tự
- MOSFET N-Channel (hỗ trợ VGS = 3.3V)
- Máy bơm mini 5V DC (dòng phù hợp với MOSFET và nguồn)
- Adapter 5V USB (≥2A)
- Resistor 10kΩ (pulldown cho Gate)
- Diode (ví dụ 1N4148 hoặc diode Schottky mạnh hơn cho protection)
- Dây kết nối, đầu Jumper, breadboard/PCB

---

## Chú ý an toàn phần cứng

- Luôn nối GND chung giữa ESP8266, nguồn 5V và MOSFET
- Dùng MOSFET có Rds(on) thấp ở VGS = 3.3V hoặc dùng driver nếu cần
- Thêm diode bảo vệ chống back-EMF cho tải inductive (bơm)
- Đảm bảo nguồn 5V có đủ dòng để cấp cho bơm và ESP
# Pinout phần cứng — Hệ Thống Tưới Cây Tự Động

## 1. Tổng quan phần cứng

Thành phần chính:
- Bộ điều khiển: ESP8266 (NodeMCU hoặc tương tự)
- MOSFET N‑Channel (low‑side switch)
- Máy bơm mini 5V DC
- Nguồn: Adapter 5V (USB) đủ dòng (tối thiểu 2A, tuỳ tải)
- 2 × Cảm biến độ ẩm đất (Sensor 1: digital; Sensor 2: digital + analog)

---

## 2. Sơ đồ khối (tóm tắt)

```
  +5V (Power) ------+             +-----------+
                     |             | Pump 5V   |
                     |             | (+)       |
               +-----+------+      +----+------+
               |            |           |
           +---+  MOSFET    +-----------+   Pump (-) -> MOSFET Drain
           |   |  N-Channel |           |
           |   +-----+------+           |
           |         |                  
  ESP8266  |         | Gate <- D6 (GPIO12)  (10kΩ pulldown to GND)
  (3.3V)   |                             GND Common
   D6 ---->+
   A0 ---->+ (Sensor2 AOUT)
   D1 ---->+ (Sensor2 DOUT)
   D5 ---->+ (Sensor1 DOUT)
```

---

## 3. Bảng kết nối (chân phần cứng chính)

### ESP8266

- `D6 (GPIO12)` — điều khiển MOSFET Gate (thông qua resistor pulldown 10kΩ)
- `A0 (ADC)` — analog input từ Sensor 2 (AOUT)
- `D1 (GPIO5)` — digital input từ Sensor 2 (DOUT)
- `D5 (GPIO14)` — digital input từ Sensor 1 (DOUT)
- `3.3V` — cấp nguồn cho cảm biến (nếu cảm biến yêu cầu 3.3V)
- `GND` — mass chung cho ESP8266, nguồn 5V và MOSFET

### MOSFET (N‑Channel, low‑side)

- Gate (G): nối tới `D6` (ESP8266) + pulldown 10kΩ → GND
- Drain (D): nối tới Pump (-)
- Source (S): nối tới GND common

Ghi chú: Pump (+) luôn nối trực tiếp tới +5V; MOSFET cắt/dẫn đường về GND.

### Máy bơm 5V DC

- Pump (+): +5V từ adapter
- Pump (-): MOSFET Drain

### Cảm biến độ ẩm

#### Sensor 1 (Digital)

- VCC → `3.3V`
- GND → `GND`
- DOUT → `D5 (GPIO14)`

#### Sensor 2 (Digital + Analog)

- VCC → `3.3V`
- GND → `GND`
- DOUT → `D1 (GPIO5)`
- AOUT → `A0 (ADC)`

Lưu ý ADC: nếu bạn dùng NodeMCU (board có mạch chia), `A0` chấp nhận tới ~3.3V; nếu là module ESP8266 trần (ESP-12) thì ADC gốc chỉ là 0–1.0V — cần mạch chia nếu cảm biến trả 0–3.3V.

---

## 4. Sơ đồ nối dây chi tiết

1) Nguồn

- Adapter USB 5V → +5V rail
- Adapter GND → GND common (kết nối với ESP8266 GND và MOSFET Source)

2) Bơm và MOSFET

- +5V rail → Pump (+)
- Pump (-) → MOSFET Drain
- MOSFET Source → GND common
- MOSFET Gate → ESP8266 `D6` (kèm 10kΩ pulldown tới GND)
- (Tùy chọn) Diode bảo vệ (flyback) nối ngược qua bơm: cathode → +5V, anode → Drain

3) Cảm biến và ESP8266

- ESP8266 `3.3V` → Sensor VCC (không cấp 5V trực tiếp cho ADC của ESP)
- ESP8266 `GND` ↔ Sensor GND
- Sensor DOUTs → `D5` (Sensor1), `D1` (Sensor2)
- Sensor2 AOUT → `A0` (qua mạch chia nếu cần)

---





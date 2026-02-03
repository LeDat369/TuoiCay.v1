# Hướng dẫn Test OTA (Over-The-Air Update)

## 📋 Yêu cầu

1. ✅ ESP8266 đã nạp code và đang chạy
2. ✅ ESP8266 đã kết nối WiFi thành công
3. ✅ Máy tính và ESP8266 cùng mạng LAN
4. ✅ PlatformIO hoặc Arduino IDE đã cài đặt

---

## 🔧 Chuẩn bị

### Bước 1: Kiểm tra secrets.h

Mở file `include/secrets.h` và đảm bảo đã set OTA password:

```cpp
#define OTA_PASSWORD        "tuoicay123"  // Đổi password của bạn
```

### Bước 2: Nạp lần đầu qua USB

OTA chỉ hoạt động sau khi đã nạp code qua USB ít nhất 1 lần:

```bash
# PlatformIO
pio run --target upload

# Hoặc trong VS Code
# Ctrl+Alt+U (Windows) / Cmd+Alt+U (Mac)
```

### Bước 3: Kiểm tra ESP8266 đã connect WiFi

Mở Serial Monitor (115200 baud) và xem log:

```
[INF][WIFI][connect] Connected to YourWiFi
[INF][WIFI][connect] IP: 192.168.1.XXX
[INF][OTA][init] OTA ready, hostname=TuoiCay-XXXXXX
```

**Ghi nhớ:**
- ✅ **IP address**: 192.168.1.XXX
- ✅ **Hostname**: TuoiCay-XXXXXX (6 ký tự cuối MAC)

---

## 🚀 Test OTA với PlatformIO

### Phương pháp 1: Upload qua mDNS hostname

1. **Sửa platformio.ini** thêm vào:

```ini
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino

; ... (giữ nguyên config hiện tại)

; OTA Configuration
upload_protocol = espota
upload_port = TuoiCay-XXXXXX.local  ; Thay XXXXXX = 6 ký tự cuối MAC
upload_flags = 
    --auth=tuoicay123                ; Password trong secrets.h
```

2. **Upload qua OTA:**

```bash
pio run --target upload
```

### Phương pháp 2: Upload qua IP address

Sửa `upload_port` trong platformio.ini:

```ini
upload_port = 192.168.1.XXX         ; Thay bằng IP thực tế của ESP8266
```

Rồi upload:

```bash
pio run --target upload
```

---

## 🔍 Test OTA với Arduino IDE

### Bước 1: Tìm device

1. Mở **Arduino IDE**
2. Vào menu **Tools → Port**
3. Bạn sẽ thấy:
   - `COM3` (USB port - nếu cắm cáp)
   - `TuoiCay-XXXXXX at 192.168.1.XXX` (OTA port - qua WiFi)

### Bước 2: Chọn OTA Port

Chọn port có dạng `TuoiCay-XXXXXX at 192.168.1.XXX`

### Bước 3: Upload

Click nút **Upload** như bình thường.

Nếu có password, sẽ hiện dialog nhập password → nhập `tuoicay123`

---

## 📊 Theo dõi quá trình OTA

### Trên Serial Monitor

Bạn sẽ thấy log như sau:

```
[INF][OTA][start] Update starting (firmware)
[INF][OTA][prog] Progress: 10%
[INF][OTA][prog] Progress: 20%
[INF][OTA][prog] Progress: 30%
...
[INF][OTA][prog] Progress: 100%
[INF][OTA][done] Update complete!
```

### Trên PlatformIO Terminal

```
Writing at 0x00000000... (10%)
Writing at 0x00010000... (20%)
...
Writing at 0x000F0000... (100%)
Wrote 1048576 bytes (XX ms)
Done
```

### LED Built-in

- **Nhấp nháy chậm**: Đang upload (mỗi 1-2%)
- **Nhấp nháy nhanh 5 lần**: Upload thành công
- **Nhấp nháy rất nhanh 10 lần**: Upload lỗi

---

## ✅ Test thành công

Sau khi upload xong:
1. ✅ ESP8266 tự động reboot
2. ✅ Serial Monitor sẽ hiện boot log mới
3. ✅ LED nhấp nháy 5 lần → Thành công!

---

## ❌ Xử lý lỗi thường gặp

### Lỗi 1: "No response from device"

**Nguyên nhân:**
- ESP8266 không cùng mạng với máy tính
- Firewall chặn port 3232

**Giải pháp:**
```bash
# Windows: Tắt firewall tạm thời hoặc allow port 3232
# Ping thử để kiểm tra kết nối
ping 192.168.1.XXX
```

### Lỗi 2: "Authentication failed"

**Nguyên nhân:** Password sai

**Giải pháp:**
- Kiểm tra `OTA_PASSWORD` trong `secrets.h`
- Upload lại qua USB để cập nhật password mới

### Lỗi 3: "Device not found"

**Nguyên nhân:** mDNS không hoạt động trên router

**Giải pháp:** Dùng IP address thay vì hostname:

```ini
upload_port = 192.168.1.XXX
```

### Lỗi 4: "Not enough space"

**Nguyên nhân:** Firmware quá lớn

**Giải pháp:** Kiểm tra trong platformio.ini:

```ini
board_build.flash_mode = dio
board_build.ldscript = eagle.flash.4m2m.ld  ; 4MB flash, 2MB cho OTA
```

---

## 🔥 Test nhanh - Thay đổi gì đó để verify

### Test 1: Thay đổi thời gian đọc sensor

Trong `include/config.h`:

```cpp
// Trước
#define SENSOR_READ_INTERVAL_MS 500

// Sau (đọc chậm hơn)
#define SENSOR_READ_INTERVAL_MS 2000
```

Upload qua OTA → Xem Serial log sensor sẽ đọc mỗi 2s thay vì 0.5s

### Test 2: Thay đổi log message

Trong `src/main.cpp`, tìm dòng:

```cpp
LOG_INF(MOD_SYSTEM, "init", "Setup complete! Entering main loop...");
```

Đổi thành:

```cpp
LOG_INF(MOD_SYSTEM, "init", "🚀 OTA TEST SUCCESS! System ready!");
```

Upload qua OTA → Xem boot message mới

---

## 📈 Benchmark

Thời gian upload thông thường:
- **Qua USB**: ~30-60 giây
- **Qua OTA**: ~45-90 giây (chậm hơn vì qua WiFi)

Kích thước firmware:
- **Hiện tại**: ~400-500KB
- **Maximum**: ~1MB (do OTA cần 2 partitions)

---

## 💡 Tips

1. **OTA chỉ hoạt động khi:**
   - ✅ WiFi connected
   - ✅ Device không đang trong Captive Portal mode
   - ✅ Không đang pump water (tuy nhiên OTA sẽ chặn pump)

2. **Best practices:**
   - 📌 Đặt password mạnh cho OTA
   - 📌 Test OTA trên bench trước khi deploy field
   - 📌 Backup firmware cũ trước khi OTA
   - 📌 Có kế hoạch rollback nếu OTA fail

3. **Debugging OTA:**
   - Dùng `monitor_filters = esp8266_exception_decoder` trong platformio.ini
   - Xem log chi tiết với `LOG_LEVEL=4` (DEBUG)

---

## 🎯 Checklist hoàn thành

- [ ] ESP8266 connect WiFi thành công
- [ ] Serial log hiện "OTA ready"
- [ ] Ping được IP của ESP8266
- [ ] PlatformIO detect được OTA port
- [ ] Upload qua OTA lần đầu thành công
- [ ] LED nhấp nháy 5 lần sau OTA
- [ ] Device reboot và hoạt động bình thường
- [ ] Thay đổi code và test OTA lần 2 thành công

---

## 📞 Troubleshooting Commands

```bash
# Tìm ESP8266 trên network (Windows)
arp -a | findstr "XX-XX-XX"  # Thay XX bằng OUI của ESP8266

# Tìm ESP8266 qua mDNS (cần Bonjour/Avahi)
ping TuoiCay-XXXXXX.local

# Test kết nối OTA port
Test-NetConnection -ComputerName 192.168.1.XXX -Port 3232

# Upload với verbose log
pio run --target upload -v
```

---

## ✨ Kết luận

Nếu làm theo hướng dẫn trên và OTA thành công → **Firmware của bạn đã production-ready** cho deployment!

Tiếp theo có thể test:
- Web server (truy cập http://192.168.1.XXX)
- MQTT commands
- Auto watering logic
- Scheduler


# 🚀 Quick Start - Test OTA

## Bước 1: Upload lần đầu qua USB

```bash
pio run --target upload
```

## Bước 2: Xem Serial Monitor để lấy IP

```bash
pio device monitor

# Tìm dòng:
# [INF][WIFI][connect] IP: 192.168.1.XXX
# [INF][OTA][init] OTA ready, hostname=TuoiCay-XXXXXX
```

## Bước 3: Upload qua OTA

### Cách 1: Tự động (Khuyên dùng)

**Windows PowerShell:**
```powershell
.\ota_upload.ps1
```

**Linux/Mac:**
```bash
python3 ota_upload.py
```

Script sẽ:
- ✅ Tự động tìm ESP8266 trên mạng
- ✅ Test kết nối
- ✅ Upload firmware

### Cách 2: Thủ công

1. **Sửa platformio.ini** - Uncomment và sửa IP:

```ini
[env:nodemcuv2_ota]
upload_port = 192.168.1.XXX    ; Thay bằng IP thực tế
```

2. **Upload:**

```bash
pio run -e nodemcuv2_ota --target upload
```

## Bước 4: Verify

Xem Serial Monitor sẽ thấy:
```
[INF][OTA][start] Update starting (firmware)
[INF][OTA][prog] Progress: 10%
...
[INF][OTA][done] Update complete!
```

LED built-in sẽ nhấp nháy 5 lần → ✅ Thành công!

---

## 🔥 Test nhanh

Thay đổi gì đó để verify OTA hoạt động:

**File:** `include/config.h`
```cpp
#define SENSOR_READ_INTERVAL_MS 2000  // Đổi từ 500 → 2000
```

Upload qua OTA → Xem sensor reading sẽ chậm hơn

---

## ❌ Lỗi thường gặp

### "No response from device"
```bash
# Test kết nối
ping 192.168.1.XXX

# Test OTA port
Test-NetConnection -ComputerName 192.168.1.XXX -Port 3232
```

### "Authentication failed"
Kiểm tra password trong `include/secrets.h`:
```cpp
#define OTA_PASSWORD "tuoicay123"
```

---

## 📚 Tài liệu đầy đủ

Xem [Docs/OTA_GUIDE.md](Docs/OTA_GUIDE.md) để biết thêm chi tiết!

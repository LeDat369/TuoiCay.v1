# AI Agent Rules - ESP-IDF Firmware Development

## Mục đích

File này hướng dẫn AI agent sinh code firmware ESP-IDF tuân thủ chặt chẽ các quy tắc đã định nghĩa.

---

## 1. NGUYÊN TẮC CHUNG

### 1.1 KISS - Keep It Simple, Stupid

- Luôn chọn giải pháp đơn giản nhất có thể
- Không over-engineering
- Mỗi hàm chỉ làm một việc duy nhất
- Tránh nested conditions quá sâu (tối đa 3 cấp)
- Ưu tiên early return để giảm độ phức tạp

### 1.2 YAGNI - You Aren't Gonna Need It

- Chỉ implement tính năng khi thực sự cần
- Không code "phòng trường lai"
- Không tạo các hàm/struct chưa dùng
- Xóa code không sử dụng

### 1.3 DRY - Don't Repeat Yourself

- Không copy-paste code
- Tạo hàm chung cho logic lặp lại
- Sử dụng macro cho các giá trị lặp lại
- Tạo utility functions cho các thao tác phổ biến

### 1.4 Không dùng Markdown trong code

- Comment không dùng định dạng markdown
- Log message không dùng markdown

### 1.5 Không dùng emoji

- Không sử dụng emoji trong bất kỳ phần nào của code
- Không sử dụng emoji trong comment
- Không sử dụng emoji trong log message

---

## 2. KIẾN TRÚC VÀ TỔ CHỨC MÃ NGUỒN

### 2.1 Cấu trúc module

Mỗi module phải có cấu trúc như sau:

- module_name.h: Header file - khai báo public interface
- module_name.c: Source file - implementation

### 2.2 Cấu trúc Header file (.h)

Thứ tự các phần trong header file:

1. Include guard
2. C++ extern "C" wrapper
3. Includes
4. Defines
5. Types (struct, enum, typedef)
6. Public function declarations
7. End extern "C" và include guard

### 2.3 Cấu trúc Source file (.c)

Thứ tự các phần trong source file:

1. Includes
2. Private defines
3. Private types
4. Private variables (static)
5. Private function prototypes
6. Public functions
7. Private functions

### 2.4 Quy tắc đặt tên

| Loại        | Quy tắc              | Ví dụ                    |
| ----------- | -------------------- | ------------------------ |
| File        | snake_case           | wifi_manager.c           |
| Hàm public  | module_action        | wifi_manager_connect()   |
| Hàm private | static + snake_case  | static validate_config() |
| Biến global | g_snake_case         | g_device_id              |
| Biến static | s_snake_case         | s_ctx                    |
| Hằng số     | UPPER_SNAKE_CASE     | MAX_RETRY_COUNT          |
| Macro       | UPPER_SNAKE_CASE     | CONFIG_WIFI_SSID         |
| Enum        | module_name_enum_t   | wifi_state_t             |
| Struct      | module_name_struct_t | wifi_config_t            |
| Typedef     | ten_t                | callback_t               |

### 2.5 Nguyên tắc coupling thấp

- Module không được truy cập trực tiếp dữ liệu nội bộ của module khác
- Sử dụng interface/callback để giao tiếp giữa các module
- Mỗi module expose API tối thiểu cần thiết
- Tránh circular dependencies

---

## 3. SOLID PRINCIPLES

### 3.1 Single Responsibility Principle (SRP)

- Mỗi module/hàm chỉ có một lý do để thay đổi
- Tách biệt các chức năng: đọc sensor, xử lý dữ liệu, gửi MQTT, lưu storage thành các module riêng

### 3.2 Open/Closed Principle (OCP)

- Mở rộng bằng cách thêm code mới, không sửa code cũ
- Sử dụng function pointer để mở rộng
- Đăng ký driver/handler thay vì hardcode

### 3.3 Liskov Substitution Principle (LSP)

- Các đối tượng con có thể thay thế đối tượng cha
- Định nghĩa interface chung cho các loại tương tự (ví dụ: các loại sensor khác nhau cùng implement sensor_interface_t)

### 3.4 Interface Segregation Principle (ISP)

- Interface nhỏ, tập trung vào một nhiệm vụ
- Tách interface lớn thành nhiều interface nhỏ chuyên biệt
- Ví dụ: tách network_interface thành connection_interface, data_transfer_interface, config_interface

### 3.5 Dependency Inversion Principle (DIP)

- Module cấp cao không phụ thuộc module cấp thấp
- Cả hai phụ thuộc vào abstraction
- Sử dụng dependency injection thông qua config struct

---

## 4. ĐỒNG BỘ VÀ AN TOÀN

### 4.1 Sử dụng Mutex

- Bảo vệ shared resources bằng mutex
- Luôn kiểm tra kết quả khi lấy mutex
- Đặt timeout hợp lý, không dùng portMAX_DELAY trừ khi cần thiết
- Giải phóng mutex ngay sau khi xong thao tác

### 4.2 Sử dụng Queue cho giao tiếp giữa tasks

- Dùng queue để truyền dữ liệu an toàn giữa các task
- Định nghĩa struct message rõ ràng
- Kiểm tra kết quả send/receive

### 4.3 Event Group cho đồng bộ nhiều tasks

- Dùng event group khi cần đợi nhiều điều kiện
- Định nghĩa event bits rõ ràng với BIT0, BIT1...
- Xác định rõ clear bits hay không sau khi đợi

### 4.4 Atomic operations

- Dùng atomic cho các biến counter đơn giản
- Sử dụng stdatomic.h

---

## 5. ESP-IDF BEST PRACTICES

### 5.1 Error Handling

- Luôn kiểm tra giá trị trả về của các hàm ESP-IDF
- Sử dụng esp_err_to_name() để log error
- Dùng ESP_ERROR_CHECK chỉ khi lỗi là fatal
- Dùng ESP_RETURN_ON_ERROR để giảm boilerplate code

### 5.2 Logging

- ESP_LOGE: Error - Lỗi nghiêm trọng
- ESP_LOGW: Warning - Cảnh báo
- ESP_LOGI: Info - Thông tin chung
- ESP_LOGD: Debug - Chỉ hiện khi debug
- ESP_LOGV: Verbose - Chi tiết nhất
- Đặt log level phù hợp cho từng module

### 5.3 Memory Management

- Sử dụng heap_caps_malloc cho yêu cầu đặc biệt (DMA, PSRAM)
- Luôn kiểm tra NULL sau malloc
- Giải phóng bộ nhớ khi không dùng
- Set pointer về NULL sau khi free để tránh dangling pointer

### 5.4 NVS Storage

- Mở NVS handle, thao tác, commit, đóng handle
- Xử lý lỗi cho từng bước
- Sử dụng namespace để phân loại dữ liệu

### 5.5 Task Creation

- Định nghĩa stack size và priority rõ ràng trong config
- Lưu task handle để có thể delete sau này
- Kiểm tra kết quả xTaskCreate
- Dùng vTaskDelay trong vòng lặp task

### 5.6 Event Loop (esp_event)

- Sử dụng default event loop cho WiFi, IP, MQTT events
- Đăng ký handler với esp_event_handler_register
- Hủy đăng ký khi không cần với esp_event_handler_unregister
- Xử lý event nhanh gọn, không blocking trong handler
- Dùng post event để gửi custom events giữa các module

### 5.7 Timer Management

- Ưu tiên esp_timer cho one-shot và periodic timer
- Callback của esp_timer chạy trong task riêng, có thể dùng các hàm blocking
- Dùng gptimer khi cần độ chính xác cao hoặc hardware timer
- Luôn stop và delete timer khi không dùng
- Không tạo quá nhiều timer, gom các timer có cùng period

### 5.8 GPIO Best Practices

- Cấu hình GPIO mode rõ ràng (input/output/open-drain)
- Sử dụng gpio_config() thay vì các hàm riêng lẻ
- Cấu hình pull-up/pull-down phù hợp
- Dùng gpio_install_isr_service cho interrupt
- Debounce cho button input (phần cứng hoặc phần mềm)

### 5.9 Peripherals (I2C/SPI/UART)

I2C:

- Cấu hình clock speed phù hợp với slave device
- Sử dụng i2c_master_bus cho ESP-IDF 5.x
- Timeout hợp lý cho các transaction
- Xử lý lỗi ACK/NACK

SPI:

- Chọn đúng SPI host (SPI2_HOST, SPI3_HOST)
- Cấu hình clock phase và polarity theo datasheet slave
- Dùng DMA cho transfer lớn
- Quản lý CS pin đúng cách

UART:

- Cấu hình baud rate, parity, stop bits đúng
- Sử dụng ring buffer cho receive
- Xử lý UART events (RX, TX, ERROR)
- Flush buffer khi cần

---

## 6. COMMENT VÀ TÀI LIỆU

### 6.1 Quy tắc comment

- Comment bằng tiếng Việt không dấu
- Giải thích WHY (tại sao), không giải thích WHAT (cái gì) - vì code đã thể hiện WHAT
- Comment cho hàm: mô tả chức năng, tham số, giá trị trả về
- Comment cho struct: mô tả mục đích, giải thích các field

### 6.2 TODO/FIXME format

- TODO: [Tên người] Mô tả việc cần làm
- FIXME: [Tên người] Mô tả lỗi cần sửa

---

## 7. ERROR HANDLING

### 7.1 Validation đầu vào

- Kiểm tra NULL pointer
- Kiểm tra giá trị hợp lệ (range, size)
- Kiểm tra trạng thái module (đã init chưa)
- Return error code phù hợp và log message

### 7.2 Resource cleanup khi lỗi

- Sử dụng pattern goto cleanup khi khởi tạo nhiều bước
- Cleanup theo thứ tự ngược với init
- Đảm bảo không leak resource khi có lỗi

### 7.3 Error code conventions

Sử dụng ESP-IDF error codes:

- ESP_OK: Thành công
- ESP_FAIL: Lỗi chung
- ESP_ERR_NO_MEM: Hết bộ nhớ
- ESP_ERR_INVALID_ARG: Tham số không hợp lệ
- ESP_ERR_INVALID_STATE: Trạng thái không hợp lệ
- ESP_ERR_INVALID_SIZE: Kích thước không hợp lệ
- ESP_ERR_NOT_FOUND: Không tìm thấy
- ESP_ERR_NOT_SUPPORTED: Không hỗ trợ
- ESP_ERR_TIMEOUT: Hết thời gian chờ

Định nghĩa error code riêng nếu cần với base offset để tránh trùng

---

## 8. CONFIG.H

### 8.1 Cấu trúc config.h

Phân chia theo nhóm:

- Hardware config (GPIO pins)
- WiFi config
- MQTT config
- Sensor config
- System config (watchdog, log level, stack size)
- Feature flags (enable/disable features)

### 8.2 Sử dụng config trong code

- Include config.h và sử dụng trực tiếp các macro
- Có thể tạo local define để code dễ đọc hơn

---

## 9. CHECKLIST TRƯỚC KHI COMMIT

- Code tuân thủ KISS, YAGNI, DRY
- Đặt tên biến, hàm theo quy tắc
- Tách module hợp lý, coupling thấp
- Xử lý lỗi đầy đủ, validation đầu vào
- Có mutex/semaphore cho shared resources
- Comment bằng tiếng Việt không dấu
- Không dùng emoji, markdown trong code
- Config tập trung trong config.h
- Đã test trên thiết bị thật
- Không có memory leak
- Log message rõ ràng, đúng log level

---

## 10. ANTI-PATTERNS - KHÔNG ĐƯỢC LÀM

### 10.1 Global variables không bảo vệ

- KHÔNG dùng biến global không có mutex/atomic
- PHẢI dùng static atomic hoặc mutex bảo vệ

### 10.2 Magic numbers

- KHÔNG dùng số trực tiếp trong code
- PHẢI định nghĩa macro/const với tên có ý nghĩa

### 10.3 Busy waiting

- KHÔNG dùng while loop chờ không có delay
- PHẢI dùng vTaskDelay, event group, hoặc semaphore

### 10.4 Không kiểm tra return value

- KHÔNG bỏ qua giá trị trả về của malloc, các hàm ESP-IDF
- PHẢI kiểm tra và xử lý lỗi

### 10.5 Hard-coded strings

- KHÔNG hard-code MQTT topic, URL trực tiếp
- PHẢI dùng macro/format string với device_id

---

## 11. PERFORMANCE GUIDELINES

### 11.1 ISR (Interrupt Service Routine)

- ISR phải ngắn gọn, không blocking
- Chỉ gửi signal trong ISR, xử lý trong task
- Đánh dấu hàm ISR với IRAM_ATTR
- Sử dụng xQueueSendFromISR, không dùng hàm thường

### 11.2 String operations

- Dùng snprintf thay sprintf để tránh buffer overflow
- Tránh strcat trong vòng lặp (O(n²))
- Dùng offset + snprintf cho việc nối chuỗi hiệu quả

### 11.3 Task stack size

- Dùng uxTaskGetStackHighWaterMark để kiểm tra stack usage
- Đặt stack size phù hợp, không quá lớn gây lãng phí RAM

---

## 12. TESTING GUIDELINES

### 12.1 Unit test structure

- Sử dụng Unity framework
- Định nghĩa setUp() và tearDown() cho mỗi test file
- Đặt tên test case mô tả rõ scenario
- Test cả happy path và error cases

---

## 13. WIFI MANAGEMENT

### 13.1 Khởi tạo WiFi

- Khởi tạo NVS trước khi init WiFi
- Tạo default event loop trước khi init WiFi
- Sử dụng WiFi station mode cho kết nối internet
- Lưu credentials vào NVS, không hardcode

### 13.2 Event Handling

- Xử lý WIFI_EVENT_STA_START: bắt đầu connect
- Xử lý WIFI_EVENT_STA_CONNECTED: đã kết nối AP
- Xử lý WIFI_EVENT_STA_DISCONNECTED: mất kết nối, cần reconnect
- Xử lý IP_EVENT_STA_GOT_IP: đã có IP, sẵn sàng giao tiếp

### 13.3 Reconnection Strategy

- Implement exponential backoff cho reconnect
- Bắt đầu với delay nhỏ (1-2 giây), tăng dần đến max (30-60 giây)
- Reset delay về min khi kết nối thành công
- Giới hạn số lần retry hoặc retry vô hạn tùy use case
- Log rõ ràng trạng thái kết nối

### 13.4 WiFi State Machine

- Định nghĩa các state: DISCONNECTED, CONNECTING, CONNECTED, RECONNECTING
- Chuyển state rõ ràng khi có event
- Notify các module khác khi state thay đổi
- Không thực hiện network operations khi chưa có IP

---

## 14. MQTT MANAGEMENT

### 14.1 Khởi tạo MQTT

- Chờ WiFi connected và có IP trước khi init MQTT
- Cấu hình broker URI, port, credentials từ config
- Cấu hình keepalive phù hợp (30-60 giây)
- Cấu hình Last Will Testament (LWT) cho offline detection

### 14.2 Event Handling

- MQTT_EVENT_CONNECTED: subscribe các topic cần thiết
- MQTT_EVENT_DISCONNECTED: đánh dấu trạng thái, chờ reconnect tự động
- MQTT_EVENT_DATA: parse và xử lý message nhận được
- MQTT_EVENT_ERROR: log error, xử lý theo loại lỗi

### 14.3 QoS Guidelines

- QoS 0: Fire and forget, dùng cho dữ liệu telemetry thường xuyên
- QoS 1: At least once, dùng cho commands và dữ liệu quan trọng
- QoS 2: Exactly once, hiếm khi cần, tốn tài nguyên
- Ưu tiên QoS 1 cho hầu hết use cases

### 14.4 Topic Naming Convention

- Cấu trúc: {project}/{device_type}/{device_id}/{function}
- Ví dụ: factory/sensor/device001/telemetry
- Command topic: .../command
- Response topic: .../response
- Status topic: .../status

### 14.5 Message Format

- Sử dụng JSON cho payload
- Bao gồm timestamp trong message
- Bao gồm device_id để tracing
- Giới hạn kích thước message (khuyến nghị dưới 1KB)

### 14.6 Reconnection

- MQTT client tự động reconnect khi mất kết nối
- Cấu hình reconnect timeout phù hợp
- Re-subscribe các topic sau khi reconnect
- Không publish khi chưa connected

---

## 15. OTA UPDATE

### 15.1 OTA Architecture

- Sử dụng 2 OTA partitions (ota_0, ota_1)
- Kiểm tra partition table có đủ space cho firmware
- Lưu version hiện tại vào NVS hoặc app description

### 15.2 OTA Process

- Kiểm tra phiên bản mới trước khi download
- Download firmware về OTA partition không active
- Validate firmware sau khi download (checksum, signature)
- Set boot partition sang partition mới
- Reboot để apply update

### 15.3 Rollback Mechanism

- Sử dụng esp_ota_mark_app_valid_cancel_rollback() sau khi boot thành công
- Nếu firmware mới lỗi, tự động rollback về firmware cũ
- Định nghĩa tiêu chí "boot thành công" (WiFi connected, MQTT connected...)
- Timeout cho việc mark valid (30-60 giây)

### 15.4 Security

- Sử dụng HTTPS cho OTA server
- Verify certificate của server
- Sign firmware và verify signature trước khi apply
- Không cho phép downgrade version (tùy chọn)

### 15.5 Progress Reporting

- Report progress qua MQTT hoặc callback
- Log rõ ràng các bước OTA
- Thông báo kết quả (success/failure) sau khi hoàn tất

---

## 16. WATCHDOG

### 16.1 Task Watchdog Timer (TWDT)

- Enable TWDT trong menuconfig
- Subscribe các task quan trọng vào TWDT
- Reset watchdog định kỳ trong main loop của mỗi task
- Timeout phù hợp (10-30 giây tùy task)

### 16.2 Interrupt Watchdog Timer (IWDT)

- Tự động enable bởi ESP-IDF
- Đảm bảo không có interrupt handler chạy quá lâu
- Đảm bảo không disable interrupt quá lâu

### 16.3 Best Practices

- Không feed watchdog trong infinite loop không có thoát
- Mỗi task tự chịu trách nhiệm feed watchdog của mình
- Log warning trước khi watchdog timeout nếu có thể
- Phân tích panic dump để tìm nguyên nhân watchdog reset

---

## 17. VERSION MANAGEMENT

### 17.1 Firmware Version

- Định nghĩa version trong config.h: FIRMWARE_VERSION_MAJOR, MINOR, PATCH
- Format: "vX.Y.Z" (ví dụ: v1.2.3)
- Tăng PATCH cho bug fixes
- Tăng MINOR cho features mới backward compatible
- Tăng MAJOR cho breaking changes

### 17.2 Version trong Code

- Lưu version trong app_description của firmware
- Report version qua MQTT khi boot
- Log version khi khởi động
- So sánh version khi OTA để quyết định update

### 17.3 Hardware Version

- Định nghĩa HARDWARE_VERSION nếu có nhiều phiên bản board
- Kiểm tra hardware version để enable/disable features

---

## 18. BOOT SEQUENCE

### 18.1 Thứ tự khởi tạo

1. NVS init
2. Load config từ NVS
3. GPIO init
4. Peripherals init (I2C, SPI, UART)
5. Event loop init
6. WiFi init và connect
7. Chờ IP
8. MQTT init và connect
9. Subscribe MQTT topics
10. Start application tasks
11. Mark OTA valid (nếu dùng OTA)

### 18.2 Xử lý lỗi khi boot

- Nếu load config lỗi: dùng default config
- Nếu WiFi không connect được: retry với backoff
- Nếu MQTT không connect được: retry, vẫn cho phép hoạt động offline
- Log chi tiết để debug boot issues

### 18.3 Boot Reason

- Đọc boot reason với esp_reset_reason()
- Log boot reason khi khởi động
- Xử lý khác nhau cho power-on reset, watchdog reset, panic reset
- Đếm số lần boot liên tục để detect boot loop

---

## 19. DEBUG GUIDELINES

### 19.1 Debug Tools

- Serial monitor cho log output
- ESP-IDF monitor với decode panic
- GDB qua JTAG cho debug sâu
- Core dump để phân tích crash

### 19.2 Debug Configuration

- Tăng log level khi debug (ESP_LOG_DEBUG hoặc ESP_LOG_VERBOSE)
- Enable core dump trong menuconfig
- Giữ CONFIG_OPTIMIZATION_LEVEL_DEBUG khi debug
- Dùng CONFIG_OPTIMIZATION_LEVEL_RELEASE cho production

### 19.3 Common Issues

- Stack overflow: tăng stack size, kiểm tra local variables lớn
- Heap exhaustion: kiểm tra memory leak, tối ưu allocation
- Watchdog reset: tìm task bị block, kiểm tra infinite loop
- Guru Meditation: phân tích panic dump, kiểm tra null pointer

### 19.4 Memory Debugging

- Dùng heap_caps_get_free_size() để monitor free heap
- Dùng heap_caps_check_integrity() để detect corruption
- Enable heap tracing để tìm memory leak
- Log heap usage định kỳ trong system monitor

---

## 20. POWER MANAGEMENT

### 20.1 Light Sleep

- Sử dụng khi cần tiết kiệm pin nhưng vẫn cần phản hồi nhanh
- WiFi có thể duy trì kết nối với DTIM
- Wake up bằng GPIO, timer, hoặc UART

### 20.2 Deep Sleep

- Sử dụng khi cần tiết kiệm pin tối đa
- Mất toàn bộ RAM, chỉ giữ RTC memory
- Lưu state quan trọng vào RTC memory hoặc NVS trước khi sleep
- Wake up bằng timer, GPIO, touch, ULP

### 20.3 Best Practices

- Tắt peripherals không dùng trước khi sleep
- Cấu hình GPIO hold state khi sleep
- Tính toán wake up time phù hợp với use case
- Test kỹ power consumption với power profiler

---

## 21. SECURITY

### 21.1 NVS Encryption

- Enable NVS encryption cho dữ liệu nhạy cảm
- Sử dụng flash encryption key
- Không lưu plain-text credentials

### 21.2 Secure Boot

- Enable secure boot cho production
- Sign firmware với private key
- Giữ private key an toàn, không commit vào git

### 21.3 Communication Security

- Sử dụng TLS cho MQTT (port 8883)
- Verify server certificate
- Sử dụng client certificate nếu cần mutual TLS
- Không disable certificate verification trong production

### 21.4 Credentials Management

- Không hardcode credentials trong code
- Lưu credentials vào NVS encrypted
- Cho phép thay đổi credentials qua provisioning
- Rotate credentials định kỳ nếu có thể

---

## 22. PARTITION TABLE

### 22.1 Cấu trúc khuyến nghị

- nvs: 24KB - NVS storage
- phy_init: 4KB - PHY calibration data
- factory: optional - Factory firmware
- ota_0: firmware size - OTA slot 0
- ota_1: firmware size - OTA slot 1
- nvs_key: 4KB - NVS encryption key (nếu dùng encryption)
- coredump: 64KB - Core dump storage (optional)

### 22.2 Size Planning

- Dự trù firmware size tối đa
- OTA partition phải đủ lớn cho firmware + metadata
- Tổng size không vượt quá flash size
- Để dư ít nhất 10% cho future growth

---

Kết thúc AI Agent Rules cho ESP-IDF Firmware Development.
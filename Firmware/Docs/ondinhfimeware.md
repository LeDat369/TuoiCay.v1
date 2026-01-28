# Kế Hoạch Ổn Định Firmware (Production-Ready)

Mục tiêu: Chuẩn hóa, ổn định và triển khai quy trình phát hành firmware cho sản phẩm (bao gồm versioning, bootloader an toàn, OTA, rollback, CI/CD và kiểm thử) để sẵn sàng cho sản xuất và vận hành.

Phạm vi: ESP8266/ESP32 (ưu tiên ESP32 nếu cần secure boot/partition), PlatformIO/Arduino hoặc ESP-IDF.

1. Yêu cầu chính
- Firmware ổn định, deterministic builds.
- Quản lý phiên bản (semantic versioning) và ghi chép release.
- Bootloader an toàn / signed firmware (nếu phần cứng hỗ trợ).
- OTA an toàn (HTTPS + signature verification) với khả năng rollback khi lỗi.
- Quá trình build & release tự động (CI/CD) kèm artifact lưu trữ.
- Kiểm thử tự động + test harness cho production test jig.

2. Tổng quan kiến trúc cập nhật
- Partition/firmware layout: bootloader + slot A/B (nếu HW hỗ trợ) hoặc fallback partition.
- Storage: LittleFS for config/log; persistent metadata for current version and last known-good.
- OTA server: HTTPS endpoint hosting signed binaries + metadata (version, checksum, signature).
- Update client: download -> verify signature -> write to other slot -> set boot flag -> reboot.

3. Bước thực hiện (giai đoạn & nhiệm vụ)

Phase A — Chuẩn bị môi trường (1–2 ngày)
- Chọn target platform: ESP8266 (hiện có) hoặc chuyển sang ESP32 nếu cần secure boot.
- Quy định toolchain: dùng PlatformIO với versions pin (platform, framework) hoặc ESP-IDF release cố định.
- Thiết lập reproducible build: lock toolchain, ghi `platformio.ini`/scripts build wrapper.

Phase B — Quản lý phiên bản & release (0.5 ngày)
- Áp `semver` (MAJOR.MINOR.PATCH). Tag release bằng git tag `v1.2.3`.
- Lưu CHANGELOG cho mỗi release.
- Lưu artifact build (firmware.bin, firmware.bin.sig, manifest.json) trong storage (GitHub Releases/Artifactory).

Phase C — Bootloader & Partitioning (1–2 ngày)
- Nếu dùng ESP32: enable partition table with app slot A/B; sử dụng esp-idf bootloader hoặc MCU boot with OTA.
- Nếu ESP8266: dùng Arduino Update library with safe update pattern + marker file to indicate success.
- Thiết kế marker/metadata: on successful boot set `last_good_version` trong LittleFS/NVS.

Phase D — Signing & Verification (1 ngày)
- Tạo keypair (Ed25519 hoặc RSA). Private key chỉ lưu trên CI signing server.
- Khi build release: ký firmware, tạo signature file (`firmware.bin.sig`) và manifest (version, checksum, signature_url).
- Client verify signature trước khi flash.

Phase E — OTA design & client (1–2 ngày)
- Mechanism: HTTP(S) download with Range / resume support.
- Verify checksum + signature before applying.
- Write to alternate slot (if available) or write and validate then reboot.
- After boot, set `last_good_version`; else auto rollback to previous version.

Phase F — Rollback & Safety (0.5–1 ngày)
- Record previous version metadata before update.
- If new image fails (no heartbeat within X seconds), bootloader/monitor revert to previous image automatically.
- Limit update retries to N attempts, log events to LittleFS.

Phase G — CI/CD & Reproducible Release (1–2 ngày)
- CI pipeline (GitHub Actions / GitLab CI): build, run static checks, run unit tests, sign artifacts, publish release artifacts.
- Store artifacts with immutable names (firmware-v1.2.3.bin, firmware-v1.2.3.bin.sig).
- Example steps: checkout -> set up toolchain -> build -> run unit tests -> sign -> upload release.

Phase H — Test & Validation (2–3 ngày)
- Unit tests (logic control) using Unity/ArduinoUnit.
- Integration tests on device: boot test, update test, rollback test, power-fail during update.
- Create test jig for production to verify boot, ADC reading, GPIO, pump drive, OTP.

4. Quy trình release (nén gọn)
- Dev: merge PR -> CI runs -> build artifacts -> create release candidate tag `vX.Y.Z-rc1`.
- QA: flash RC batch, run integration tests và field pilot.
- Release: khi QA pass, ký và publish final `vX.Y.Z` artifacts và cập nhật OTA manifest.

5. Chi tiết kỹ thuật cần implement trong firmware
- Metadata file: `/fw_manifest.json` (fields: version, build_time, checksum, signature_url)
- Verify steps: checksum -> signature -> write.
- Heartbeat/health timer: on first boot after update, run health checks và set last_good_version.
- Safe write: use `Update` API (Arduino) with `Update.begin(size)` + `Update.writeStream()` và `Update.end()` checks.
- Logging: append update events to LittleFS for diagnostics.

6. Bảo mật
- Ký firmware với private key trên CI; public key nhúng vào firmware (read-only) để verify.
- OTA server dùng HTTPS + HSTS; manifest files signed.
- Không lưu private key trong repo; rotate keys theo chính sách.

7. Monitoring & Telemetry (sau khi deploy)
- Ghi event update/log vào LittleFS và upload khi có kết nối (optional).
- Nếu có dịch vụ backend, gửi periodic ping with version and health.

8. Tài liệu & artifacts cần chuẩn bị
- Script build reproducible, tài liệu build/release, private signing server notes, test plan, rollback plan, production test jig schematic.

9. Timeline ước lượng (total ~8–12 ngày tích lũy với 1 dev)
- Mô tả ở Phase A..H bên trên cộng testing pilot.

10. Checklist trước khi phát hành hàng loạt
- Reproducible build OK
- OTA signed & verified
- Rollback validated
- Test jig hoạt động và chạy batch test
- Documentation (release notes, upgrade instructions)

11. Lệnh & ví dụ nhanh
- Tag và release:
  - `git tag -a v1.2.3 -m "Release v1.2.3"`
  - `git push origin v1.2.3`
- CI: export signed artifact and upload to release storage.
- Client verify pseudocode:
  - download manifest -> download firmware.bin and firmware.bin.sig
  - verify signature(firmware.bin, firmware.bin.sig, public_key)
  - if ok -> apply update

---
Ghi chú: Nếu cần hỗ trợ chi tiết cho một task cụ thể (ví dụ: thiết lập CI/GHA để ký firmware, hoặc mã hoá/verify signature trên device), nói rõ tôi sẽ tạo patch code / workflow mẫu.

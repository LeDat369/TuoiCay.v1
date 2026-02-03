#!/usr/bin/env python3
"""
Script tự động tìm ESP8266 trên mạng và upload qua OTA
"""

import subprocess
import sys
import re

def find_esp8266_ip():
    """Tìm IP của ESP8266 bằng cách scan ARP table"""
    print("🔍 Đang tìm ESP8266 trên mạng...")
    
    # Lấy ARP table
    result = subprocess.run(['arp', '-a'], capture_output=True, text=True)
    
    # ESP8266 MAC address thường có OUI: 
    # - Espressif: 24:0a:c4, 30:ae:a4, 84:f3:eb, ...
    esp_oui_patterns = [
        r'24-0a-c4', r'30-ae-a4', r'84-f3-eb', 
        r'cc-50-e3', r'dc-4f-22', r'ec-fa-bc'
    ]
    
    lines = result.stdout.split('\n')
    candidates = []
    
    for line in lines:
        for pattern in esp_oui_patterns:
            if pattern in line.lower():
                # Extract IP address
                match = re.search(r'(\d+\.\d+\.\d+\.\d+)', line)
                if match:
                    ip = match.group(1)
                    if ip not in candidates:
                        candidates.append(ip)
                        print(f"   ✅ Tìm thấy: {ip}")
    
    if not candidates:
        print("   ❌ Không tìm thấy ESP8266!")
        print("\n💡 Tip: Đảm bảo ESP8266 đã kết nối WiFi và cùng mạng với máy tính")
        return None
    
    if len(candidates) == 1:
        return candidates[0]
    
    # Multiple devices found
    print(f"\n⚠️  Tìm thấy {len(candidates)} thiết bị:")
    for i, ip in enumerate(candidates, 1):
        print(f"   {i}. {ip}")
    
    choice = input("\nChọn thiết bị (1-{}): ".format(len(candidates)))
    try:
        idx = int(choice) - 1
        if 0 <= idx < len(candidates):
            return candidates[idx]
    except:
        pass
    
    return None

def test_connection(ip):
    """Test kết nối đến ESP8266"""
    print(f"\n🔌 Kiểm tra kết nối đến {ip}...")
    result = subprocess.run(['ping', '-n', '1', ip], capture_output=True)
    return result.returncode == 0

def upload_ota(ip, password="tuoicay123"):
    """Upload firmware qua OTA"""
    print(f"\n🚀 Đang upload firmware qua OTA đến {ip}...")
    print("=" * 60)
    
    cmd = [
        'pio', 'run', 
        '-e', 'nodemcuv2_ota',
        '--target', 'upload',
        '--upload-port', ip,
    ]
    
    # Override platformio.ini settings
    result = subprocess.run(cmd)
    
    if result.returncode == 0:
        print("\n✅ Upload thành công!")
        print("💡 ESP8266 sẽ tự động reboot")
        return True
    else:
        print("\n❌ Upload thất bại!")
        print("💡 Kiểm tra:")
        print("   1. ESP8266 có đang chạy?")
        print("   2. Password OTA đúng chưa?")
        print("   3. Firewall có chặn không?")
        return False

def main():
    print("""
╔════════════════════════════════════════════════════════════╗
║           TuoiCay OTA Upload Helper Script                 ║
║                                                            ║
║  Script này sẽ:                                            ║
║  1. Tự động tìm ESP8266 trên mạng                          ║
║  2. Test kết nối                                           ║
║  3. Upload firmware qua OTA                                ║
╚════════════════════════════════════════════════════════════╝
    """)
    
    # Find ESP8266
    ip = find_esp8266_ip()
    if not ip:
        sys.exit(1)
    
    print(f"\n📍 Sử dụng IP: {ip}")
    
    # Test connection
    if not test_connection(ip):
        print("❌ Không thể ping đến thiết bị!")
        print("💡 Kiểm tra:")
        print("   1. ESP8266 có đang bật?")
        print("   2. WiFi có hoạt động?")
        print("   3. Cùng subnet không?")
        sys.exit(1)
    
    print("✅ Kết nối OK!")
    
    # Confirm upload
    password = input("\n🔐 Nhập OTA password [tuoicay123]: ").strip() or "tuoicay123"
    
    confirm = input(f"\n⚠️  Upload firmware đến {ip}? (y/N): ").strip().lower()
    if confirm != 'y':
        print("❌ Đã hủy")
        sys.exit(0)
    
    # Upload
    success = upload_ota(ip, password)
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n❌ Đã hủy bởi người dùng")
        sys.exit(1)

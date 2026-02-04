# TuoiCay OTA Upload Helper (PowerShell)
# Script tự động tìm và upload firmware qua OTA

param(
    [string]$IP = "",
    [string]$Password = "tuoicay123"
)

Write-Host @"

╔════════════════════════════════════════════════════════════╗
║           TuoiCay OTA Upload Helper (PowerShell)           ║
║                                                            ║
║  Script này sẽ:                                            ║
║  1. Tự động tìm ESP8266 trên mạng                          ║
║  2. Test kết nối                                           ║
║  3. Upload firmware qua OTA                                ║
╚════════════════════════════════════════════════════════════╝

"@ -ForegroundColor Cyan

# Function to find ESP8266 on network
function Find-ESP8266 {
    Write-Host "🔍 Đang tìm ESP8266 trên mạng..." -ForegroundColor Yellow
    
    # Get ARP table
    $arp = arp -a | Out-String
    
    # ESP8266 MAC OUI patterns (Espressif)
    $patterns = @(
        '24-0a-c4', '30-ae-a4', '84-f3-eb',
        'cc-50-e3', 'dc-4f-22', 'ec-fa-bc'
    )
    
    $candidates = @()
    
    foreach ($pattern in $patterns) {
        if ($arp -match $pattern) {
            # Extract IP addresses
            $matches = [regex]::Matches($arp, "(\d+\.\d+\.\d+\.\d+)\s+$pattern")
            foreach ($match in $matches) {
                $ip = $match.Groups[1].Value
                if ($candidates -notcontains $ip) {
                    $candidates += $ip
                    Write-Host "   ✅ Tìm thấy: $ip" -ForegroundColor Green
                }
            }
        }
    }
    
    if ($candidates.Count -eq 0) {
        Write-Host "   ❌ Không tìm thấy ESP8266!" -ForegroundColor Red
        Write-Host "`n💡 Tip: Đảm bảo ESP8266 đã kết nối WiFi và cùng mạng với máy tính" -ForegroundColor Yellow
        return $null
    }
    
    if ($candidates.Count -eq 1) {
        return $candidates[0]
    }
    
    # Multiple devices found
    Write-Host "`n⚠️  Tìm thấy $($candidates.Count) thiết bị:" -ForegroundColor Yellow
    for ($i = 0; $i -lt $candidates.Count; $i++) {
        Write-Host "   $($i + 1). $($candidates[$i])"
    }
    
    $choice = Read-Host "`nChọn thiết bị (1-$($candidates.Count))"
    $idx = [int]$choice - 1
    
    if ($idx -ge 0 -and $idx -lt $candidates.Count) {
        return $candidates[$idx]
    }
    
    return $null
}

# Function to test connection
function Test-ESP8266Connection {
    param([string]$IP)
    
    Write-Host "`n🔌 Kiểm tra kết nối đến $IP..." -ForegroundColor Yellow
    $ping = Test-Connection -ComputerName $IP -Count 1 -Quiet
    
    if ($ping) {
        Write-Host "✅ Kết nối OK!" -ForegroundColor Green
        return $true
    } else {
        Write-Host "❌ Không thể ping đến thiết bị!" -ForegroundColor Red
        Write-Host "💡 Kiểm tra:" -ForegroundColor Yellow
        Write-Host "   1. ESP8266 có đang bật?"
        Write-Host "   2. WiFi có hoạt động?"
        Write-Host "   3. Cùng subnet không?"
        return $false
    }
}

# Function to upload OTA
function Start-OTAUpload {
    param(
        [string]$IP,
        [string]$Password
    )
    
    Write-Host "`n🚀 Đang upload firmware qua OTA đến $IP..." -ForegroundColor Cyan
    Write-Host ("=" * 60)
    
    # Tạm thời sửa platformio.ini
    $iniPath = "platformio.ini"
    $iniBackup = "platformio.ini.bak"
    
    # Backup
    Copy-Item $iniPath $iniBackup -Force
    
    try {
        # Đọc nội dung
        $content = Get-Content $iniPath -Raw
        
        # Tìm và uncomment OTA settings trong env:nodemcuv2_ota
        $content = $content -replace '; upload_port = 192\.168\.1\.100', "upload_port = $IP"
        $content = $content -replace '; upload_flags =', 'upload_flags ='
        $authLine = "    --auth=$Password"
        $content = $content -replace ';     `-`-auth=tuoicay123', $authLine
        
        # Ghi lại
        $content | Set-Content $iniPath -NoNewline
        
        Write-Host "⚙️  Đã cấu hình OTA settings" -ForegroundColor Gray
        
        # Upload
        $result = & pio run -e nodemcuv2_ota --target upload
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "`n✅ Upload thành công!" -ForegroundColor Green
            Write-Host "💡 ESP8266 sẽ tự động reboot" -ForegroundColor Yellow
            return $true
        } else {
            Write-Host "`n❌ Upload thất bại!" -ForegroundColor Red
            Write-Host "💡 Kiểm tra:" -ForegroundColor Yellow
            Write-Host "   1. ESP8266 có đang chạy?"
            Write-Host "   2. Password OTA đúng chưa? (hiện tại: $Password)"
            Write-Host "   3. Firewall có chặn không?"
            return $false
        }
    }
    finally {
        # Restore backup
        Move-Item $iniBackup $iniPath -Force
        Write-Host "`n🔄 Đã khôi phục platformio.ini" -ForegroundColor Gray
    }
}

# Main script
try {
    # Find or use provided IP
    if ([string]::IsNullOrEmpty($IP)) {
        $IP = Find-ESP8266
        if ([string]::IsNullOrEmpty($IP)) {
            exit 1
        }
    }
    
    Write-Host "`n📍 Sử dụng IP: $IP" -ForegroundColor Cyan
    
    # Test connection
    if (-not (Test-ESP8266Connection -IP $IP)) {
        exit 1
    }
    
    # Confirm
    Write-Host "`n⚠️  Upload firmware đến $IP với password '$Password'?" -ForegroundColor Yellow
    $confirm = Read-Host "Tiếp tục? (y/N)"
    
    if ($confirm -ne 'y') {
        Write-Host "❌ Đã hủy" -ForegroundColor Red
        exit 0
    }
    
    # Upload
    $success = Start-OTAUpload -IP $IP -Password $Password
    
    if ($success) {
        exit 0
    } else {
        exit 1
    }
}
catch {
    Write-Host "`n❌ Lỗi: $_" -ForegroundColor Red
    exit 1
}

# Quick OTA Upload Script
param(
    [string]$IP = "",
    [string]$Password = "tuoicay2026"
)

Write-Host "`n=== TuoiCay OTA Upload ===" -ForegroundColor Cyan

# Find ESP8266 if no IP provided
if ([string]::IsNullOrEmpty($IP)) {
    Write-Host "Đang tìm ESP8266..." -ForegroundColor Yellow
    
    $arp = arp -a | Out-String
    $patterns = @('24-0a-c4', '30-ae-a4', '84-f3-eb', 'cc-50-e3', 'dc-4f-22', 'ec-fa-bc')
    
    foreach ($pattern in $patterns) {
        $matches = [regex]::Matches($arp, "(\d+\.\d+\.\d+\.\d+)\s+$pattern")
        if ($matches.Count -gt 0) {
            $IP = $matches[0].Groups[1].Value
            Write-Host "Tìm thấy ESP8266: $IP" -ForegroundColor Green
            break
        }
    }
    
    if ([string]::IsNullOrEmpty($IP)) {
        Write-Host "Không tìm thấy ESP8266!" -ForegroundColor Red
        Write-Host "Vui lòng chạy: .\ota_quick.ps1 -IP <địa_chỉ_IP>" -ForegroundColor Yellow
        exit 1
    }
}

Write-Host "IP: $IP" -ForegroundColor Cyan
Write-Host "Password: $Password" -ForegroundColor Cyan

# Test connection
Write-Host "`nTest kết nối..." -ForegroundColor Yellow
$ping = Test-Connection -ComputerName $IP -Count 1 -Quiet
if (-not $ping) {
    Write-Host "Không thể kết nối đến $IP" -ForegroundColor Red
    exit 1
}
Write-Host "Kết nối OK" -ForegroundColor Green

# Update platformio.ini
Write-Host "`nCấu hình OTA..." -ForegroundColor Yellow
Copy-Item "platformio.ini" "platformio.ini.bak" -Force

try {
    $content = Get-Content "platformio.ini" -Raw
    
    # Find [env:nodemcuv2_ota] section and update
    if ($content -match '\[env:nodemcuv2_ota\]') {
        $content = $content -replace '; upload_port = .*', "upload_port = $IP"
        $content = $content -replace '; upload_flags =', 'upload_flags ='
        $content = $content -replace ';     --auth=.*', "    --auth=$Password"
        
        $content | Set-Content "platformio.ini" -NoNewline
        Write-Host "Đã cấu hình OTA settings" -ForegroundColor Green
    }
    
    # Upload
    Write-Host "`nUpload firmware..." -ForegroundColor Yellow
    Write-Host ("=" * 50)
    
    python -m platformio run -e nodemcuv2_ota --target upload
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`n✅ Upload thành công!" -ForegroundColor Green
        Write-Host "ESP8266 sẽ tự động reboot" -ForegroundColor Yellow
    } else {
        Write-Host "`n❌ Upload thất bại!" -ForegroundColor Red
        Write-Host "Kiểm tra: ESP8266 đang chạy? Password đúng? Firewall?" -ForegroundColor Yellow
    }
}
finally {
    # Restore backup
    Move-Item "platformio.ini.bak" "platformio.ini" -Force
    Write-Host "Đã khôi phục platformio.ini" -ForegroundColor Gray
}

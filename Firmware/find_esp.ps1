# Quick ESP8266 Finder
Write-Host "`n=== Tìm ESP8266 trên mạng ===" -ForegroundColor Cyan

# Get local subnet
$localIP = (Get-NetIPAddress -AddressFamily IPv4 | Where-Object {$_.InterfaceAlias -notlike "*Loopback*" -and $_.IPAddress -like "192.168.*"} | Select-Object -First 1).IPAddress

if (-not $localIP) {
    Write-Host "Không tìm thấy mạng local!" -ForegroundColor Red
    exit 1
}

$subnet = $localIP -replace '\.\d+$',''
Write-Host "Subnet: $subnet.0/24" -ForegroundColor Yellow
Write-Host "Đang quét (có thể mất 30-60s)...`n" -ForegroundColor Yellow

# Scan common IPs first (faster)
$commonIPs = @(100, 101, 102, 103, 104, 105, 110, 120, 150, 200)
$found = @()

foreach ($last in $commonIPs) {
    $ip = "$subnet.$last"
    if (Test-Connection -ComputerName $ip -Count 1 -Quiet) {
        Write-Host "  ✓ $ip" -ForegroundColor Green
        $found += $ip
        
        # Try to check if it's ESP8266 via HTTP
        try {
            $response = Invoke-WebRequest -Uri "http://${ip}/" -TimeoutSec 2 -ErrorAction SilentlyContinue
            if ($response.Content -match "TuoiCay|ESP8266") {
                Write-Host "    -> Có thể là ESP8266!" -ForegroundColor Cyan
            }
        } catch {}
    }
}

if ($found.Count -eq 0) {
    Write-Host "`nKhong tim thay thiet bi!" -ForegroundColor Red
    Write-Host "Vui long cung cap IP thu cong:" -ForegroundColor Yellow
    Write-Host '  .\ota_quick.ps1 -IP 192.168.x.x' -ForegroundColor Gray
} else {
    Write-Host "`nTim thay $($found.Count) thiet bi" -ForegroundColor Green
    $firstIP = $found[0]
    Write-Host "`nThu upload OTA voi IP dau tien: $firstIP" -ForegroundColor Yellow
    Write-Host "Chay: .\ota_quick.ps1 -IP $firstIP" -ForegroundColor Cyan
}

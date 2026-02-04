# ESP8266 Network Scanner
Write-Host ""
Write-Host "=== Tim ESP8266 tren mang ===" -ForegroundColor Cyan

$localIP = (Get-NetIPAddress -AddressFamily IPv4 | Where-Object {$_.InterfaceAlias -notlike "*Loopback*" -and $_.IPAddress -like "192.168.*"} | Select-Object -First 1).IPAddress

if (-not $localIP) {
    Write-Host "Khong tim thay mang local!" -ForegroundColor Red
    exit 1
}

$subnet = $localIP -replace '\.\d+$',''
Write-Host "Subnet: $subnet.0/24" -ForegroundColor Yellow
Write-Host "Dang quet cac IP pho bien..." -ForegroundColor Yellow
Write-Host ""

$commonIPs = @(100, 101, 102, 103, 104, 105, 110, 120, 150, 200)
$found = @()

foreach ($last in $commonIPs) {
    $ip = "$subnet.$last"
    if (Test-Connection -ComputerName $ip -Count 1 -Quiet) {
        Write-Host "  Found: $ip" -ForegroundColor Green
        $found += $ip
        
        try {
            $response = Invoke-WebRequest -Uri "http://${ip}/" -TimeoutSec 2 -ErrorAction SilentlyContinue
            if ($response.Content -match "TuoiCay|ESP8266") {
                Write-Host "    --> ESP8266 detected!" -ForegroundColor Cyan
            }
        } catch {}
    }
}

Write-Host ""
if ($found.Count -eq 0) {
    Write-Host "Khong tim thay thiet bi nao!" -ForegroundColor Red
    Write-Host "Nhap IP thu cong:" -ForegroundColor Yellow
    $manualIP = Read-Host "IP cua ESP8266"
    if ($manualIP) {
        & ".\ota_quick.ps1" -IP $manualIP
    }
} else {
    Write-Host "Tim thay $($found.Count) thiet bi" -ForegroundColor Green
    $firstIP = $found[0]
    Write-Host "Dang thu upload voi IP: $firstIP" -ForegroundColor Yellow
    Write-Host ""
    & ".\ota_quick.ps1" -IP $firstIP
}

 Write-Host 'Stopping common serial apps...'
$names = @('Code','arduino','putty','TeraTerm','teraterm')
foreach($n in $names) {
  Get-Process -Name $n -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
}
$py = Get-CimInstance Win32_Process | Where-Object { ($_.Name -match 'python') -and ($_.CommandLine -match 'platformio|miniterm|esptool|serial') }
foreach($p in $py) {
  Write-Host ("Killing python pid {0}" -f $p.ProcessId)
  Stop-Process -Id $p.ProcessId -Force -ErrorAction SilentlyContinue
}
Write-Host 'Retrying upload...'
& 'C:\Users\Le Van Dat\.platformio\penv\Scripts\platformio.exe' run --target upload

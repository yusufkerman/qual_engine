# PowerShell script to generate Doxygen documentation
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

$doxygenExe = if (Test-Path "$ScriptDir\bin\doxygen.exe") {
    "$ScriptDir\bin\doxygen.exe"
} elseif (Get-Command doxygen -ErrorAction SilentlyContinue) {
    "doxygen"
} else {
    $null
}

if (-not $doxygenExe) {
    Write-Error "[HATA] Doxygen bulunamadı! 'docs\doxygen\bin\doxygen.exe' yolunda veya sistem PATH ortam değişkeninde bulunmalıdır."
    exit 1
}

Write-Host "[Doxygen] Modern karanlık temalı dokümantasyon oluşturuluyor..." -ForegroundColor Cyan
& $doxygenExe Doxyfile

if ($LASTEXITCODE -eq 0) {
    $htmlIndex = Join-Path $ScriptDir "output\html\index.html"
    Write-Host "[BAŞARILI] Dokümantasyon oluşturuldu: $htmlIndex" -ForegroundColor Green
    Start-Process $htmlIndex
} else {
    Write-Error "[HATA] Doxygen dokümantasyon oluşturulurken bir hata oluştu."
}

# =============================================================================
# QualEngine - Otomatik Dağıtım ve Paketleme Betiği
# Sürüm: v0.2.0-alpha
# =============================================================================

param(
    [string]$Version = "0.2.0-alpha",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

Write-Host "===============================================================" -ForegroundColor Cyan
Write-Host " QualEngine Release Paketleme Hattı (v$Version)" -ForegroundColor Cyan
Write-Host "===============================================================" -ForegroundColor Cyan

# 1. Aşama: CMake Release Derlemesi
if (-not $SkipBuild) {
    Write-Host "`n[1/4] Release Derlemesi Calistiriliyor (cmake --build --preset release)..." -ForegroundColor Yellow
    cmake --build --preset release
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Release derlemesi basarisiz oldu! Paketleme durduruldu."
        exit 1
    }
    Write-Host "  -> Release derlemesi basariyla tamamlandi." -ForegroundColor Green
} else {
    Write-Host "`n[1/4] Derleme adimi atlandi (--SkipBuild)." -ForegroundColor Gray
}

# Gerekli dosyaların varlık denetimi
$ReleaseBinDir = Join-Path $RepoRoot "x64\Release"
$AppExe = Join-Path $ReleaseBinDir "Application.exe"
$PlayerExe = Join-Path $ReleaseBinDir "GameRuntime.exe"
$AssimpDll = Join-Path $ReleaseBinDir "assimp-vc145-mt.dll"

if (-not (Test-Path $AppExe) -or -not (Test-Path $PlayerExe) -or -not (Test-Path $AssimpDll)) {
    Write-Error "Gerekli Release dosyalari (Application.exe, GameRuntime.exe, assimp-vc145-mt.dll) bulunamadi!"
    exit 1
}

# 2. Aşama: Dağıtım (Staging) Klasörünün Hazırlanması
Write-Host "`n[2/4] Dagitim Klasoru (dist/staging) Olusturuluyor..." -ForegroundColor Yellow
$DistDir = Join-Path $RepoRoot "dist"
$StagingDir = Join-Path $DistDir "staging"

if (Test-Path $StagingDir) {
    Remove-Item -Recurse -Force $StagingDir
}
New-Item -ItemType Directory -Force -Path $StagingDir | Out-Null

# Binary dosyalarını kopyala
Copy-Item $AppExe -Destination $StagingDir
Copy-Item $PlayerExe -Destination $StagingDir
Copy-Item $AssimpDll -Destination $StagingDir

# EngineResources klasörünü hazırla
$TargetResources = Join-Path $StagingDir "EngineResources"
$SourceResources = Join-Path $RepoRoot "Engine\Resources"
Copy-Item -Recurse -Force $SourceResources -Destination $TargetResources

# Temiz şablon proje klasörünü kopyala (DemoGame)
$TargetTemplates = Join-Path $StagingDir "Templates\DemoGame"
$SourceDemo = Join-Path $RepoRoot "Projects\DemoGame"
if (Test-Path $SourceDemo) {
    New-Item -ItemType Directory -Force -Path (Join-Path $StagingDir "Templates") | Out-Null
    Copy-Item -Recurse -Force $SourceDemo -Destination $TargetTemplates
    # Gereksiz derleme çıktılarını temizle
    $BinariesFolder = Join-Path $TargetTemplates "Binaries"
    if (Test-Path $BinariesFolder) { Remove-Item -Recurse -Force $BinariesFolder }
    $VsFolder = Join-Path $TargetTemplates "Scripts\.vs"
    if (Test-Path $VsFolder) { Remove-Item -Recurse -Force $VsFolder }
    $ObjFolder = Join-Path $TargetTemplates "Scripts\obj"
    if (Test-Path $ObjFolder) { Remove-Item -Recurse -Force $ObjFolder }
}

# Lisans ve Beni Oku dosyalarını ekle
$LicenseFile = Join-Path $RepoRoot "LICENSE"
if (Test-Path $LicenseFile) {
    Copy-Item $LicenseFile -Destination $StagingDir
}

$ReadmeContent = @"
===============================================================================
 QualEngine v$Version (Alpha Preview)
 Modern C++20 & OpenGL 4.5 Game Engine & Vesta Editor Suite
===============================================================================

Gereksinimler:
- Windows 10 / 11 (x64)
- OpenGL 4.5 Core Profile destekli GPU
- Visual C++ 2015-2022 Redistributable (x64)

Calistirma:
- Vesta Editor:   Application.exe
- Standalone:     GameRuntime.exe (Export edilmis oyun calistiricisi)

Dokumantasyon & Destek:
https://github.com/yusufkerman/qual_engine
===============================================================================
"@
Set-Content -Path (Join-Path $StagingDir "README.txt") -Value $ReadmeContent -Encoding UTF8

Write-Host "  -> Dagitim klasoru basariyla hazirlandi: $StagingDir" -ForegroundColor Green

# 3. Aşama: Taşınabilir Arşiv (Portable ZIP) Paketi Üretimi
Write-Host "`n[3/4] Portable ZIP Paketi Sikistiriliyor..." -ForegroundColor Yellow
$ZipFilename = "QualEngine-v$Version-win64.zip"
$ZipPath = Join-Path $DistDir $ZipFilename

if (Test-Path $ZipPath) {
    Remove-Item -Force $ZipPath
}

Compress-Archive -Path "$StagingDir\*" -DestinationPath $ZipPath -CompressionLevel Optimal
$ZipSizeMB = [math]::Round((Get-Item $ZipPath).Length / 1MB, 2)
Write-Host "  -> [BASARILI] Portable ZIP olusturuldu: $ZipPath ($ZipSizeMB MB)" -ForegroundColor Green

# 4. Aşama: Inno Setup ile Setup.exe Üretimi
Write-Host "`n[4/4] Inno Setup Denetleniyor ve Setup.exe Paketleniyor..." -ForegroundColor Yellow

$InnoCandidates = @(
    (Get-Command iscc -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source),
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    "C:\Program Files\Inno Setup 6\ISCC.exe",
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
)

$IsccPath = $null
foreach ($cand in $InnoCandidates) {
    if ($cand -and (Test-Path $cand)) {
        $IsccPath = $cand
        break
    }
}

$IssScript = Join-Path $RepoRoot "installer\qualengine.iss"

if ($IsccPath) {
    Write-Host "  -> Inno Setup derleyicisi bulundu: $IsccPath" -ForegroundColor Cyan
    & $IsccPath $IssScript
    if ($LASTEXITCODE -eq 0) {
        $SetupExe = Join-Path $DistDir "QualEngine-v$Version-Setup.exe"
        if (Test-Path $SetupExe) {
            $SetupSizeMB = [math]::Round((Get-Item $SetupExe).Length / 1MB, 2)
            Write-Host "  -> [BASARILI] Setup.exe olusturuldu: $SetupExe ($SetupSizeMB MB)" -ForegroundColor Green
        }
    } else {
        Write-Warning "Inno Setup derleme sirasinda hata verdi!"
    }
} else {
    Write-Host "  [BILGI] Inno Setup sistemi kurulu degil. Portable ZIP hazirlandi." -ForegroundColor Yellow
    Write-Host "  Setup.exe uretmek icin Inno Setup 6 kurabilirsiniz:" -ForegroundColor Gray
    Write-Host "    winget install JRSoftware.InnoSetup" -ForegroundColor Cyan
    Write-Host "    ardindan bu betigi tekrar calistiriniz: .\scripts\package_release.ps1" -ForegroundColor Gray
}

Write-Host "`n===============================================================" -ForegroundColor Cyan
Write-Host " PAKETLEME TAMAMLANDI!" -ForegroundColor Cyan
Write-Host " Cikti Dizini: $DistDir" -ForegroundColor Cyan
Write-Host "===============================================================" -ForegroundColor Cyan


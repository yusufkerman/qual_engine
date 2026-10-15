@echo off
setlocal
cd /d "%~dp0"

echo [Doxygen] Dokumantasyon olusturuluyor...

if exist "bin\doxygen.exe" (
    set "DOXYGEN_EXE=bin\doxygen.exe"
) else (
    where doxygen >nul 2>nul
    if %errorlevel% equ 0 (
        set "DOXYGEN_EXE=doxygen"
    ) else (
        echo [HATA] Doxygen bulunamadi! docs\doxygen\bin\doxygen.exe veya PATH icinde olmali.
        pause
        exit /b 1
    )
)

"%DOXYGEN_EXE%" Doxyfile

if %errorlevel% equ 0 (
    echo [BASARILI] Dokumantasyon olusturuldu: docs\doxygen\output\html\index.html
    echo Sayfayi acmak icin bir tusa basiniz...
    pause >nul
    start "" "output\html\index.html"
) else (
    echo [HATA] Doxygen calisirken bir sorun olustu.
    pause
)

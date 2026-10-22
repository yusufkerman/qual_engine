@echo off
setlocal EnableDelayedExpansion
set DEVENV_EXE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.com
if not exist "%DEVENV_EXE%" (
    for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -find Common7\IDE\devenv.com`) do (
        set "DEVENV_EXE=%%i"
    )
)
if exist "!DEVENV_EXE!" (
    "!DEVENV_EXE!" "%~dp0GameModule.vcxproj" /Build "Debug|x64"
    exit /b !ERRORLEVEL!
)

set MSBUILD_EXE=C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe
if not exist "%MSBUILD_EXE%" (
    for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
        set "MSBUILD_EXE=%%i"
    )
)
if exist "!MSBUILD_EXE!" (
    "!MSBUILD_EXE!" "%~dp0GameModule.vcxproj" /p:Configuration=Debug /p:Platform=x64 /nodeReuse:false /m /v:m
    exit /b !ERRORLEVEL!
)

echo [ERROR] Neither devenv.com nor MSBuild.exe could be located!
exit /b 1

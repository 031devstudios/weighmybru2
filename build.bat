@echo off
setlocal enabledelayedexpansion

rem WeighMyBru² Build Script for Windows
rem This script builds firmware for both board variants with proper versioning

set "VERSION="
set "BUILD_NUMBER="
set "OUTPUT_DIR=build-output"
set "CLEAN_BUILD="
set "IS_RELEASE="

rem Get current timestamp for build number if not specified
for /f "tokens=1-4 delims=/ " %%a in ('date /t') do (
    for /f "tokens=1-2 delims=: " %%e in ('time /t') do (
        set "BUILD_NUMBER=%%a%%b%%c%%e%%f"
    )
)

rem Get git commit hash
for /f "tokens=*" %%i in ('git rev-parse --short HEAD 2^>nul') do set "COMMIT_HASH=%%i"
if "%COMMIT_HASH%"=="" set "COMMIT_HASH=unknown"

rem Get build date and time
for /f "tokens=*" %%i in ('powershell -command "Get-Date -Format 'MMM dd yyyy'"') do set "BUILD_DATE=%%i"
for /f "tokens=*" %%i in ('powershell -command "Get-Date -Format 'HH:mm:ss'"') do set "BUILD_TIME=%%i"

:parse_args
if "%~1"=="" goto args_done
if "%~1"=="-v" goto set_version
if "%~1"=="--version" goto set_version
if "%~1"=="-b" goto set_build_number
if "%~1"=="--build-number" goto set_build_number
if "%~1"=="-o" goto set_output
if "%~1"=="--output" goto set_output
if "%~1"=="-r" goto set_release
if "%~1"=="--release" goto set_release
if "%~1"=="-c" goto set_clean
if "%~1"=="--clean" goto set_clean
if "%~1"=="-h" goto show_help
if "%~1"=="--help" goto show_help

echo Unknown option: %~1
goto show_help

:set_version
set "VERSION=%~2"
shift
shift
goto parse_args

:set_build_number
set "BUILD_NUMBER=%~2"
shift
shift
goto parse_args

:set_output
set "OUTPUT_DIR=%~2"
shift
shift
goto parse_args

:set_release
set "IS_RELEASE=true"
shift
goto parse_args

:set_clean
set "CLEAN_BUILD=true"
shift
goto parse_args

:show_help
echo Usage: %~nx0 [OPTIONS]
echo.
echo Options:
echo   -v, --version VERSION    Set version string (e.g., 2.1.0)
echo   -b, --build-number NUM   Set build number (default: timestamp)
echo   -o, --output DIR         Output directory (default: build-output)
echo   -r, --release           Build release version (clean build)
echo   -c, --clean             Clean build directories first
echo   -h, --help              Show this help
echo.
echo Examples:
echo   %~nx0 -v 2.1.0 -r                    # Release build v2.1.0
echo   %~nx0 -v 2.1.0-beta -b 123           # Beta build with custom build number
echo   %~nx0 -c                              # Development build (clean)
exit /b 0

:args_done

echo ========================================
echo     WeighMyBru² Build Script
echo ========================================
echo.

rem Setup version
if "%VERSION%"=="" (
    rem Try to get version from git tag
    for /f "tokens=*" %%i in ('git describe --tags --exact-match HEAD 2^>nul') do (
        set "VERSION=%%i"
        set "VERSION=!VERSION:v=!"
        set "IS_RELEASE=true"
        echo [STEP] Detected release version from git tag: !VERSION!
    )
    
    if "!VERSION!"=="" (
        rem Development version
        for /f "tokens=*" %%i in ('powershell -command "Get-Date -Format 'yyyyMMdd'"') do set "VERSION=dev-%%i"
        echo [STEP] Using development version: !VERSION!
    )
)

set "FULL_VERSION=%VERSION%+build.%BUILD_NUMBER%.%COMMIT_HASH%"

echo Version: %VERSION%
echo Build Number: %BUILD_NUMBER%
echo Commit Hash: %COMMIT_HASH%
echo Full Version: %FULL_VERSION%
echo Build Date: %BUILD_DATE% %BUILD_TIME%
echo.

rem Setup build flags
set "PLATFORMIO_BUILD_FLAGS=-DWEIGHMYBRU_BUILD_NUMBER=%BUILD_NUMBER% -DWEIGHMYBRU_COMMIT_HASH=\"%COMMIT_HASH%\" -DWEIGHMYBRU_BUILD_DATE=\"%BUILD_DATE%\" -DWEIGHMYBRU_BUILD_TIME=\"%BUILD_TIME%\""

echo [STEP] Build flags: %PLATFORMIO_BUILD_FLAGS%

rem Clean build if requested
if "%CLEAN_BUILD%"=="true" (
    echo [STEP] Cleaning build directories...
    python -m platformio run -t clean
    if exist .pio\build rmdir /s /q .pio\build
    echo [SUCCESS] Build directories cleaned
)

if "%IS_RELEASE%"=="true" (
    echo [STEP] Release build - cleaning directories...
    python -m platformio run -t clean
    if exist .pio\build rmdir /s /q .pio\build
)

rem Check if PlatformIO is available
python -m platformio --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] PlatformIO not found. Please install PlatformIO first.
    exit /b 1
)

rem Build ESP32-S3 Supermini
echo [STEP] Building esp32s3-supermini...
python -m platformio run -e esp32s3-supermini
if errorlevel 1 (
    echo [ERROR] Failed to build esp32s3-supermini
    exit /b 1
)

echo [STEP] Building filesystem for esp32s3-supermini...
python -m platformio run -e esp32s3-supermini -t buildfs
echo [SUCCESS] Build complete for esp32s3-supermini

rem Build XIAO ESP32S3
echo [STEP] Building esp32s3-xiao...
python -m platformio run -e esp32s3-xiao
if errorlevel 1 (
    echo [ERROR] Failed to build esp32s3-xiao
    exit /b 1
)

echo [STEP] Building filesystem for esp32s3-xiao...
python -m platformio run -e esp32s3-xiao -t buildfs
echo [SUCCESS] Build complete for esp32s3-xiao

rem Create output directory
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

rem Copy binaries for Supermini
echo [STEP] Copying binaries for supermini...
if exist ".pio\build\esp32s3-supermini\firmware.bin" (
    copy ".pio\build\esp32s3-supermini\firmware.bin" "%OUTPUT_DIR%\weighmybru-supermini-v%VERSION%.bin" >nul
    echo [SUCCESS] Copied firmware binary
)
if exist ".pio\build\esp32s3-supermini\littlefs.bin" (
    copy ".pio\build\esp32s3-supermini\littlefs.bin" "%OUTPUT_DIR%\weighmybru-supermini-v%VERSION%-littlefs.bin" >nul
    echo [SUCCESS] Copied filesystem binary
)
if exist ".pio\build\esp32s3-supermini\bootloader.bin" (
    copy ".pio\build\esp32s3-supermini\bootloader.bin" "%OUTPUT_DIR%\weighmybru-supermini-v%VERSION%-bootloader.bin" >nul
)
if exist ".pio\build\esp32s3-supermini\partitions.bin" (
    copy ".pio\build\esp32s3-supermini\partitions.bin" "%OUTPUT_DIR%\weighmybru-supermini-v%VERSION%-partitions.bin" >nul
)

rem Copy binaries for XIAO
echo [STEP] Copying binaries for xiao...
if exist ".pio\build\esp32s3-xiao\firmware.bin" (
    copy ".pio\build\esp32s3-xiao\firmware.bin" "%OUTPUT_DIR%\weighmybru-xiao-v%VERSION%.bin" >nul
    echo [SUCCESS] Copied firmware binary
)
if exist ".pio\build\esp32s3-xiao\littlefs.bin" (
    copy ".pio\build\esp32s3-xiao\littlefs.bin" "%OUTPUT_DIR%\weighmybru-xiao-v%VERSION%-littlefs.bin" >nul
    echo [SUCCESS] Copied filesystem binary
)
if exist ".pio\build\esp32s3-xiao\bootloader.bin" (
    copy ".pio\build\esp32s3-xiao\bootloader.bin" "%OUTPUT_DIR%\weighmybru-xiao-v%VERSION%-bootloader.bin" >nul
)
if exist ".pio\build\esp32s3-xiao\partitions.bin" (
    copy ".pio\build\esp32s3-xiao\partitions.bin" "%OUTPUT_DIR%\weighmybru-xiao-v%VERSION%-partitions.bin" >nul
)

rem Generate ESP32 Web Tools manifests
echo [STEP] Generating ESP32 Web Tools manifest for supermini...
echo {> "%OUTPUT_DIR%\manifest-supermini.json"
echo   "name": "WeighMyBru² - ESP32-S3 Supermini",>> "%OUTPUT_DIR%\manifest-supermini.json"
echo   "version": "%VERSION%",>> "%OUTPUT_DIR%\manifest-supermini.json"
echo   "home_assistant_domain": "weighmybru",>> "%OUTPUT_DIR%\manifest-supermini.json"
echo   "new_install_prompt_erase": true,>> "%OUTPUT_DIR%\manifest-supermini.json"
echo   "funding_url": "https://github.com/031devstudios/weighmybru2",>> "%OUTPUT_DIR%\manifest-supermini.json"
echo   "builds": [>> "%OUTPUT_DIR%\manifest-supermini.json"
echo     {>> "%OUTPUT_DIR%\manifest-supermini.json"
echo       "chipFamily": "ESP32-S3",>> "%OUTPUT_DIR%\manifest-supermini.json"
echo       "parts": [>> "%OUTPUT_DIR%\manifest-supermini.json"
echo         {"path": "weighmybru-supermini-v%VERSION%-bootloader.bin", "offset": 0},>> "%OUTPUT_DIR%\manifest-supermini.json"
echo         {"path": "weighmybru-supermini-v%VERSION%-partitions.bin", "offset": 32768},>> "%OUTPUT_DIR%\manifest-supermini.json"
echo         {"path": "weighmybru-supermini-v%VERSION%.bin", "offset": 65536},>> "%OUTPUT_DIR%\manifest-supermini.json"
echo         {"path": "weighmybru-supermini-v%VERSION%-littlefs.bin", "offset": 2686976}>> "%OUTPUT_DIR%\manifest-supermini.json"
echo       ]>> "%OUTPUT_DIR%\manifest-supermini.json"
echo     }>> "%OUTPUT_DIR%\manifest-supermini.json"
echo   ]>> "%OUTPUT_DIR%\manifest-supermini.json"
echo }>> "%OUTPUT_DIR%\manifest-supermini.json"

echo [STEP] Generating ESP32 Web Tools manifest for xiao...
echo {> "%OUTPUT_DIR%\manifest-xiao.json"
echo   "name": "WeighMyBru² - XIAO ESP32S3",>> "%OUTPUT_DIR%\manifest-xiao.json"
echo   "version": "%VERSION%",>> "%OUTPUT_DIR%\manifest-xiao.json"
echo   "home_assistant_domain": "weighmybru",>> "%OUTPUT_DIR%\manifest-xiao.json"
echo   "new_install_prompt_erase": true,>> "%OUTPUT_DIR%\manifest-xiao.json"
echo   "funding_url": "https://github.com/031devstudios/weighmybru2",>> "%OUTPUT_DIR%\manifest-xiao.json"
echo   "builds": [>> "%OUTPUT_DIR%\manifest-xiao.json"
echo     {>> "%OUTPUT_DIR%\manifest-xiao.json"
echo       "chipFamily": "ESP32-S3",>> "%OUTPUT_DIR%\manifest-xiao.json"
echo       "parts": [>> "%OUTPUT_DIR%\manifest-xiao.json"
echo         {"path": "weighmybru-xiao-v%VERSION%-bootloader.bin", "offset": 0},>> "%OUTPUT_DIR%\manifest-xiao.json"
echo         {"path": "weighmybru-xiao-v%VERSION%-partitions.bin", "offset": 32768},>> "%OUTPUT_DIR%\manifest-xiao.json"
echo         {"path": "weighmybru-xiao-v%VERSION%.bin", "offset": 65536},>> "%OUTPUT_DIR%\manifest-xiao.json"
echo         {"path": "weighmybru-xiao-v%VERSION%-littlefs.bin", "offset": 2686976}>> "%OUTPUT_DIR%\manifest-xiao.json"
echo       ]>> "%OUTPUT_DIR%\manifest-xiao.json"
echo     }>> "%OUTPUT_DIR%\manifest-xiao.json"
echo   ]>> "%OUTPUT_DIR%\manifest-xiao.json"
echo }>> "%OUTPUT_DIR%\manifest-xiao.json"

rem Generate build info
echo [STEP] Generating build information...
echo {> "%OUTPUT_DIR%\build-info.json"
echo   "version": "%VERSION%",>> "%OUTPUT_DIR%\build-info.json"
echo   "full_version": "%FULL_VERSION%",>> "%OUTPUT_DIR%\build-info.json"
echo   "build_number": %BUILD_NUMBER%,>> "%OUTPUT_DIR%\build-info.json"
echo   "commit_hash": "%COMMIT_HASH%",>> "%OUTPUT_DIR%\build-info.json"
echo   "build_date": "%BUILD_DATE%",>> "%OUTPUT_DIR%\build-info.json"
echo   "build_time": "%BUILD_TIME%",>> "%OUTPUT_DIR%\build-info.json"
echo   "is_release": %IS_RELEASE%,>> "%OUTPUT_DIR%\build-info.json"
echo   "environments": ["esp32s3-supermini", "esp32s3-xiao"]>> "%OUTPUT_DIR%\build-info.json"
echo }>> "%OUTPUT_DIR%\build-info.json"

echo.
echo ========================================
echo          Build Summary
echo ========================================
echo Version: %VERSION%
echo Output Directory: %OUTPUT_DIR%
echo.
echo Generated Files:
dir "%OUTPUT_DIR%"
echo.
echo [SUCCESS] Build completed successfully!
echo.
echo To flash firmware:
echo   python -m platformio run -e esp32s3-supermini -t upload
echo   python -m platformio run -e esp32s3-xiao -t upload
echo.
echo To upload filesystem:
echo   python -m platformio run -e esp32s3-supermini -t uploadfs
echo   python -m platformio run -e esp32s3-xiao -t uploadfs

endlocal
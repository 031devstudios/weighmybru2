@echo off
REM WeighMyBru² Astro Deployment Script (Simple Version)
REM This script copies ESP32 Web Tools components to your Astro website

setlocal EnableDelayedExpansion

set "DEST_PATH=D:\Weigh My Bru\Concept\weighmybru-site"
set "SOURCE_PATH=%~dp0"

echo ===============================================
echo    WeighMyBru² Astro Deployment Script
echo ===============================================
echo.

echo Source: %SOURCE_PATH%
echo Destination: %DEST_PATH%
echo.

REM Check if destination exists
if not exist "%DEST_PATH%" (
    echo [ERROR] Destination directory does not exist: %DEST_PATH%
    echo Please create your Astro website directory first or edit this script
    echo to point to the correct location.
    pause
    exit /b 1
)

REM Check if it's an Astro project
if not exist "%DEST_PATH%\package.json" (
    if not exist "%DEST_PATH%\src" (
        echo [WARNING] Destination doesn't appear to be an Astro project
        echo Expected to find package.json or src\ directory
        echo.
        set /p confirm="Continue anyway? (y/N): "
        if /i not "!confirm!"=="y" (
            echo Deployment cancelled
            exit /b 0
        )
    )
)

echo [INFO] Creating directory structure...

REM Create necessary directories
mkdir "%DEST_PATH%\src\layouts" 2>nul
mkdir "%DEST_PATH%\src\components" 2>nul
mkdir "%DEST_PATH%\src\pages" 2>nul
mkdir "%DEST_PATH%\public\releases" 2>nul
mkdir "%DEST_PATH%\.github\workflows" 2>nul

echo [INFO] Copying component files...

REM Copy Astro components
copy "%SOURCE_PATH%BaseLayout.astro" "%DEST_PATH%\src\layouts\BaseLayout.astro" >nul
if errorlevel 1 echo [ERROR] Failed to copy BaseLayout.astro

copy "%SOURCE_PATH%Navigation.astro" "%DEST_PATH%\src\components\Navigation.astro" >nul
if errorlevel 1 echo [ERROR] Failed to copy Navigation.astro

copy "%SOURCE_PATH%HeroSection.astro" "%DEST_PATH%\src\components\HeroSection.astro" >nul
if errorlevel 1 echo [ERROR] Failed to copy HeroSection.astro

copy "%SOURCE_PATH%FeaturesGrid.astro" "%DEST_PATH%\src\components\FeaturesGrid.astro" >nul
if errorlevel 1 echo [ERROR] Failed to copy FeaturesGrid.astro

copy "%SOURCE_PATH%FlashFirmware.astro" "%DEST_PATH%\src\components\FlashFirmware.astro" >nul
if errorlevel 1 echo [ERROR] Failed to copy FlashFirmware.astro

copy "%SOURCE_PATH%index.astro" "%DEST_PATH%\src\pages\example-homepage.astro" >nul
if errorlevel 1 echo [ERROR] Failed to copy example homepage

REM Copy documentation and workflow
copy "%SOURCE_PATH%ASTRO_INTEGRATION.md" "%DEST_PATH%\ASTRO_INTEGRATION.md" >nul
if errorlevel 1 echo [ERROR] Failed to copy integration guide

copy "%SOURCE_PATH%sync-release-workflow.yml" "%DEST_PATH%\.github\workflows\sync-weighmybru-release.yml" >nul
if errorlevel 1 echo [ERROR] Failed to copy GitHub workflow

echo [INFO] Creating additional files...

REM Create flash page
(
echo ---
echo // Flash firmware page - ESP32 Web Tools integration
echo import FlashFirmware from '../components/FlashFirmware.astro';
echo ---
echo.
echo ^<FlashFirmware /^>
) > "%DEST_PATH%\src\pages\flash.astro"

REM Create releases directory placeholder
echo # This directory will contain WeighMyBru² firmware releases > "%DEST_PATH%\public\releases\.gitkeep"
echo # It will be populated by the release sync workflow >> "%DEST_PATH%\public\releases\.gitkeep"

echo.
echo [SUCCESS] ✅ Deployment Complete!
echo.
echo Next Steps:
echo 1. Navigate to: cd "%DEST_PATH%"
echo 2. Install dependencies: npm install
echo 3. Add Tailwind CSS: npm install @astrojs/tailwind tailwindcss
echo 4. Test the flash page: npm run dev
echo 5. Visit: http://localhost:4321/flash
echo.
echo 📖 For detailed instructions, see: ASTRO_INTEGRATION.md
echo.
pause
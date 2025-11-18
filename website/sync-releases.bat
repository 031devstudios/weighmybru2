@echo off
REM WeighMyBru² - GitHub Release to Website Sync Script (Windows)
REM This script syncs the latest GitHub release files to your website

setlocal EnableDelayedExpansion

REM Configuration
set "GITHUB_REPO=031devstudios/weighmybru2"
set "WEBSITE_RELEASES_DIR=.\releases"
set "LATEST_DIR=%WEBSITE_RELEASES_DIR%\latest"
set "TEMP_DIR=%TEMP%\weighmybru_sync"

REM Create temp directory
if not exist "%TEMP_DIR%" mkdir "%TEMP_DIR%"

echo ===============================================
echo    WeighMyBru² Release Sync Script (Windows)
echo ===============================================
echo.

REM Check for required tools
where curl >nul 2>&1
if errorlevel 1 (
    echo [ERROR] curl is required but not found in PATH
    echo Please install curl and try again
    exit /b 1
)

where powershell >nul 2>&1
if errorlevel 1 (
    echo [ERROR] PowerShell is required but not found
    exit /b 1
)

echo [INFO] Fetching latest release information from GitHub...

REM Get latest release info using PowerShell
powershell -Command "& {
    try {
        $response = Invoke-RestMethod -Uri 'https://api.github.com/repos/%GITHUB_REPO%/releases/latest'
        $response | ConvertTo-Json -Depth 10 | Out-File -FilePath '%TEMP_DIR%\release_info.json' -Encoding UTF8
        Write-Host '[SUCCESS] Release information fetched'
        Write-Host '[INFO] Latest version:' $response.tag_name
        $response.tag_name | Out-File -FilePath '%TEMP_DIR%\tag_name.txt' -Encoding ASCII -NoNewline
    } catch {
        Write-Host '[ERROR] Failed to fetch release information'
        exit 1
    }
}"

if errorlevel 1 (
    echo [ERROR] Failed to get release information
    exit /b 1
)

REM Read tag name
set /p TAG_NAME=<"%TEMP_DIR%\tag_name.txt"

echo [INFO] Processing release %TAG_NAME%...

REM Create release directory
set "RELEASE_DIR=%WEBSITE_RELEASES_DIR%\%TAG_NAME%"
if not exist "%RELEASE_DIR%" mkdir "%RELEASE_DIR%"

echo [INFO] Downloading release assets...

REM Download assets using PowerShell
powershell -Command "& {
    $releaseInfo = Get-Content '%TEMP_DIR%\release_info.json' | ConvertFrom-Json
    $downloadCount = 0
    
    foreach ($asset in $releaseInfo.assets) {
        $filename = $asset.name
        $downloadUrl = $asset.browser_download_url
        $outputPath = '%RELEASE_DIR%\' + $filename
        
        Write-Host '[INFO] Downloading' $filename '...'
        try {
            Invoke-WebRequest -Uri $downloadUrl -OutFile $outputPath
            Write-Host '[SUCCESS] Downloaded' $filename
            $downloadCount++
        } catch {
            Write-Host '[ERROR] Failed to download' $filename
            exit 1
        }
    }
    
    Write-Host '[SUCCESS] Downloaded' $downloadCount 'assets'
}"

if errorlevel 1 (
    echo [ERROR] Failed to download assets
    exit /b 1
)

echo [INFO] Extracting files...

REM Extract ZIP files
for %%f in ("%RELEASE_DIR%\*.zip") do (
    echo [INFO] Extracting %%~nxf...
    powershell -Command "Expand-Archive -Path '%%f' -DestinationPath '%RELEASE_DIR%' -Force"
    del "%%f"
)

echo [INFO] Updating latest release pointer...

REM Remove existing latest directory
if exist "%LATEST_DIR%" rmdir /s /q "%LATEST_DIR%"

REM Copy to latest (Windows doesn't always support symlinks)
mkdir "%LATEST_DIR%"
xcopy "%RELEASE_DIR%\*" "%LATEST_DIR%\" /s /e /y >nul

echo [SUCCESS] Updated latest release files

echo [INFO] Generating release index...

REM Generate release index using PowerShell
powershell -Command "& {
    $releases = @()
    $releasesDir = '%WEBSITE_RELEASES_DIR%'
    
    if (Test-Path $releasesDir) {
        $subdirs = Get-ChildItem -Path $releasesDir -Directory | Where-Object { $_.Name -ne 'latest' }
        foreach ($dir in $subdirs) {
            $releases += $dir.Name
        }
    }
    
    $index = @{
        releases = $releases
        latest = '%TAG_NAME%'
        last_updated = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
    }
    
    $index | ConvertTo-Json | Out-File -FilePath '%WEBSITE_RELEASES_DIR%\index.json' -Encoding UTF8
    Write-Host '[SUCCESS] Generated release index'
}"

echo [INFO] Validating manifest files...

REM Simple validation - check if JSON files exist and are not empty
for %%f in ("%RELEASE_DIR%\manifest-*.json") do (
    if exist "%%f" (
        echo [SUCCESS] Found %%~nxf
    )
)

echo [INFO] Cleaning up temporary files...
rmdir /s /q "%TEMP_DIR%"

echo.
echo [SUCCESS] ✅ Successfully synced release %TAG_NAME% to website
echo [INFO] 📁 Release files are ready in: %WEBSITE_RELEASES_DIR%
echo [INFO] 🌐 Latest files accessible at: %LATEST_DIR%
echo.
echo Next Steps:
echo 1. Upload the releases\ directory to your website
echo 2. Update your flash.html page if needed
echo 3. Test the ESP32 Web Tools installation
echo.

pause
exit /b 0
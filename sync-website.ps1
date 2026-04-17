# WeighMyBru² Website Sync Script
# This script syncs the correct v2.2.0 manifest files to the actual website directory

param(
    [string]$WebsitePath = "D:\Weigh My Bru\Concept\weighmybru-site"
)

Write-Host "🔄 Syncing WeighMyBru² website files..." -ForegroundColor Cyan
Write-Host ""

$SourcePath = "D:\Weigh My Bru\Concept\WeighMyBru²"
$WebsiteReleasesPath = Join-Path $WebsitePath "releases\latest"

# Check if website directory exists
if (-not (Test-Path $WebsitePath)) {
    Write-Error "Website directory not found: $WebsitePath"
    exit 1
}

# Create releases directory structure in website if it doesn't exist
if (-not (Test-Path $WebsiteReleasesPath)) {
    Write-Host "📁 Creating releases directory structure..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $WebsiteReleasesPath -Force | Out-Null
}

# Copy manifest files from build output to website
Write-Host "📋 Copying v2.2.0 manifest files..." -ForegroundColor Yellow

$SourceManifests = Join-Path $SourcePath "build-output\latest"
if (Test-Path $SourceManifests) {
    Copy-Item -Path "$SourceManifests\manifest-*.json" -Destination $WebsiteReleasesPath -Force
    Copy-Item -Path "$SourceManifests\*.bin" -Destination $WebsiteReleasesPath -Force
    Write-Host "✅ Copied manifest files and binaries" -ForegroundColor Green
} else {
    Write-Warning "Source manifests not found at: $SourceManifests"
}

# Create versioned manifest files with unique names
Write-Host "🆕 Creating versioned manifest files..." -ForegroundColor Yellow

$SuperminiSource = Join-Path $WebsiteReleasesPath "manifest-supermini.json"
$XiaoSource = Join-Path $WebsiteReleasesPath "manifest-xiao.json"

if (Test-Path $SuperminiSource) {
    Copy-Item $SuperminiSource -Destination (Join-Path $WebsiteReleasesPath "manifest-supermini-v220.json") -Force
    Write-Host "✅ Created manifest-supermini-v220.json" -ForegroundColor Green
}

if (Test-Path $XiaoSource) {
    Copy-Item $XiaoSource -Destination (Join-Path $WebsiteReleasesPath "manifest-xiao-v220.json") -Force
    Write-Host "✅ Created manifest-xiao-v220.json" -ForegroundColor Green
}

# Update flash.html if it exists
$FlashHtml = Join-Path $WebsitePath "flash.html"
if (-not (Test-Path $FlashHtml)) {
    $FlashHtml = Join-Path $WebsitePath "flash\index.html"
}

if (Test-Path $FlashHtml) {
    Write-Host "🔧 Updating flash.html..." -ForegroundColor Yellow
    
    $content = Get-Content $FlashHtml -Raw
    
    # Update manifest paths to use versioned files
    $content = $content -replace 'manifest="[^"]*manifest-supermini[^"]*"', 'manifest="./releases/latest/manifest-supermini-v220.json"'
    $content = $content -replace 'manifest="[^"]*manifest-xiao[^"]*"', 'manifest="./releases/latest/manifest-xiao-v220.json"'
    
    Set-Content -Path $FlashHtml -Value $content -Encoding UTF8
    Write-Host "✅ Updated flash.html with new manifest paths" -ForegroundColor Green
} else {
    Write-Warning "flash.html not found. Please update manually."
}

Write-Host ""
Write-Host "🎯 Next steps:" -ForegroundColor Cyan
Write-Host "1. Navigate to your website directory: cd '$WebsitePath'"
Write-Host "2. Check git status: git status"
Write-Host "3. Add files: git add ."
Write-Host "4. Commit: git commit -m 'Update to v2.2.0 firmware manifests'"
Write-Host "5. Push: git push"
Write-Host ""
Write-Host "🚀 After pushing, ESP32 Web Tools should show v2.2.0!" -ForegroundColor Green
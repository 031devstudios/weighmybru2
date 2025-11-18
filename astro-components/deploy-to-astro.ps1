# WeighMyBru² Astro Website Deployment Script
# This script copies the ESP32 Web Tools components to your Astro website

param(
    [Parameter(Mandatory=$false)]
    [string]$DestinationPath = "D:\Weigh My Bru\Concept\weighmybru-site",
    
    [switch]$DryRun = $false,
    [switch]$Force = $false
)

$ErrorActionPreference = "Stop"

# Colors for output
function Write-ColorOutput($ForegroundColor) {
    $fc = $host.UI.RawUI.ForegroundColor
    $host.UI.RawUI.ForegroundColor = $ForegroundColor
    if ($args) {
        Write-Output $args
    } else {
        $input | Write-Output
    }
    $host.UI.RawUI.ForegroundColor = $fc
}

function Write-Success { Write-ColorOutput Green $args }
function Write-Info { Write-ColorOutput Cyan $args }
function Write-Warning { Write-ColorOutput Yellow $args }
function Write-Error { Write-ColorOutput Red $args }

Write-Info "==============================================="
Write-Info "   WeighMyBru² Astro Deployment Script"
Write-Info "==============================================="
Write-Info ""

# Source and destination paths
$SourceDir = $PSScriptRoot
$DestDir = $DestinationPath

Write-Info "Source: $SourceDir"
Write-Info "Destination: $DestDir"

if ($DryRun) {
    Write-Warning "DRY RUN MODE - No files will be copied"
}

# Check if destination exists
if (!(Test-Path $DestDir)) {
    Write-Error "Destination directory does not exist: $DestDir"
    Write-Info "Please create your Astro website directory first or provide the correct path"
    exit 1
}

# Check if it looks like an Astro project
$AstroIndicators = @("package.json", "astro.config.mjs", "astro.config.js", "src")
$IsAstroProject = $false

foreach ($indicator in $AstroIndicators) {
    if (Test-Path (Join-Path $DestDir $indicator)) {
        $IsAstroProject = $true
        break
    }
}

if (!$IsAstroProject -and !$Force) {
    Write-Warning "The destination doesn't appear to be an Astro project."
    Write-Warning "Expected to find: package.json, astro.config.mjs, or src/ directory"
    Write-Info "Use -Force to override this check"
    exit 1
}

# File mapping for Astro project structure
$FileMappings = @{
    # Astro Components
    "BaseLayout.astro" = "src/layouts/BaseLayout.astro"
    "Navigation.astro" = "src/components/Navigation.astro" 
    "HeroSection.astro" = "src/components/HeroSection.astro"
    "FeaturesGrid.astro" = "src/components/FeaturesGrid.astro"
    "FlashFirmware.astro" = "src/components/FlashFirmware.astro"
    
    # Pages
    "index.astro" = "src/pages/example-homepage.astro"  # Don't overwrite existing index
    
    # Documentation and workflows
    "ASTRO_INTEGRATION.md" = "ASTRO_INTEGRATION.md"
    "sync-release-workflow.yml" = ".github/workflows/sync-weighmybru-release.yml"
}

# Additional files to create
$AdditionalFiles = @{
    "src/pages/flash.astro" = @'
---
// Flash firmware page - ESP32 Web Tools integration
import FlashFirmware from '../components/FlashFirmware.astro';
---

<FlashFirmware />
'@

    "public/releases/.gitkeep" = @'
# This directory will contain WeighMyBru² firmware releases
# It will be populated by the release sync workflow
'@

    "src/content/config.ts" = @'
// Content collections configuration
import { defineCollection, z } from 'astro:content';

const docs = defineCollection({
  type: 'content',
  schema: z.object({
    title: z.string(),
    description: z.string().optional(),
    date: z.date().optional(),
    tags: z.array(z.string()).optional(),
  }),
});

export const collections = { docs };
'@
}

Write-Info ""
Write-Success "📂 File Deployment Plan:"
Write-Info ""

# Show what will be copied
foreach ($mapping in $FileMappings.GetEnumerator()) {
    $sourcePath = Join-Path $SourceDir $mapping.Key
    $destPath = Join-Path $DestDir $mapping.Value
    $destExists = Test-Path $destPath
    
    $status = if ($destExists) { "[EXISTS]" } else { "[NEW]" }
    Write-Info "  $status $($mapping.Key) → $($mapping.Value)"
}

Write-Info ""
Write-Success "📄 Additional Files to Create:"
foreach ($file in $AdditionalFiles.GetEnumerator()) {
    $destPath = Join-Path $DestDir $file.Key
    $destExists = Test-Path $destPath
    $status = if ($destExists) { "[EXISTS]" } else { "[NEW]" }
    Write-Info "  $status $($file.Key)"
}

if ($DryRun) {
    Write-Info ""
    Write-Warning "DRY RUN COMPLETE - No files were copied"
    Write-Info "Run without -DryRun to perform the actual deployment"
    exit 0
}

Write-Info ""
$confirmation = Read-Host "Proceed with deployment? (y/N)"
if ($confirmation -ne 'y' -and $confirmation -ne 'Y') {
    Write-Warning "Deployment cancelled"
    exit 0
}

Write-Info ""
Write-Success "🚀 Starting deployment..."

# Copy main component files
foreach ($mapping in $FileMappings.GetEnumerator()) {
    $sourcePath = Join-Path $SourceDir $mapping.Key
    $destPath = Join-Path $DestDir $mapping.Value
    $destDir = Split-Path $destPath -Parent
    
    # Create directory if it doesn't exist
    if (!(Test-Path $destDir)) {
        Write-Info "  Creating directory: $destDir"
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
    }
    
    # Check if source file exists
    if (Test-Path $sourcePath) {
        Write-Info "  Copying: $($mapping.Key) → $($mapping.Value)"
        Copy-Item $sourcePath $destPath -Force
        Write-Success "    ✅ Copied successfully"
    } else {
        Write-Warning "    ⚠️ Source file not found: $sourcePath"
    }
}

# Create additional files
Write-Info ""
Write-Success "📄 Creating additional files..."
foreach ($file in $AdditionalFiles.GetEnumerator()) {
    $destPath = Join-Path $DestDir $file.Key
    $destDir = Split-Path $destPath -Parent
    
    # Create directory if it doesn't exist
    if (!(Test-Path $destDir)) {
        Write-Info "  Creating directory: $destDir"
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
    }
    
    Write-Info "  Creating: $($file.Key)"
    $file.Value | Out-File -FilePath $destPath -Encoding UTF8
    Write-Success "    ✅ Created successfully"
}

# Check for package.json and suggest dependencies
$PackageJsonPath = Join-Path $DestDir "package.json"
if (Test-Path $PackageJsonPath) {
    Write-Info ""
    Write-Success "📦 Checking package.json dependencies..."
    
    $packageContent = Get-Content $PackageJsonPath -Raw | ConvertFrom-Json
    $hasAstro = $packageContent.dependencies.astro -or $packageContent.devDependencies.astro
    
    if ($hasAstro) {
        Write-Success "  ✅ Astro found in dependencies"
    } else {
        Write-Warning "  ⚠️ Astro not found in dependencies"
    }
    
    Write-Info ""
    Write-Info "🔧 Recommended dependencies for ESP32 Web Tools integration:"
    Write-Info "  npm install @astrojs/tailwind"
    Write-Info "  npm install tailwindcss"
    Write-Info "  npm install @fontsource/inter"
}

Write-Info ""
Write-Success "🎉 Deployment Complete!"
Write-Info ""
Write-Info "Next Steps:"
Write-Info "1. Navigate to your Astro website: cd '$DestDir'"
Write-Info "2. Install recommended dependencies (see above)"
Write-Info "3. Update your astro.config.mjs to include Tailwind CSS"
Write-Info "4. Test the flash page: npm run dev → visit /flash"
Write-Info "5. Set up release syncing workflow"
Write-Info "6. Deploy to your Cloudflare domain"
Write-Info ""
Write-Info "📖 For detailed integration instructions, see: ASTRO_INTEGRATION.md"
Write-Info ""
Write-Success "Happy flashing! 🚀"
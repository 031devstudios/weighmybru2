# 🚀 WeighMyBru² Automated Build & Release Workflow

This document outlines the improved build and release process that automates website deployment.

## 🎯 Quick Start

### Linux / macOS
```bash
./build-and-release.sh
```

### Windows (PowerShell - Recommended)
```powershell
.\build-and-release.ps1
```

This single command will:
1. ✅ Build firmware for both board variants
2. ✅ Extract version from `Version.h`
3. ✅ Copy files to website releases directory
4. ✅ Update release index
5. ✅ Auto-deploy to Cloudflare (manual git push)

## 📋 Available Scripts

### 1. Enhanced Build & Release

**Linux / macOS:**
```bash
./build-and-release.sh [options]
```

**Windows (PowerShell - Recommended):**
```powershell
.\build-and-release.ps1 [options]
```

**Windows (Batch):**
```cmd
build-and-release.bat [options]
```

**Options:**
| Linux/macOS | Windows | Description |
|-------------|---------|-------------|
| `--clean` | `-Clean` | Clean build directories first |
| `--release` | `-Release` | Build release version |
| `--help` | `-Help` | Show help |

**Examples:**
```bash
# Standard release build (auto-detects version from Version.h)
./build-and-release.sh --release

# Development build with clean
./build-and-release.sh --clean

# Windows PowerShell
.\build-and-release.ps1 -Release
.\build-and-release.ps1 -Clean
```

### 2. Original Build Script (Firmware Only)

**Linux / macOS:**
```bash
./build.sh [options]
```

**Windows:**
```cmd
build.bat [options]
```

### 3. Website Release Sync (Linux/macOS only)

Downloads latest GitHub release to website:
```bash
./website/sync-releases.sh
```

## 🔄 Automated Workflow

When you run `build-and-release.sh` or `build-and-release.ps1`, here's what happens:

```mermaid
graph TD
    A[Start Build] --> B[Extract Version from Version.h]
    B --> C[Run Original Build Script]
    C --> D[Create Website Directory Structure]
    D --> E[Copy Manifest Files]
    E --> F[Copy Firmware Binaries]  
    F --> G[Update releases/index.json]
    G --> H[Manual: git add, commit, push]
    H --> I[✅ Live on Cloudflare!]
```

## 📁 Generated File Structure

After running the enhanced build script:

```
build-output/
├── latest/
│   ├── manifest-supermini.json     # ESP32 Web Tools manifests
│   ├── manifest-xiao.json
│   ├── firmware-supermini.bin      # Firmware binaries
│   ├── firmware-xiao.bin
│   ├── bootloader.bin              # ESP32 system files
│   ├── partitions.bin
│   └── spiffs.bin

website/
└── releases/
    ├── index.json                  # Release index (for APIs)
    ├── latest/                     # Always points to newest
    │   ├── manifest-supermini.json
    │   ├── manifest-xiao.json
    │   └── *.bin
    └── v2.2.0/                     # Version-specific directory
        ├── manifest-supermini.json
        ├── manifest-xiao.json  
        └── *.bin
```

## 🎛️ Version Management

### Automatic Version Detection
The script automatically reads version from [include/Version.h](include/Version.h):

```cpp
#define WEIGHMYBRU_VERSION_MAJOR 2
#define WEIGHMYBRU_VERSION_MINOR 2  
#define WEIGHMYBRU_VERSION_PATCH 0
```

Results in version: `2.2.0`

### Manual Version Override
```powershell
.\build-and-release.ps1 -Version "2.3.0-beta"
```

## 🚀 Deployment to Cloudflare

### Manual Deployment
After running the build script, manually commit and push:

**Linux/macOS:**
```bash
git add website/releases/
git commit -m "Release v2.2.0 - Updated firmware manifests"
git push
```

**Windows:**
```cmd
git add website/releases/
git commit -m "Release v2.2.0 - Updated firmware manifests"
git push
```

Your Cloudflare site will auto-update within 2-3 minutes.

## 🔧 Updating to New Version

### For Minor Updates (bug fixes, small features):
1. Update version in [include/Version.h](include/Version.h)
2. Run: `./build-and-release.sh --release` (Linux) or `.\build-and-release.ps1 -Release` (Windows)
3. Commit and push manually
4. ✅ Done! Live in ~3 minutes

### For Major Updates:
1. Update version in [include/Version.h](include/Version.h)
2. Test locally first: `./build-and-release.sh --clean` (Linux) or `.\build-and-release.ps1 -Clean` (Windows)
3. When ready: `./build-and-release.sh --release` (Linux) or `.\build-and-release.ps1 -Release` (Windows)
4. Commit and push manually

## 🎯 Best Practices

### ✅ Do This:
- Always use `build-and-release.sh` (Linux) or `build-and-release.ps1` (Windows) for releases
- Update `Version.h` before building
- Test locally before pushing
- Use descriptive commit messages

### ❌ Avoid This:  
- Don't manually copy files between directories
- Don't forget to update version in `Version.h`
- Don't mix development and release builds
- Don't skip testing new versions

## 🛠️ Troubleshooting

### "Build failed with exit code 1"
- Check PlatformIO is installed: `pio --version` (Linux) or `python -m platformio --version` (Windows)
- Try clean build: `./build-and-release.sh --clean` (Linux) or `.\build-and-release.ps1 -Clean` (Windows)

### "Script permission denied" (Linux/macOS)
```bash
chmod +x build.sh build-and-release.sh
```

### "Git add failed"
- Check git is working: `git status`
- Verify you're in the right directory
- Check file permissions

### "Website still shows old version"
- Wait 3-5 minutes for Cloudflare deployment
- Check GitHub repo has the new files
- Clear browser cache

### "PowerShell execution policy error" (Windows)
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

## 📚 Additional Resources

- [ESP32 Web Tools Documentation](https://esphome.github.io/esp-web-tools/)
- [Cloudflare Pages Deployment](https://developers.cloudflare.com/pages/)
- [PlatformIO Build System](https://docs.platformio.org/en/latest/)

---

**Happy Building!** 🎉
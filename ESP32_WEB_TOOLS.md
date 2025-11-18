# ESP32 Web Tools Integration

This directory contains the automation and tooling for ESP32 Web Tools integration, making WeighMyBru² easily installable from web browsers.

## 🚀 Features

- **Automated Release Builds**: GitHub Actions automatically builds firmware for both board variants
- **ESP32 Web Tools Manifests**: Auto-generated manifests for web-based installation
- **Version Management**: Hybrid manual/automatic versioning system
- **Cross-Platform Build Scripts**: Support for Windows and Linux/macOS

## 📦 Release Process

### 1. Creating a Release

To create a new release:

1. Update the version in `include/Version.h`:
   ```cpp
   #define WEIGHMYBRU_VERSION_MAJOR 2
   #define WEIGHMYBRU_VERSION_MINOR 2  // Update this
   #define WEIGHMYBRU_VERSION_PATCH 0
   ```

2. Commit the changes:
   ```bash
   git add include/Version.h
   git commit -m "🔖 Bump version to 2.2.0"
   ```

3. Create and push the tag:
   ```bash
   git tag v2.2.0
   git push origin v2.2.0
   ```

4. GitHub Actions will automatically:
   - Build firmware for both boards
   - Generate ESP32 Web Tools manifests
   - Create a GitHub release with all files

### 2. Development Builds

For testing and development:

```bash
# Local build (both boards)
./build.sh -v 2.2.0-dev -c

# Windows
build.bat -v 2.2.0-dev -c
```

## 📋 Generated Files

Each release generates the following files:

### ESP32-S3 Supermini
- `weighmybru-supermini-vX.Y.Z.bin` - Main firmware
- `weighmybru-supermini-vX.Y.Z-littlefs.bin` - Web interface files
- `weighmybru-supermini-vX.Y.Z-bootloader.bin` - Bootloader
- `weighmybru-supermini-vX.Y.Z-partitions.bin` - Partition table
- `manifest-supermini.json` - ESP32 Web Tools manifest

### XIAO ESP32S3
- `weighmybru-xiao-vX.Y.Z.bin` - Main firmware
- `weighmybru-xiao-vX.Y.Z-littlefs.bin` - Web interface files  
- `weighmybru-xiao-vX.Y.Z-bootloader.bin` - Bootloader
- `weighmybru-xiao-vX.Y.Z-partitions.bin` - Partition table
- `manifest-xiao.json` - ESP32 Web Tools manifest

### Build Information
- `build-info.json` - Complete build metadata

## 🌐 Website Integration

To integrate with your website:

```html
<script type="module" src="https://unpkg.com/esp-web-tools@9/dist/web/install-button.js?module"></script>

<!-- ESP32-S3 Supermini -->
<esp-web-install-button 
  manifest="./releases/latest/manifest-supermini.json">
  <button slot="activate">Install WeighMyBru² (ESP32-S3 Supermini)</button>
</esp-web-install-button>

<!-- XIAO ESP32S3 -->
<esp-web-install-button 
  manifest="./releases/latest/manifest-xiao.json">
  <button slot="activate">Install WeighMyBru² (XIAO ESP32S3)</button>
</esp-web-install-button>
```

## 🔧 Build Scripts

### Local Development

```bash
# Build with custom version
./build.sh -v 2.1.0-beta

# Clean release build  
./build.sh -v 2.1.0 -r

# Development build with cleaning
./build.sh -c
```

### Manual Flashing

If you prefer manual flashing:

```bash
# Flash firmware only
esptool.py write_flash 0x10000 weighmybru-supermini-v2.1.0.bin

# Complete flash (bootloader + firmware + filesystem)
esptool.py write_flash \
  0x0 weighmybru-supermini-v2.1.0-bootloader.bin \
  0x8000 weighmybru-supermini-v2.1.0-partitions.bin \
  0x10000 weighmybru-supermini-v2.1.0.bin \
  0x290000 weighmybru-supermini-v2.1.0-littlefs.bin
```

## 📊 Version Information

The firmware includes comprehensive version information accessible via:

### Serial Output (Boot)
```
=================================
WeighMyBru² v2.1.0
Board: ESP32-S3 Supermini
Build: Nov 18 2025 10:30:25  
Full Version: 2.1.0+build.123.abc1234
Flash Size: 4MB
=================================
```

### Web API
```bash
# Basic version in dashboard
curl http://192.168.1.100/api/dashboard

# Detailed device information
curl http://192.168.1.100/api/device/info
```

## 🛠️ GitHub Actions Workflows

### build-release.yml
- Triggers on version tags (`v*`)
- Builds both board variants
- Generates ESP32 Web Tools manifests
- Creates GitHub releases automatically

### build-dev.yml  
- Runs on pull requests and pushes
- Tests builds for both boards
- Validates firmware size limits
- Provides build feedback

## 📖 ESP32 Web Tools Documentation

For more information about ESP32 Web Tools:
- [ESP32 Web Tools GitHub](https://github.com/esphome/esp-web-tools)
- [Manifest Format](https://github.com/esphome/esp-web-tools#manifest-format)
- [Integration Guide](https://esphome.github.io/esp-web-tools/)

## 🔍 Troubleshooting

### Build Issues
```bash
# Clean everything and rebuild
./build.sh -c -v 2.1.0

# Check PlatformIO installation
pio --version
```

### Version Issues
- Ensure version is updated in `include/Version.h` before tagging
- Use semantic versioning (MAJOR.MINOR.PATCH)
- Check that git tags match version numbers

### Manifest Issues
- Verify all binary files exist before generating manifests
- Check file offsets match your partition table
- Ensure proper JSON formatting
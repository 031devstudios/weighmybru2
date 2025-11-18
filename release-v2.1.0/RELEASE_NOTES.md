# WeighMyBru² v2.1.0 Release Notes

## 🚀 Major Feature: ESP32 Web Tools Integration

This release introduces browser-based firmware flashing, making it incredibly easy for users to install and update WeighMyBru² firmware without any software installation.

### ✨ New Features

**ESP32 Web Tools Support**
- Flash firmware directly from your browser at https://weighmybru.com/flash
- Complete 4-part flash: bootloader, partitions, firmware, and web interface
- No software installation required (Chrome, Edge, Opera)
- Support for ESP32-S3 Supermini and XIAO ESP32S3 boards
- One-click installation with automatic filesystem deployment
- Full web interface included: dashboard, settings, calibration, updates

**Professional Web Interface**
- Responsive design matching WeighMyBru² branding
- Board detection and automatic configuration
- Built-in troubleshooting guide
- Version tracking and update notifications

**Enhanced Build System**
- Improved PlatformIO configuration
- Automated release generation
- Board-specific optimizations
- Enhanced version management

### 📦 Release Files

**Firmware Binaries**
- `weighmybru2-v2.1.0-esp32s3-supermini.bin` - Firmware for ESP32-S3 Supermini boards
- `weighmybru2-v2.1.0-xiao-esp32s3.bin` - Firmware for XIAO ESP32S3 boards

**ESP32 Web Tools Complete Flash Package**
- `bootloader.bin` - ESP32 bootloader (shared)
- `partitions.bin` - Partition table with LittleFS support (shared)
- `littlefs-supermini.bin` - Web interface filesystem for ESP32-S3 Supermini (896KB)
- `littlefs-xiao.bin` - Web interface filesystem for XIAO ESP32S3 (896KB)
- `manifest-supermini.json` - ESP32 Web Tools manifest for ESP32-S3 Supermini
- `manifest-xiao.json` - ESP32 Web Tools manifest for XIAO ESP32S3

### 🔧 Installation Options

1. **ESP32 Web Tools (Recommended)**: Visit https://weighmybru.com/flash
   - Includes complete web interface (LittleFS filesystem)
   - Automatic 4-part flash process
   - No additional setup required
2. **PlatformIO**: Use the provided `.bin` files with esptool
3. **Manual Flash**: Use ESP32 Flash Download Tool with complete manifest

### ⚙️ Technical Details

- Firmware Size: ~1.1MB
- LittleFS Filesystem: 896KB (web interface)
- Flash Requirements: 8MB ESP32-S3 module
- Partition Layout: Custom with LittleFS at 0x310000
- Memory Usage: Optimized for both board variants
- Web Serial API compatibility for browser flashing

### 🌐 Browser Requirements

- ✅ Chrome/Chromium 89+
- ✅ Microsoft Edge 89+
- ✅ Opera 75+
- ❌ Firefox (Web Serial not supported)
- ❌ Safari (Web Serial not supported)

### 🔄 Upgrading

Users can upgrade from any previous version using either:
- The new web-based installer
- Traditional PlatformIO methods
- ESP32 download tools

### 📋 Changelog

- Added ESP32 Web Tools manifest files and integration
- Created professional Astro website components
- Enhanced version management and release automation
- Improved build system for multiple board targets
- Added comprehensive documentation and setup guides
- Optimized firmware size and performance

### ��️ Development

This release includes the complete development infrastructure for ESP32 Web Tools:
- Astro components for website integration  
- Automated build and release scripts
- Professional documentation and setup guides
- Cloudflare Pages deployment configuration

---

For technical support, visit our [GitHub repository](https://github.com/031devstudios/weighmybru2) or join our [Discord community](https://discord.gg/HYp4TSEjSf).

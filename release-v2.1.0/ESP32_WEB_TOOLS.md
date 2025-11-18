# ESP32 Web Tools Integration

## Overview
This release includes full ESP32 Web Tools support for browser-based firmware flashing. No need to install drivers, PlatformIO, or any other software - just use your web browser!

## Supported Boards
- **ESP32-S3 Supermini**: Use `manifest-supermini.json`
- **XIAO ESP32S3**: Use `manifest-xiao.json`

## Flash Components
Each manifest includes a complete 4-part flash process:

### 1. Bootloader (`bootloader.bin`)
- **Offset**: 0x0000
- **Size**: ~15KB
- ESP32-S3 bootloader

### 2. Partition Table (`partitions.bin`)
- **Offset**: 0x8000  
- **Size**: ~3KB
- Defines memory layout including LittleFS filesystem

### 3. Firmware (`weighmybru2-v2.1.0-*.bin`)
- **Offset**: 0x10000
- **Size**: ~1.1MB
- Main application code

### 4. LittleFS Filesystem (`littlefs-*.bin`)
- **Offset**: 0x310000 (3,211,264 bytes)
- **Size**: 896KB
- Contains complete web interface:
  - Dashboard (`/index.html`)
  - Settings (`/settings.html`) 
  - Calibration (`/calibration.html`)
  - Updates (`/updates.html`)
  - All CSS, JS, and assets

## Usage Instructions

### Option 1: Direct Flash (Recommended)
Visit the official flash page: **https://weighmybru.com/flash**

1. Select your board type
2. Connect ESP32 via USB
3. Click "Connect & Flash"
4. Wait for completion

### Option 2: Self-Host ESP32 Web Tools
If you want to host your own flash interface:

1. Download the appropriate manifest file
2. Host it on a web server with HTTPS
3. Use ESP32 Web Tools JavaScript library:

```html
<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"></script>

<esp-web-install-button manifest="./manifest-supermini.json">
  <button slot="activate">Flash WeighMyBru²</button>
</esp-web-install-button>
```

## Post-Flash Verification
After successful flashing:

1. **Reset the ESP32** (press reset button)
2. **Connect to WiFi** using the device's AP mode if needed
3. **Access web interface** at the device's IP address
4. **Verify all pages load**: Dashboard, Settings, Calibration, Updates

## Troubleshooting

### Web Interface Not Loading
- Ensure LittleFS was flashed correctly (check manifest includes filesystem)
- Verify partition table allows for filesystem at 0x310000
- Reset device after flashing

### Flash Errors
- Use Chrome or Edge browser (best Web Serial support)
- Ensure good USB cable (data, not charging cable)
- Try different USB port
- Put ESP32 in flash mode if needed (hold BOOT button while connecting)

### Browser Compatibility
- ✅ Chrome 89+
- ✅ Edge 89+  
- ✅ Opera 76+
- ❌ Firefox (no Web Serial API support)
- ❌ Safari (no Web Serial API support)

## File Checksums (SHA256)
```
manifest-supermini.json: [calculated on download]
manifest-xiao.json: [calculated on download]  
bootloader.bin: [calculated on download]
partitions.bin: [calculated on download]
weighmybru2-v2.1.0-esp32s3-supermini.bin: [calculated on download]
weighmybru2-v2.1.0-xiao-esp32s3.bin: [calculated on download]
littlefs-supermini.bin: [calculated on download] 
littlefs-xiao.bin: [calculated on download]
```

## Technical Details
- **ESP-IDF Version**: Latest
- **Arduino Framework**: ESP32 Arduino Core
- **Filesystem**: LittleFS (SPIFFS successor)
- **Web Server**: AsyncWebServer
- **Flash Size**: 8MB (boards support up to 8MB)
- **Partition Scheme**: Custom with LittleFS support

For issues or questions, visit: https://github.com/031devstudios/weighmybru2/issues
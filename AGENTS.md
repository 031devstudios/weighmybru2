# WeighMyBru2 - Agent Development Guide

## Project Overview

- **Type**: ESP32-S3 embedded firmware (C++/Arduino framework)
- **Build System**: PlatformIO
- **Hardware**: HX711 load cell, SSD1306 OLED, BLE, WiFi
- **Web UI**: Alpine.js + Chart.js on LittleFS
- **Repository**: github.com/031devstudios/weighmybru2

## IMPORTANT: ASK THE USER EVERY TIME FOR CONFIRMATION BEFORE COMMIT

**NEVER** run `git commit` without explicit user confirmation. Always present the staged changes and proposed commit message, wait for user approval, then commit only after they explicitly say to proceed.

## Build Commands

```bash
# Build firmware (select environment)
pio run -e esp32s3-supermini          # Build for Supermini
pio run -e esp32s3-xiao               # Build for XIAO

# Flash firmware to hardware
pio run -e esp32s3-supermini -t upload
pio run -e esp32s3-xiao -t upload

# Upload filesystem (required after firmware flash)
pio run -e esp32s3-supermini -t uploadfs
pio run -e esp32s3-xiao -t uploadfs

# Serial monitor
pio run -e esp32s3-supermini -t monitor

# Clean build
pio run -t clean

# Run unit tests (native)
pio test -e native
```

## Code Conventions

### Style
- K&R brace style, 4-space indent
- PascalCase for classes (e.g., `Scale`, `Display`, `WebServer`)
- camelCase for methods and variables (e.g., `getWeight()`, `currentWeight`)
- SCREAMING_SNAKE_CASE for constants (e.g., `MAX_SAMPLES`, `SCREEN_WIDTH`)

### File Structure
```
src/                    # C++ source files
  main.cpp             # Entry point, hardware initialization
  Scale.cpp            # HX711 load cell interface
  WebServer.cpp        # Async web server with REST API
  Display.cpp          # OLED display, timer management
  FlowRate.cpp         # Real-time flow rate calculation
  BluetoothScale.cpp   # BLE GATT server
  ...
include/               # Header files
  *.h                  # Class declarations
data/                   # LittleFS filesystem (web UI)
  index.html           # Main dashboard
  settings.html        # Configuration page
  calibration.html     # Calibration page
  js/                  # JavaScript (Alpine.js)
  css/                 # Stylesheets
test/                   # PlatformIO unit tests
```

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        main.cpp                             │
│  setup(): Hardware init sequence                            │
│  loop(): 100Hz task scheduler                               │
└─────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│    Scale      │   │   Display     │   │ BluetoothScale│
│  (HX711)     │   │   (OLED)     │   │   (BLE)      │
└───────────────┘   └───────────────┘   └───────────────┘
        │                     │
        ▼                     ▼
┌───────────────┐   ┌───────────────┐
│   FlowRate    │   │  WebServer   │
│  (calculations)│   │  (REST API)  │
└───────────────┘   └───────────────┘
```

### Timer Management

The `Display` class owns the timer system:
- `Display::startTimer()` - Start timer, begin flow averaging
- `Display::stopTimer()` - Stop timer, stop flow averaging
- `Display::resetTimer()` - Reset timer to zero AND sets timerState to IDLE
- `Display::isTimerRunning()` - Check if timer active
- `Display::getElapsedTime()` - Get elapsed milliseconds
- `Display::getTimerState()` - Returns TimerState enum (IDLE, RUNNING, STOPPED)

**Timer States**: IDLE (waiting), RUNNING (timing), STOPPED (paused after brew)

### Scale Brewing States

`Scale` class tracks brewing detection:
- `STABLE` - No weight change, using average filter
- `BREWING` - Active weight change detected, using median filter
- `TRANSITIONING` - Between brewing and stable

### REST API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/dashboard` | Main polling endpoint (all data) |
| POST | `/api/timer/start` | Start timer |
| POST | `/api/timer/stop` | Stop timer |
| POST | `/api/timer/reset` | Reset timer |
| POST | `/api/tare` | Tare scale; if STOPPED resets timer, if RUNNING resets auto-brew only |
| GET | `/api/weight-fast` | Fast weight-only response |
| GET | `/api/brew/weight` | GaggiMate-compatible weight |
| GET | `/api/brew/status` | GaggiMate-compatible status |

### Preferences (NVS)

Settings stored via `Preferences` class:
- `scale` namespace - calibration, filter settings
- `display` namespace - decimal places
- `wifi` namespace - WiFi credentials

## Testing Strategy

### Unit Tests
- Location: `test/` directory
- Run on native platform: `pio test -e native`
- Use Unity test framework (PlatformIO built-in)

### Hardware Testing
- Manual flash + test on physical hardware
- Test on BOTH board variants:
  - ESP32-S3 Supermini
  - XIAO ESP32S3

### CI/CD
- GitHub Actions (`.github/workflows/`)
- `build-dev.yml` - Build verification on push/PR
- No automated hardware tests (yet)

## Feature Development Workflow

### Branch Naming
```
main              # Stable releases
AutoBrewTimer    # Feature: Auto-brew timer checkbox
```

### Development Process
1. Create feature branch from `main`
2. Implement changes following conventions
3. Build verification: `pio run -e esp32s3-supermini`
4. Test on hardware
5. Submit PR with code review checklist

## Code Review Checklist

- [ ] Code compiles without warnings
- [ ] Follows K&R brace style, 4-space indent
- [ ] PascalCase classes, camelCase methods
- [ ] No heap allocations in loops
- [ ] HX711 connection checked before operations
- [ ] API endpoints follow REST conventions
- [ ] Preferences keys documented with defaults
- [ ] Tested on ESP32-S3 Supermini AND XIAO
- [ ] Web UI follows Alpine.js patterns
- [ ] AGENTS.md updated if needed
- [ ] No secrets/keys committed

## Important Notes

### PlatformIO Installation
```bash
# Linux/macOS
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-init/master/install.sh | bash

# Verify
pio --version
```

### Filesystem Upload
After flashing firmware, MUST upload filesystem:
```bash
pio run -e esp32s3-supermini -t uploadfs
```

### Build Scripts
```bash
./build.sh              # Build both board variants
./build-and-release.sh  # Build + prepare website releases
```

## Future Features (See AGENTS.md in docs/)

- AutoBrewTimer: Checkbox to auto-start/stop timer based on weight detection

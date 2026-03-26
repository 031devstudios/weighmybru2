# PlatformIO Unit Testing

This directory contains unit tests for the WeighMyBru2 firmware.

## Running Tests

### Native Testing (Recommended for CI/CD)
```bash
# Run all tests on native platform (Linux/macOS)
pio test -e native

# Run tests with verbose output
pio test -e native -v
```

### Hardware Testing (On actual ESP32-S3)
```bash
# Upload and run tests on XIAO ESP32S3 hardware
pio run -e esp32s3-xiao-test -t upload --upload-port /dev/ttyACM0

# To see test output, use serial monitor on Windows or
# (requires USB passthrough setup)
pio device monitor -e esp32s3-xiao-test --baud 115200
```

## Test Environments

| Environment | Purpose | Hardware Required |
|-------------|---------|------------------|
| `native` | Fast iteration, CI/CD | No |
| `esp32s3-xiao-test` | Full framework test | Yes (XIAO ESP32S3) |

## Test Structure

- `test_*.cpp` - Test files following PlatformIO naming convention
- Uses conditional compilation:
  - `#ifdef ARDUINO` - Unity framework with `setup()`/`loop()` (hardware)
  - Native C++ with `main()` (host)

## Adding New Tests

1. Create a new file: `test/test_yourfeature.cpp`
2. Use `#ifdef ARDUINO` / `#else` for platform-specific code
3. Run `pio test -e native` to execute

## Example Test Structure

```cpp
#ifdef ARDUINO
#include <Arduino.h>
#include <unity.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#endif

void test_example(void) {
    int value = 42;
    #ifdef ARDUINO
    TEST_ASSERT_EQUAL_INT(42, value);
    #else
    if (value != 42) { printf("FAIL\n"); }
    #endif
}

#ifdef ARDUINO
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_example);
    UNITY_END();
}
void loop() {}
#else
int main() {
    test_example();
    return 0;
}
#endif
```

## Current Tests

- `test_scale.cpp` - Scale class tests (median/average filtering)
- More tests to be added as features are developed

## Notes

- Native tests run on every push/PR (see `.github/workflows/build-dev.yml`)
- Hardware tests require USB connection to ESP32-S3 XIAO
- Serial output for hardware tests requires Windows serial monitor or proper USB passthrough
- Always verify changes on actual ESP32-S3 hardware before release

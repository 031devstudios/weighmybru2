# PlatformIO Unit Testing

This directory contains unit tests for the WeighMyBru2 firmware.

## Running Tests

```bash
# Run all tests on native platform
pio test -e native

# Run tests with verbose output
pio test -e native -v

# Run specific test file
pio test -e native -f test_scale
```

## Test Structure

- `test_*.cpp` - Test files following PlatformIO naming convention
- Tests run on native Linux/macOS platform (no hardware required)
- Uses Unity test framework (built into PlatformIO)

## Adding New Tests

1. Create a new file: `test/test_yourfeature.cpp`
2. Include Unity framework and any modules you're testing
3. Write test functions with `TEST_CASE` or basic test functions
4. Run `pio test -e native` to execute

## Example Test Structure

```cpp
#include <Arduino.h>
#include <unity.h>

void setUp(void) {
    // Setup code runs before each test
}

void tearDown(void) {
    // Cleanup code runs after each test
}

void test_example(void) {
    TEST_ASSERT_TRUE(true);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_example);
    UNITY_END();
}

void loop() {
    // Not used in unit tests
}
```

## Current Tests

- `test_scale.cpp` - Scale class tests (filtering, brewing detection)
- More tests to be added as features are developed

## Notes

- Unit tests do NOT replace hardware testing
- Always verify changes on actual ESP32-S3 hardware
- Test on BOTH board variants (Supermini AND XIAO)

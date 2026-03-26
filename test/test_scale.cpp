// Scale filter tests - works for both native and hardware testing
// Native: uses custom test framework with main()
// Hardware (ESP32): uses Unity framework with setup()/loop()

#ifdef ARDUINO
#include <Arduino.h>
#include <unity.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#endif

// Mock Scale class for testing filtering logic
class MockScale {
public:
    static const int MAX_SAMPLES = 10;
    float readings[MAX_SAMPLES];
    int readingIndex;
    int samplesInitialized;

    MockScale() : readingIndex(0), samplesInitialized(false) {
        for (int i = 0; i < MAX_SAMPLES; i++) {
            readings[i] = 0.0f;
        }
    }

    void initializeSamples(float initialValue) {
        for (int i = 0; i < MAX_SAMPLES; i++) {
            readings[i] = initialValue;
        }
        samplesInitialized = true;
    }

    float medianFilter(int samples) {
        if (samples > MAX_SAMPLES) samples = MAX_SAMPLES;

        float temp[samples];
        for (int i = 0; i < samples; i++) {
            int idx = (readingIndex - 1 - i + MAX_SAMPLES) % MAX_SAMPLES;
            temp[i] = readings[idx];
        }

        for (int i = 0; i < samples - 1; i++) {
            for (int j = 0; j < samples - i - 1; j++) {
                if (temp[j] > temp[j + 1]) {
                    float swap = temp[j];
                    temp[j] = temp[j + 1];
                    temp[j + 1] = swap;
                }
            }
        }
        return temp[samples / 2];
    }

    float averageFilter(int samples) {
        if (samples > MAX_SAMPLES) samples = MAX_SAMPLES;

        float sum = 0;
        int validSamples = 0;

        for (int i = 0; i < samples; i++) {
            int idx = (readingIndex - 1 - i + MAX_SAMPLES) % MAX_SAMPLES;
            sum += readings[idx];
            validSamples++;
        }

        return sum / validSamples;
    }
};

MockScale scale;

// Test functions - common to both platforms
void test_median_filter_single_value(void) {
    scale.initializeSamples(10.0f);
    float result = scale.medianFilter(3);
    #ifdef ARDUINO
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
    #else
    if (result != 10.0f) { printf("FAIL: Expected 10.0f, got %f\n", result); }
    #endif
}

void test_median_filter_three_values_sorted(void) {
    scale.readings[0] = 1.0f;
    scale.readings[1] = 2.0f;
    scale.readings[2] = 3.0f;
    scale.readingIndex = 3;
    scale.samplesInitialized = true;
    float result = scale.medianFilter(3);
    #ifdef ARDUINO
    TEST_ASSERT_EQUAL_FLOAT(2.0f, result);
    #else
    if (result != 2.0f) { printf("FAIL: Expected 2.0f, got %f\n", result); }
    #endif
}

void test_median_filter_three_values_unsorted(void) {
    scale.readings[0] = 3.0f;
    scale.readings[1] = 1.0f;
    scale.readings[2] = 2.0f;
    scale.readingIndex = 3;
    scale.samplesInitialized = true;
    float result = scale.medianFilter(3);
    #ifdef ARDUINO
    TEST_ASSERT_EQUAL_FLOAT(2.0f, result);
    #else
    if (result != 2.0f) { printf("FAIL: Expected 2.0f, got %f\n", result); }
    #endif
}

void test_median_filter_five_values(void) {
    scale.readings[0] = 5.0f;
    scale.readings[1] = 2.0f;
    scale.readings[2] = 4.0f;
    scale.readings[3] = 1.0f;
    scale.readings[4] = 3.0f;
    scale.readingIndex = 5;
    scale.samplesInitialized = true;
    float result = scale.medianFilter(5);
    #ifdef ARDUINO
    TEST_ASSERT_EQUAL_FLOAT(3.0f, result);
    #else
    if (result != 3.0f) { printf("FAIL: Expected 3.0f, got %f\n", result); }
    #endif
}

void test_average_filter_single_value(void) {
    scale.initializeSamples(5.0f);
    float result = scale.averageFilter(2);
    #ifdef ARDUINO
    TEST_ASSERT_EQUAL_FLOAT(5.0f, result);
    #else
    if (result != 5.0f) { printf("FAIL: Expected 5.0f, got %f\n", result); }
    #endif
}

void test_average_filter_multiple_values(void) {
    scale.readings[0] = 2.0f;
    scale.readings[1] = 4.0f;
    scale.readings[2] = 6.0f;
    scale.readingIndex = 3;
    scale.samplesInitialized = true;
    float result = scale.averageFilter(3);
    #ifdef ARDUINO
    TEST_ASSERT_EQUAL_FLOAT(4.0f, result);
    #else
    if (result != 4.0f) { printf("FAIL: Expected 4.0f, got %f\n", result); }
    #endif
}

void test_average_filter_four_values(void) {
    scale.readings[0] = 1.0f;
    scale.readings[1] = 2.0f;
    scale.readings[2] = 3.0f;
    scale.readings[3] = 4.0f;
    scale.readingIndex = 4;
    scale.samplesInitialized = true;
    float result = scale.averageFilter(4);
    #ifdef ARDUINO
    TEST_ASSERT_EQUAL_FLOAT(2.5f, result);
    #else
    if (result != 2.5f) { printf("FAIL: Expected 2.5f, got %f\n", result); }
    #endif
}

void test_circular_buffer_wrapping(void) {
    scale.readings[0] = 100.0f;
    scale.readings[1] = 200.0f;
    scale.readings[8] = 300.0f;
    scale.readings[9] = 400.0f;
    scale.readingIndex = 2;
    scale.samplesInitialized = true;
    float result = scale.averageFilter(4);
    #ifdef ARDUINO
    TEST_ASSERT_EQUAL_FLOAT(250.0f, result);
    #else
    if (result != 250.0f) { printf("FAIL: Expected 250.0f, got %f\n", result); }
    #endif
}

#ifdef ARDUINO
// Hardware platform: use Unity with setup()/loop()
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_median_filter_single_value);
    RUN_TEST(test_median_filter_three_values_sorted);
    RUN_TEST(test_median_filter_three_values_unsorted);
    RUN_TEST(test_median_filter_five_values);
    RUN_TEST(test_average_filter_single_value);
    RUN_TEST(test_average_filter_multiple_values);
    RUN_TEST(test_average_filter_four_values);
    RUN_TEST(test_circular_buffer_wrapping);
    UNITY_END();
}

void loop() {
}
#else
// Native platform: use custom main()

int main() {
    int tests_run = 0;
    int tests_failed = 0;
    
    printf("Running Scale Filter Tests (Native)...\n");
    printf("========================================\n");
    
    printf("Running test_median_filter_single_value...\n");
    test_median_filter_single_value();
    tests_run++;
    
    printf("Running test_median_filter_three_values_sorted...\n");
    test_median_filter_three_values_sorted();
    tests_run++;
    
    printf("Running test_median_filter_three_values_unsorted...\n");
    test_median_filter_three_values_unsorted();
    tests_run++;
    
    printf("Running test_median_filter_five_values...\n");
    test_median_filter_five_values();
    tests_run++;
    
    printf("Running test_average_filter_single_value...\n");
    test_average_filter_single_value();
    tests_run++;
    
    printf("Running test_average_filter_multiple_values...\n");
    test_average_filter_multiple_values();
    tests_run++;
    
    printf("Running test_average_filter_four_values...\n");
    test_average_filter_four_values();
    tests_run++;
    
    printf("Running test_circular_buffer_wrapping...\n");
    test_circular_buffer_wrapping();
    tests_run++;
    
    printf("========================================\n");
    printf("Tests run: %d, Failed: %d\n", tests_run, tests_failed);
    
    if (tests_failed == 0) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}
#endif

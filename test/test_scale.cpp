// Lightweight test framework for native testing
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST_ASSERT_TRUE(condition) do { \
    tests_run++; \
    if (!(condition)) { \
        printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        tests_failed++; \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_INT(expected, actual) do { \
    tests_run++; \
    if ((expected) != (actual)) { \
        printf("FAIL: %s:%d: Expected %d, got %d\n", __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        tests_failed++; \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_FLOAT(expected, actual) do { \
    tests_run++; \
    if (fabs((expected) - (actual)) > 0.001f) { \
        printf("FAIL: %s:%d: Expected %f, got %f\n", __FILE__, __LINE__, (float)(expected), (float)(actual)); \
        tests_failed++; \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, actual, len) do { \
    tests_run++; \
    for (int i = 0; i < (len); i++) { \
        if (fabs((expected)[i] - (actual)[i]) > 0.001f) { \
            printf("FAIL: %s:%d: Array mismatch at index %d\n", __FILE__, __LINE__, i); \
            tests_failed++; \
            break; \
        } \
    } \
} while(0)

#define RUN_TEST(func) do { \
    printf("Running %s...\n", #func); \
    func(); \
} while(0)

#define TEST_ASSERT_FALSE(condition) TEST_ASSERT_TRUE(!(condition))

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

void setUp(void) {
    scale.readingIndex = 0;
    scale.samplesInitialized = false;
    for (int i = 0; i < MockScale::MAX_SAMPLES; i++) {
        scale.readings[i] = 0.0f;
    }
}

void tearDown(void) {
}

void test_median_filter_single_value(void) {
    scale.initializeSamples(10.0f);
    float result = scale.medianFilter(3);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
}

void test_median_filter_three_values_sorted(void) {
    scale.readings[0] = 1.0f;
    scale.readings[1] = 2.0f;
    scale.readings[2] = 3.0f;
    scale.readingIndex = 3;
    scale.samplesInitialized = true;

    float result = scale.medianFilter(3);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, result);
}

void test_median_filter_three_values_unsorted(void) {
    scale.readings[0] = 3.0f;
    scale.readings[1] = 1.0f;
    scale.readings[2] = 2.0f;
    scale.readingIndex = 3;
    scale.samplesInitialized = true;

    float result = scale.medianFilter(3);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, result);
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
    TEST_ASSERT_EQUAL_FLOAT(3.0f, result);
}

void test_average_filter_single_value(void) {
    scale.initializeSamples(5.0f);
    float result = scale.averageFilter(2);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, result);
}

void test_average_filter_multiple_values(void) {
    scale.readings[0] = 2.0f;
    scale.readings[1] = 4.0f;
    scale.readings[2] = 6.0f;
    scale.readingIndex = 3;
    scale.samplesInitialized = true;

    float result = scale.averageFilter(3);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, result);
}

void test_average_filter_four_values(void) {
    scale.readings[0] = 1.0f;
    scale.readings[1] = 2.0f;
    scale.readings[2] = 3.0f;
    scale.readings[3] = 4.0f;
    scale.readingIndex = 4;
    scale.samplesInitialized = true;

    float result = scale.averageFilter(4);
    TEST_ASSERT_EQUAL_FLOAT(2.5f, result);
}

void test_circular_buffer_wrapping(void) {
    scale.readings[0] = 100.0f;
    scale.readings[1] = 200.0f;
    scale.readings[8] = 300.0f;
    scale.readings[9] = 400.0f;
    scale.readingIndex = 2;
    scale.samplesInitialized = true;

    float result = scale.averageFilter(4);
    TEST_ASSERT_EQUAL_FLOAT(250.0f, result);
}

int main() {
    printf("Running Scale Filter Tests...\n");
    printf("========================================\n");

    RUN_TEST(test_median_filter_single_value);
    RUN_TEST(test_median_filter_three_values_sorted);
    RUN_TEST(test_median_filter_three_values_unsorted);
    RUN_TEST(test_median_filter_five_values);

    RUN_TEST(test_average_filter_single_value);
    RUN_TEST(test_average_filter_multiple_values);
    RUN_TEST(test_average_filter_four_values);
    RUN_TEST(test_circular_buffer_wrapping);

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

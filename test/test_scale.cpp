#include <Arduino.h>
#include <unity.h>

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

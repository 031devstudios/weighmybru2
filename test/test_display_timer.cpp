// Display Timer State Machine Unit Tests
// Tests the timer state machine logic in isolation (no hardware needed)
// Run with: g++ -x c++ -o /tmp/test tests/test_display_timer.cpp && /tmp/test

#ifdef ARDUINO
#include <Arduino.h>
#include <unity.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#endif

// Mimic the TimerState enum from Display.h
enum class TimerState { IDLE, RUNNING, STOPPED };

// Mimic Display's timer state variables and methods
struct TimerContext {
    unsigned long timerStartTime;
    unsigned long timerPausedTime;
    bool timerRunning;
    bool timerPaused;
    TimerState timerState;
    bool autoBrewTimerEnabled;
    bool autoBrewWaitingForStart;
    bool autoBrewWaitingForStop;
    
    TimerContext() {
        timerStartTime = 0;
        timerPausedTime = 0;
        timerRunning = false;
        timerPaused = false;
        timerState = TimerState::IDLE;
        autoBrewTimerEnabled = true;
        autoBrewWaitingForStart = true;
        autoBrewWaitingForStop = false;
    }
    
    void startTimer() {
        if (!timerRunning) {
            timerStartTime = 1000;
            timerRunning = true;
            timerPaused = false;
        } else if (timerPaused) {
            timerStartTime = 2000 - timerPausedTime;
            timerPaused = false;
        }
    }
    
    void stopTimer() {
        if (timerRunning && !timerPaused) {
            timerPausedTime = 1500;
            timerPaused = true;
        }
    }
    
    // This is the FIXED version - also resets timerState
    void resetTimer() {
        timerStartTime = 0;
        timerPausedTime = 0;
        timerRunning = false;
        timerPaused = false;
        timerState = TimerState::IDLE;
    }
    
    void resetAutoBrewDetection() {
        autoBrewWaitingForStart = true;
        autoBrewWaitingForStop = false;
    }
    
    bool isTimerRunning() const {
        return timerRunning && !timerPaused;
    }
};

// Global context for testing
TimerContext ctx;

// Test result tracking
int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        tests_run++; \
        if ((expected) == (actual)) { \
            tests_passed++; \
            printf("  PASS: %s == %s\n", #expected, #actual); \
        } else { \
            tests_failed++; \
            printf("  FAIL: %s (expected %d, got %d)\n", #expected " == " #actual, (int)(expected), (int)(actual)); \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(actual) \
    do { \
        tests_run++; \
        if (actual) { \
            tests_passed++; \
            printf("  PASS: %s is true\n", #actual); \
        } else { \
            tests_failed++; \
            printf("  FAIL: %s is false (expected true)\n", #actual); \
        } \
    } while(0)

#define TEST_ASSERT_FALSE(actual) \
    do { \
        tests_run++; \
        if (!(actual)) { \
            tests_passed++; \
            printf("  PASS: %s is false\n", #actual); \
        } else { \
            tests_failed++; \
            printf("  FAIL: %s is true (expected false)\n", #actual); \
        } \
    } while(0)

#define TEST_ASSERT_NOT_EQUAL(expected, actual) \
    do { \
        tests_run++; \
        if ((expected) != (actual)) { \
            tests_passed++; \
            printf("  PASS: %s != %s\n", #expected, #actual); \
        } else { \
            tests_failed++; \
            printf("  FAIL: %s (expected NOT %d, got %d)\n", #expected " != " #actual, (int)(expected), (int)(actual)); \
        } \
    } while(0)

// Simulate GPIO3 short press - returns true if timer control fully handled
bool simulateGPIO3ShortPress() {
    switch(ctx.timerState) {
        case TimerState::IDLE:
            ctx.startTimer();
            ctx.timerState = TimerState::RUNNING;
            printf("    [GPIO3] IDLE -> RUNNING\n");
            return false;
        case TimerState::RUNNING:
            ctx.stopTimer();
            ctx.timerState = TimerState::STOPPED;
            printf("    [GPIO3] RUNNING -> STOPPED\n");
            return false;
        case TimerState::STOPPED:
            ctx.resetTimer();
            ctx.resetAutoBrewDetection();
            // NOTE: resetTimer() now sets timerState = IDLE
            printf("    [GPIO3] STOPPED -> IDLE (fully handled)\n");
            return true;
    }
    return false;
}

// Simulate /api/tare behavior (FIXED version)
void simulateWebTare() {
    if (ctx.timerState == TimerState::STOPPED) {
        ctx.resetTimer();
    } else if (ctx.timerState == TimerState::RUNNING) {
        ctx.resetAutoBrewDetection();
    }
}

// Test 1: resetTimer() in IDLE state
void test_resetTimer_in_idle() {
    printf("\nTest 1: resetTimer() in IDLE state\n");
    ctx.timerState = TimerState::IDLE;
    ctx.timerRunning = false;
    
    ctx.resetTimer();
    
    TEST_ASSERT_EQUAL(TimerState::IDLE, ctx.timerState);
    TEST_ASSERT_FALSE(ctx.timerRunning);
}

// Test 2: resetTimer() in RUNNING state (was problematic before fix)
void test_resetTimer_in_running() {
    printf("\nTest 2: resetTimer() in RUNNING state\n");
    ctx.timerState = TimerState::RUNNING;
    ctx.timerRunning = true;
    ctx.timerPaused = false;
    
    ctx.resetTimer();
    
    // After fix: timerState should be IDLE, not RUNNING
    TEST_ASSERT_EQUAL(TimerState::IDLE, ctx.timerState);
    TEST_ASSERT_FALSE(ctx.timerRunning);
}

// Test 3: resetTimer() in STOPPED state
void test_resetTimer_in_stopped() {
    printf("\nTest 3: resetTimer() in STOPPED state\n");
    ctx.timerState = TimerState::STOPPED;
    ctx.timerRunning = false;
    ctx.timerPaused = true;
    
    ctx.resetTimer();
    
    TEST_ASSERT_EQUAL(TimerState::IDLE, ctx.timerState);
    TEST_ASSERT_FALSE(ctx.timerRunning);
    TEST_ASSERT_FALSE(ctx.timerPaused);
}

// Test 4: GPIO3 full cycle with resetTimer fix
void test_gpio3_full_cycle() {
    printf("\nTest 4: Full GPIO3 cycle (IDLE->RUNNING->STOPPED->IDLE)\n");
    
    ctx.timerState = TimerState::IDLE;
    ctx.timerRunning = false;
    
    // Press 1: IDLE -> RUNNING
    bool consumed1 = simulateGPIO3ShortPress();
    TEST_ASSERT_EQUAL(TimerState::RUNNING, ctx.timerState);
    TEST_ASSERT_TRUE(ctx.timerRunning);
    TEST_ASSERT_FALSE(consumed1);
    
    // Press 2: RUNNING -> STOPPED
    bool consumed2 = simulateGPIO3ShortPress();
    TEST_ASSERT_EQUAL(TimerState::STOPPED, ctx.timerState);
    TEST_ASSERT_FALSE(consumed2);
    TEST_ASSERT_FALSE(ctx.isTimerRunning());  // timer is stopped even if timerRunning=true
    
    // Press 3: STOPPED -> IDLE (resetTimer now sets state)
    bool consumed3 = simulateGPIO3ShortPress();
    TEST_ASSERT_EQUAL(TimerState::IDLE, ctx.timerState);
    TEST_ASSERT_TRUE(consumed3);
    TEST_ASSERT_FALSE(ctx.timerRunning);
}

// Test 5: /api/tare in RUNNING should NOT call resetTimer
void test_web_tare_in_running() {
    printf("\nTest 5: /api/tare in RUNNING (should NOT reset timer)\n");
    ctx.timerState = TimerState::RUNNING;
    ctx.timerRunning = true;
    ctx.autoBrewWaitingForStart = false;
    ctx.autoBrewWaitingForStop = true;
    
    simulateWebTare();
    
    // Timer should still be RUNNING
    TEST_ASSERT_EQUAL(TimerState::RUNNING, ctx.timerState);
    TEST_ASSERT_TRUE(ctx.timerRunning);
    TEST_ASSERT_FALSE(ctx.autoBrewWaitingForStop);
    TEST_ASSERT_TRUE(ctx.autoBrewWaitingForStart);
}

// Test 6: /api/tare in STOPPED should call resetTimer
void test_web_tare_in_stopped() {
    printf("\nTest 6: /api/tare in STOPPED (should reset timer)\n");
    ctx.timerState = TimerState::STOPPED;
    ctx.timerRunning = false;
    ctx.timerPaused = true;
    
    simulateWebTare();
    
    // Timer should be reset to IDLE
    TEST_ASSERT_EQUAL(TimerState::IDLE, ctx.timerState);
    TEST_ASSERT_FALSE(ctx.timerRunning);
    TEST_ASSERT_FALSE(ctx.timerPaused);
}

// Test 7: /api/tare in IDLE should do nothing
void test_web_tare_in_idle() {
    printf("\nTest 7: /api/tare in IDLE (should do nothing)\n");
    ctx.timerState = TimerState::IDLE;
    ctx.timerRunning = false;
    
    simulateWebTare();
    
    // Should remain IDLE
    TEST_ASSERT_EQUAL(TimerState::IDLE, ctx.timerState);
    TEST_ASSERT_FALSE(ctx.timerRunning);
}

// Test 8: isTimerRunning() consistency after resetTimer in RUNNING
void test_isTimerRunning_after_reset_in_running() {
    printf("\nTest 8: isTimerRunning() after resetTimer in RUNNING\n");
    ctx.timerState = TimerState::RUNNING;
    ctx.timerRunning = true;
    
    // Before reset
    TEST_ASSERT_TRUE(ctx.isTimerRunning());
    
    ctx.resetTimer();
    
    // After reset - should return false
    TEST_ASSERT_FALSE(ctx.isTimerRunning());
}

// Test 9: State consistency - timerRunning and timerState match
void test_state_consistency() {
    printf("\nTest 9: State consistency checks\n");
    
    // IDLE state
    ctx.timerState = TimerState::IDLE;
    ctx.timerRunning = false;
    bool idleConsistent = (ctx.timerState == TimerState::IDLE) && (!ctx.timerRunning || ctx.timerPaused);
    TEST_ASSERT_TRUE(idleConsistent);
    
    // RUNNING state
    ctx.timerState = TimerState::RUNNING;
    ctx.timerRunning = true;
    ctx.timerPaused = false;
    bool runningConsistent = (ctx.timerState == TimerState::RUNNING) && ctx.timerRunning && !ctx.timerPaused;
    TEST_ASSERT_TRUE(runningConsistent);
    
    // STOPPED state
    ctx.timerState = TimerState::STOPPED;
    ctx.timerRunning = false;
    ctx.timerPaused = true;
    bool stoppedConsistent = (ctx.timerState == TimerState::STOPPED) && !ctx.timerRunning && ctx.timerPaused;
    TEST_ASSERT_TRUE(stoppedConsistent);
}

// Test 10: Inconsistent state detection - old bug scenario
void test_inconsistent_state_detection() {
    printf("\nTest 10: Inconsistent state (old bug - timerRunning=false but state=RUNNING)\n");
    
    // Simulate the bug: timerRunning=false but state=RUNNING
    ctx.timerState = TimerState::RUNNING;
    ctx.timerRunning = false;  // Inconsistent!
    
    // isTimerRunning() should return false even if state says RUNNING
    bool isRunning = ctx.isTimerRunning();
    TEST_ASSERT_FALSE(isRunning);
    
    // Now fix it
    ctx.resetTimer();
    
    // Should now be consistent
    TEST_ASSERT_EQUAL(TimerState::IDLE, ctx.timerState);
    TEST_ASSERT_FALSE(ctx.timerRunning);
    TEST_ASSERT_FALSE(ctx.isTimerRunning());
}

#ifndef ARDUINO
int main() {
    printf("========================================\n");
    printf("Display Timer State Machine Unit Tests\n");
    printf("========================================\n");
    
    // Run tests
    test_resetTimer_in_idle();
    test_resetTimer_in_running();
    test_resetTimer_in_stopped();
    test_gpio3_full_cycle();
    test_web_tare_in_running();
    test_web_tare_in_stopped();
    test_web_tare_in_idle();
    test_isTimerRunning_after_reset_in_running();
    test_state_consistency();
    test_inconsistent_state_detection();
    
    // Summary
    printf("\n========================================\n");
    printf("Test Results: %d run, %d passed, %d failed\n", tests_run, tests_passed, tests_failed);
    printf("========================================\n");
    
    if (tests_failed > 0) {
        printf("SOME TESTS FAILED!\n");
        return 1;
    } else {
        printf("ALL TESTS PASSED!\n");
        return 0;
    }
}
#endif

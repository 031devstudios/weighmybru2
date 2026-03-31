# Vivecoding Guide: From Idea to Implementation

*A practical guide to agent-assisted embedded development*

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [The Vivecoding Workflow](#2-the-vivecoding-workflow)
3. [Phase 1: Foundation](#3-phase-1-foundation)
4. [Phase 2: Planning](#4-phase-2-planning)
5. [Phase 3: Implementation](#5-phase-3-implementation)
6. [Debugging and Iteration](#6-debugging-and-iteration)
7. [What Worked, What Didn't](#7-what-worked-what-didnt)
8. [Prompt Templates](#8-prompt-templates)
9. [The AutoBrewTimer Story](#9-the-autobrewtimer-story)

---

## 1. Introduction

### What is Vivecoding?

Vivecoding is collaborative development where a human and an AI agent work together iteratively. The human provides direction, validates results, and handles hardware testing. The agent explores codebases, plans features, generates code, and runs tests.

### Why This Guide?

This guide documents the workflow we developed while building the AutoBrewTimer feature for WeighMyBru2—an ESP32-S3 coffee scale with web UI, BLE, and real-time flow detection.

The guide teaches through real examples, showing both successes and failures, because learning from actual debug sessions is more valuable than polished tutorials.

---

## 2. The Vivecoding Workflow

```
┌─────────────────────────────────────────────────────────────┐
│                    THREE-PHASE APPROACH                      │
├─────────────────────────────────────────────────────────────┤
│  Phase 1: Foundation (Plan Mode)                           │
│  └── Explore, document, setup testing infrastructure        │
├─────────────────────────────────────────────────────────────┤
│  Phase 2: Planning (Plan Mode)                             │
│  └── Design feature, get user approval, plan implementation │
├─────────────────────────────────────────────────────────────┤
│  Phase 3: Implementation (Build Mode)                      │
│  └── Code, test, debug, iterate, commit                   │
└─────────────────────────────────────────────────────────────┘
```

### Key Principle: Mode Awareness

The agent operates in two modes:

- **Plan Mode**: Read-only. Explores codebase, creates plans, asks questions. No file modifications.
- **Build Mode**: Full read/write access. Implements plans, runs tests, commits changes.

Never mix modes in a single response. If a prompt asks for planning then implementation, respond with only the planning portion until the user says to proceed.

---

## 3. Phase 1: Foundation

### Goal

Build a foundation for productive agent-assisted development.

### What to Create

1. **AGENTS.md**: Project-specific conventions, architecture, and build commands
2. **Native Test Environment**: PlatformIO configuration for host-side testing
3. **Basic Unit Tests**: Template tests that can be extended

### The Initial Prompt

```
I have not enough experience in C/embedded. Build a foundation 
for agent-assisted development on this ESP32 project. 

This phase is about creating infrastructure:
1. Create AGENTS.md with project conventions (naming, architecture)
2. Add native test environment to platformio.ini
3. Create basic unit tests

Use plan mode first. Explore the codebase, then create the files.
```

### What Worked

- Creating `AGENTS.md` early gave the agent clear conventions to follow
- Native testing allowed logic validation before hardware flash
- The agent naturally documented edge cases it discovered

### What Didn't

- PlatformIO's native test integration was minimal—tests needed manual compilation
- Solution: Compile test files directly with g++ instead of using `pio test`

---

## 4. Phase 2: Planning

### Goal

Transform a vague feature idea into a detailed implementation plan.

### The Feature Prompt

```
We want to add Auto Brew Timer to the coffee scale:
- Checkbox on web dashboard to enable/disable
- Timer starts automatically when brewing is detected
- Timer stops when brewing ends
- User can toggle with dual-button press (GPIO3 + GPIO4)

Use plan mode to design:
1. Brewing detection strategy (debounce, thresholds)
2. Timer state machine (IDLE, RUNNING, STOPPED)
3. Web API endpoints
4. Settings persistence
5. OLED display behavior

Propose a state machine architecture. Get approval before implementing.
```

### Key Design Decisions

#### State Machine Pattern

For the AutoBrewTimer, we designed a simple but effective state machine:

```cpp
enum class TimerState { IDLE, RUNNING, STOPPED };

// GPIO3 short press behavior:
switch(timerState) {
    case TimerState::IDLE:
        startTimer();
        timerState = TimerState::RUNNING;
        break;
    case TimerState::RUNNING:
        stopTimer();
        timerState = TimerState::STOPPED;
        break;
    case TimerState::STOPPED:
        resetTimer();
        timerState = TimerState::IDLE;
        break;
}
```

This pattern worked well because:
- State transitions are explicit and traceable
- Debug output shows exactly which state triggered
- Testing is straightforward with mock state machines

#### Flow-Based Detection vs. Raw Weight

**Initial approach**: Detect brewing by weight change magnitude.

**Problem**: Scale is too sensitive—vibrations triggered false positives.

**Solution**: Use flow rate instead of raw weight.

```
Flow rate > threshold for 6 consecutive samples (stable flow)
AND |slope| < threshold (linear, not spike)
→ Real brewing detected
```

This was a key insight: the system already calculated flow rate, but we were ignoring it for detection.

### What Worked

- Detailed planning caught the "dual handler" problem before implementation
- User's domain knowledge (coffee brewing has consistent flow) shaped the solution
- State machine diagram made expectations clear to both parties

### What Didn't

- Some plans assumed hardware behavior that didn't match reality (button bounce)
- Solution: Add debug output to understand actual behavior, then adapt plan

---

## 5. Phase 3: Implementation

### Goal

Implement the planned feature with iterative testing.

### Implementation Prompt

```
Proceed with Auto Brew Timer implementation:

Priority order:
1. Display state machine (TimerState enum, transitions)
2. Flow-based detection in Display.cpp
3. Web API endpoints
4. Settings page UI
5. Button handlers

Keep changes small. Build and verify after each step.
```

### The Build Loop

```
┌─────────────────────────────────────────────────────────────┐
│                    ITERATIVE BUILD LOOP                      │
├─────────────────────────────────────────────────────────────┤
│  1. Make code change                                       │
│  2. pio run -e esp32s3-xiao        # Build locally         │
│  3. pio run -e esp32s3-xiao -t upload  # Flash hardware   │
│  4. Test on hardware, share serial output                  │
│  5. Report results                                         │
│  6. Repeat                                                │
└─────────────────────────────────────────────────────────────┘
```

### Small Steps Principle

We implemented in this order:

1. **State machine enum** in Display.h (5 lines)
2. **TimerState member** and constructor init
3. **GPIO3 handler** with state transitions
4. **Auto-brew detection** using flow rate samples
5. **Web API** for threshold setting
6. **Settings page** HTML form

Each step was buildable and testable.

---

## 6. Debugging and Iteration

### The Debug Loop

Hardware debugging requires the user to test and share serial output. This is unavoidable for embedded systems.

```
┌─────────────────────────────────────────────────────────────┐
│                  HARDWARE DEBUG LOOP                         │
├─────────────────────────────────────────────────────────────┤
│  User: "Timer doesn't stop when brewing ends"               │
│  Agent: Analyzes serial output, identifies bug              │
│  Agent: "Found issue: onWeightStableDetected() resets      │
│          brewingSampleCount even when timer never started"   │
│  Agent: Proposes fix, user approves                         │
│  Agent: Implements fix                                      │
│  User: Reflashes, tests, shares new output               │
│  Repeat until working                                      │
└─────────────────────────────────────────────────────────────┘
```

### Debug Output Strategy

Add serial output that shows decision-making:

```cpp
Serial.printf("[AutoBrew] Sample %d/%d received\n", 
             brewingSampleCount, autoBrewDebounceSamples);

if (brewingSampleCount >= autoBrewDebounceSamples) {
    Serial.printf("[AutoBrew] STARTING TIMER! back-calculated\n");
}
```

This output was crucial for understanding why the timer started/stopped at unexpected times.

### Example: The Button Bounce Problem

**Symptom**: GPIO3 press sometimes triggered timer twice.

**Investigation**:
```
[Timer] GPIO3: STOPPED -> resetting timer, state=IDLE
Timer control consumed by Display state machine
Timer control touch started      <-- This shouldn't happen!
```

**Root Cause**: The capacitive touch sensor generates button bounce—multiple HIGH/LOW transitions from one press. Our 200ms debounce wasn't enough.

**Failed Solutions**:
- Single re-entry flag didn't work (bounce happened faster than flag could prevent)
- Increasing to 400ms debounce helped but didn't fully solve it

**Working Solution**: Remove the dual-handler pattern entirely. PowerManager only reads the touch sensor; Display's state machine handles all timer logic. One owner, no race conditions.

```cpp
// PowerManager.cpp - BEFORE (problematic)
if (displayPtr != nullptr) {
    displayPtr->onSleepButtonShortPress();  // Handler 1
}
handleTimerControl();  // Handler 2 - caused double-trigger!

// AFTER (clean)
if (displayPtr != nullptr) {
    displayPtr->onSleepButtonShortPress();  // Sole handler
}
// No handleTimerControl() call
```

---

## 7. What Worked, What Didn't

### What Worked

| Technique | Why |
|-----------|-----|
| State machine enums | Explicit, testable, traceable state |
| Mock-based unit tests | Validate logic before hardware |
| Serial debug output | Essential for understanding runtime behavior |
| Small incremental steps | Each change is verifiable |
| User hardware testing | Cannot be simulated for embedded |
| Flow rate for detection | More stable than raw weight |
| Single-source-of-truth | One module owns each feature |

### What Didn't

| Problem | Cause | Solution |
|---------|-------|---------|
| WSL2 USB passthrough | Kernel/USB limitation | Wait for device, use native Linux |
| PlatformIO native tests | Not fully integrated | Compile with g++ directly |
| Button bounce | Hardware issue, not code | Remove dual handlers, increase debounce |
| Multiple state owners | Architecture problem | Consolidate to single owner |
| Removing code vs. patching | Accumulated complexity | Option C: rewrite cleaner |

### Lessons Learned

1. **Trust hardware testing over assumptions**: Serial output revealed button bounce that logic couldn't predict.

2. **Remove dual handlers**: When two modules modify the same state, race conditions occur. Consolidate to one owner.

3. **Flow > Raw Weight**: For detection, processed signals (flow rate) work better than raw signals (weight change).

4. **Mock tests catch logic bugs**: We found state transition bugs in unit tests before flashing hardware.

---

## 8. Prompt Templates

### Foundation Prompt

```
I want to set up agent-assisted development on this ESP32 project.

Create:
1. AGENTS.md - Project conventions (naming, architecture, build commands)
2. Native test environment in platformio.ini
3. Basic unit test template in test/

Use plan mode first. Explore the codebase structure.
```

### Feature Planning Prompt

```
I want to add [FEATURE] to this coffee scale.

Requirements:
- [User requirement 1]
- [User requirement 2]
- [User requirement 3]

Use plan mode to design:
1. State machine (if applicable)
2. Detection/triggering logic
3. User feedback (display, web)
4. Settings persistence
5. Edge cases

Propose architecture. Include a state diagram if relevant.
```

### Implementation Prompt

```
Proceed with [FEATURE] implementation.

Priority order:
1. [Most critical component]
2. [Supporting component]
3. [UI/UX components]
4. [Testing/integration]

Build after each step. Report any compilation errors.
```

### Debug Prompt

```
I'm seeing unexpected behavior:
[Symptom description]

Serial output:
```
[paste relevant serial log]
```

Analyze the log. Identify the bug. Propose a fix.
```

### Commit Prompt

```
Commit all [FEATURE] changes to a feature branch.

Branch name: [feature-name]

Commit message should describe:
- What changed
- Architecture decisions
- Testing performed

Do NOT push to remote unless explicitly asked.
```

---

## 9. The AutoBrewTimer Story

### The Beginning

User: "I have not enough experience in C. Build a foundation for agent-assisted development."

Agent created AGENTS.md with conventions, added native tests, and documented the architecture. This foundation made the subsequent feature development smoother.

### Planning the Feature

User wanted automatic coffee extraction timing. The agent explored the codebase and found:
- Weight detection existed
- Flow rate was calculated but not used for detection
- Timer system was simple (start/stop/reset)

Initial design used raw weight change. User said: "Too sensitive. Use flow rate—coffee has consistent flow when brewing."

This feedback was crucial. The agent redesigned detection to use flow rate slope analysis.

### Implementation Struggles

**First attempt**: Weight-based detection triggered on vibrations.

**Second attempt**: Flow-based detection worked but timer stopped too early (2s stability timeout).

**Third attempt**: Slope-based detection worked but button press caused double-trigger (button bounce).

**Fourth attempt**: Consolidated to single handler—fixed.

### Key Moments

1. **User's domain knowledge**: "Coffee flow is linear, you can use slope to detect it"
2. **User's patience**: Testing each firmware flash, sharing detailed serial output
3. **Unit tests**: Catching state transition bugs before hardware

### The Final Solution

```
Auto-brew detection:
1. Flow rate > 0.7 g/s for 6 consecutive samples
2. |slope| < 0.5 g/s² (stable flow)
3. Back-calculate timer start from first sample

Timer states:
- IDLE: Waiting for brew or user action
- RUNNING: Timer active, monitoring for stop
- STOPPED: Brew ended, showing final time

GPIO3 short press:
- IDLE → start timer
- RUNNING → stop timer
- STOPPED → reset and re-enable auto-brew
```

---

## Quick Reference

### Build Commands

```bash
# Build
pio run -e esp32s3-xiao

# Flash
pio run -e esp32s3-xiao -t upload

# Filesystem
pio run -e esp32s3-xiao -t uploadfs

# Native test
g++ -x c++ -o /tmp/test test/test_timer_state.cpp && /tmp/test
```

### Debug Output Levels

```cpp
// State transitions
Serial.println("[Timer] GPIO3: STOPPED -> IDLE");

// Detection events
Serial.printf("[AutoBrew] AUTO-START! slope=%.2f\n", slope);

// Errors
Serial.println("[ERROR] displayPtr is null");
```

### State Machine Checklist

When designing a feature with states:

- [ ] Define all possible states
- [ ] Define all valid transitions
- [ ] Define what triggers each transition
- [ ] Add debug output for transitions
- [ ] Test each transition manually
- [ ] Consider race conditions (dual handlers?)

---

*This guide was created from the actual development experience of building the AutoBrewTimer feature. The problems, solutions, and lessons learned are real—not sanitized for presentation.*

**WeighMyBru2 Project**: github.com/031devstudios/weighmybru2  
**AutoBrewTimer Branch**: github.com/031devstudios/weighmybru2/tree/AutoBrewTimer

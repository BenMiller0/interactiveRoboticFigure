# Legacy Animatronic Control Systems

This directory contains the original Arduino and C implementations that were used before the current C++ architecture. These systems are provided for reference and historical purposes.

## Overview

The legacy implementation consists of two main components:
1. **Arduino-based body control** (`body.ino`) - Handles servo movement for head, base, and wings
2. **C-based mouth control** (`taromouth.c`) - Audio-driven mouth synchronization

## Files

### `body.ino`
Arduino sketch for controlling the physical movement of the animatronic figure.

**Features:**
- Dual joystick control for base and head rotation
- Wing flapping with configurable speed profiles
- EEPROM memory for position persistence
- Angry mode with rapid wing movement
- Head shaking motion for "no" gesture
- Smooth servo motion with dynamic speed profiles

**Hardware Requirements:**
- Arduino board (Uno/Nano compatible)
- 3x Servo motors (base, head, wings)
- 2x Joysticks with button press capability
- 1x Push button for wing control
- External power supply for servos

**Pin Configuration:**
- SERVO_PIN_0 (7): Base servo
- SERVO_PIN_1 (6): Head servo  
- SERVOS (10): Wings servo
- BUTTON (9): Wing flap button
- JOYSTICK_BUTTON_1 (4): Angry mode trigger
- JOYSTICK_BUTTON_2 (5): Head shake trigger
- y0Pin (A0): Base joystick Y-axis
- y1Pin (A2): Head joystick Y-axis

**Controls:**
- **Left Joystick**: Base rotation (left/right)
- **Right Joystick**: Head rotation (up/down)
- **Button**: Single wing flap
- **Joystick 1 Press**: Angry mode (4 seconds of rapid flapping)
- **Joystick 2 Press**: Head shaking motion

### `taromouth.c`
C program for audio-driven mouth synchronization using ALSA and WiringPi.

**Features:**
- Real-time audio capture from USB microphone
- Amplitude-based mouth movement calculation
- Smooth servo motion with speed limiting
- Configurable dead zones and thresholds
- Safety margins to prevent servo damage
- ALSA audio interface for low-latency capture

**Dependencies:**
- ALSA development libraries (`libasound2-dev`)
- WiringPi library for GPIO control
- C compiler (gcc)

**Configuration:**
- Sample Rate: 48 kHz
- Buffer Size: 1024 frames
- Servo Range: 75-100 degrees (limited for safety)
- Audio Device: `plughw:CARD=Device,DEV=0`

**Build and Run:**
```bash
make
./tarosound
```

## Migration to Current System

The legacy systems have been consolidated into the modern C++ architecture with the following improvements:

### From Arduino (`body.ino`) → C++ Actuation System
- **Mouth.h/.cpp**: Audio-driven mouth control (replaces `taromouth.c`)
- **Neck.h/.cpp**: Enhanced neck/head servo control
- **Wings.h/.cpp**: Improved wing control with cooldown timers
- **RandomController.h/.cpp**: Autonomous movement behaviors
- **PCA9685.h/.cpp**: I2C servo driver (replaces direct Arduino servo control)

### Key Improvements
1. **Unified Architecture**: All components integrated into single C++ application
2. **Better Hardware Interface**: I2C PWM driver instead of direct servo control
3. **Enhanced Audio**: Multi-threaded audio processing with dual output
4. **AI Integration**: Voice interaction and conversation capabilities
5. **Terminal UI**: Real-time status monitoring and debugging
6. **Modular Design**: Separated concerns for better maintainability

## Limitations of Legacy Systems

### Arduino Implementation
- Limited to simple servo control
- No audio integration
- No autonomous behaviors
- Position persistence only via EEPROM
- Limited debugging capabilities

### C Mouth Control
- Single-purpose functionality
- No integration with body movements
- Hardware-specific (WiringPi dependency)
- Limited configurability
- No user interface

## Usage Notes

These legacy systems are provided for:
- Educational purposes and reference
- Understanding the evolution of the project
- Potential use in simpler hardware setups
- Debugging and comparison with the current system

For new installations or development, use the main C++ system described in the project root README.

## Hardware Compatibility

The legacy systems were designed for:
- Arduino Uno/Nano or compatible boards
- Raspberry Pi with WiringPi (for mouth control)
- Direct servo connection (no I2C driver)
- Simple joystick and button inputs

The current system requires:
- Raspberry Pi 5
- PCA9685 I2C PWM driver
- USB audio devices
- More advanced servo setup
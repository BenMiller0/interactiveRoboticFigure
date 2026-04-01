# Taro's Codebase - Architecture Document

This interactive robotic figure is built as a modular embedded C++ application with clear separation of concerns across five primary architectural layers with modules outlined below. The purpose of this architecture is to clearly define and translate hardware to software and back.

Taro is intentionally built with modularized layers outlined in the sections below. If something breaks, isolate the layer. Debug one layer at a time. Debug one layer at a time. Below is a visual of how the layers are organized from the main.cpp software down to the servo motors: 

```
main.cpp
|
AI Integration Layer
AIVoice
|
Control Layer
TaroUI / RandomController
|                                      |
Actuation Layer           Audio Layer
Neck / Wings / Mouth    Audio 
|                                      |
Hardware Abstraction Layer
PCA9685
|
I2C (0x40)
|
PCA9685 Servo Controller
├─ Channel 0  → Left Wing
├─ Channel 1  → Right Wing
├─ Channel  2 → Neck
└─ Channel  3 → Mouth
```

## 1. Hardware Abstraction Layer

This layer directly interacts with the motor controller via I2C communication, which handles the communication to the servo motors.

### PCA9685
**Purpose**: Low-level I2C communication with the PCA9685 PWM controller

**Responsibilities**: 
- Initialize I2C device connection (`/dev/i2c-1`)
- Configure PWM frequency and servo control
- Provide servo angle and pulse width control methods

**Important Interfaces**: `setPWM()`, `setServoAngle()`, `setServoPulse()` 

## 2. Actuation Layer

This layer calls the interfaces defined in the hardware abstraction layer to move servos on specific joints of the robotic figure. 

### Neck 
**Hardware**: Channel 2 servo, 500-2500μs pulse range

**Features**: Smooth motion interpolation, recentering capability

**State Management**: Current/target position tracking

**Key Methods**: `turnLeft()`, `turnRight()`, `recenter()`, `update()` 

### Mouth  
**Hardware**: Channel 3 servo, 850-1300μs pulse range

**Features**: Audio-reactive animation, pause/resume functionality

**Audio Integration**: Real-time audio frame processing for lip sync

**State Management**: Servo position smoothing and movement thresholds

### Wings 
**Hardware**: Dual servos (channels 0-1) with asymmetric movement

**Features**: Cooldown management, timed flap animations

**Timing**: 600ms up delay, 2000ms flap cooldown

## 3. Audio Processing Layer

Real-time audio capture and playback for lip-sync and voice interaction. ALSA (Advanced Linux Sound Architecture) based audio subsystem with callback-driven processing.

### Audio
**Hardware Interface**:
- Input: plughw:CARD=Device,DEV=0 (microphone)
- Output: plughw:CARD=UACDemoV10,DEV=0 and plughw:CARD=Device_1,DEV=0 (speakers)

**NOTE**: Sometimes these constants needs to be changed because the names of these devices randomly change on rebooting

**Audio Specifications**: 48kHz sample rate, mono, 1024-frame buffers

**Features**:
- Real-time audio frame processing
- Pause/resume functionality
- Callback-based architecture for integration with Mouth controller

**Threading**: Dedicated audio thread for continuous stream processing

**Integration**: Provides audio frames to Mouth controller via FrameCallback

## 4. Control Layer

This layer is responsible for giving the user control of the figure or having the figure be randomly controlled.

### TaroUI
**Display**: Terminal-based real-time status visualization

**Features**: ANSI color codes, servo position bars, control hints

**Update Modes**: Normal and random mode displays

**Rendering**: Buffered output with frame rate control

### Random Controller
**Purpose**: Autonomous behavior generation

**Features**: Activity level control (1-10), timed random actions

**Integration**: Coordinates neck and wing movements

## 5. AI Integration Layer

This was kinda just a side project I took on and honestly would not give too much effort to understanding how it works as it uses a lot of advanced OS level concepts like fork().

### AIVoice
**Architecture**: Inter-process communication (pipes) with Python AI backend

**State Machine**: IDLE → READY → LISTENING → PROCESSING → SPEAKING

**Features**: Voice recognition, speech synthesis, amplitude extraction

**Threading**: Asynchronous communication with dedicated reader thread

## Main Loop Architecture

The application follows a reactive event loop pattern with three concurrent processing streams that all stem from the main function in main.cpp. The lines included next to each stream are the corresponding lines of code in main.cpp that handle that stream.

### 1. Input Processing Stream (Lines 26-57)
- Non-blocking stdin character reading
- Command dispatching to appropriate controllers
- Mode switching logic (manual vs AI vs random)

### 2. AI State Management Stream (Lines 59-71)
- State transition monitoring
- Mouth servo automation during speech
- Cross-component coordination

### 3. Update Stream (Lines 73-83)
- Servo position interpolation (`neck.update()`)
- Random behavior execution (`random.update()`)
- UI refresh with current system state

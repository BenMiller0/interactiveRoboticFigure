# Interactive Robotic Figure - Taro The Talking Bird

This project is a Raspberry Pi based animatronic control system designed to drive a character named "Taro." It combines real-time audio processing with servo control to create responsive mouth movement synchronized to sound, along with manual controls for head movement and wing flapping.

Watch taro in action:

<a href="https://www.youtube.com/shorts/PKlPhNjFk18">
    <img src="https://img.youtube.com/vi/PKlPhNjFk18/0.jpg" height="200">
</a>

Taro has been seen around campus talking to students:

<p>
    <img src="assets/promotional_material/Tabling_With_Taro.png" alt="Concept Art" height="150">
    <img src="assets/promotional_material/Tabling_With_Taro2.png" alt="Concept Art" height="150">
</p>

Taro concept art:

<p>
    <img src="assets/concept_art/Taro.jpg" alt="Concept Art" height="200">
    <img src="assets/concept_art/Taro_Sticker.png" alt="Concept Art" height="200">
</p>

## Features

* Real-time audio-driven mouth animation using microphone input
* AI voice integration with conversation capabilities
* Random movement controller for autonomous behavior
* Simultaneous audio playback to two output devices with pitch effects
* Smooth servo motion with speed limiting and dead zones
* Wing flap control with cooldown timer
* Manual keyboard controls for wings and head movement
* Terminal-based UI for live status and debugging
* Modular C++ design with organized component architecture

## Project Structure

```
assets/
  concept_art/          Concept art and diagrams
  promotional_material/ Photos of Taro in the wild
  legacy/               Older C implementations
src/                   Main C++ source files
  main.cpp             Entry point and main control loop
  actuation/           Servo control components
    Mouth.h/.cpp       Audio-driven mouth servo controller
    Neck.h/.cpp        Neck servo controller
    Wings.h/.cpp       Wing servo controller with cooldown
  ai/                  AI integration components
    AIVoice.h/.cpp     AI voice conversation system
    taro_ai.py         Python AI backend
  audio/               Audio processing components
    Audio.h/.cpp       Audio capture and playback management
  control/             Control system components
    TaroUI.h/.cpp      Terminal UI and input handling
    RandomController.h/.cpp  Autonomous movement controller
  i2c/                 Hardware interface components
    PCA9685.h/.cpp     I2C PWM servo driver
test/                  Experimental and test code
Makefile               Build configuration
README.md              Project documentation
```

## Hardware Specs

* Raspberry Pi 5 with I2C enabled
* PCA9685 16-channel PWM servo driver
* Servos (tested with MG996R-style PWM ranges)
* USB microphone (audio input)
* Two USB audio output devices (speakers or sound cards)
* External power supply for servos

## Software

* Linux (Raspberry Pi OS)
* g++ with C++11
* ALSA development libraries
* I2C enabled

Dependencies:

```bash
sudo apt update
sudo apt install -y build-essential libasound2-dev i2c-tools
```

## Build

From the project root:

```bash
make
```

This produces the `tea_animatronic` executable.

## Run

```bash
./tea_animatronic
```

> **Note:** Run without `sudo` — the program accesses I2C and audio as the current user. If I2C permission is denied, add your user to the `i2c` group: `sudo usermod -aG i2c $USER`

## Audio Device Configuration

Device names are configured in `Mouth.h`:

```cpp
const char* DEVICE_INPUT   = "plughw:CARD=Device,DEV=0";
const char* DEVICE_OUTPUT1 = "plughw:CARD=UACDemoV10,DEV=0";
const char* DEVICE_OUTPUT2 = "plughw:CARD=Device_1,DEV=0";
```

Use `arecord -l` and `aplay -l` to confirm the correct card names for your system.

## Controls

While running, the program captures keyboard input in the terminal:

**Manual Controls:**
* `E` — Flap wings (2 second cooldown)
* `A` — Turn head left
* `D` — Turn head right
* `R` — Recenter head

**AI and Automation:**
* `I` — Trigger AI voice interaction (listen mode)
* `O` — Toggle AI auto mode (autonomous conversation + random movement)
* `X` — Toggle random movement controller only

**System:**
* `Q` — Quit program

**Random Controller Adjustment** (when active):
* `A` — Decrease activity level
* `D` — Increase activity level

The terminal UI updates in real time showing wing cooldown, mouth opening, head position, AI status, and random controller activity.

## src/actuation/ - Servo Control Components

Contains all servo motor control classes:

**Mouth.h/.cpp** - Audio-driven mouth servo controller
* ALSA audio capture at 48 kHz, mono, 1024-frame buffers
* Average amplitude analysis per frame
* Mapping amplitude to servo pulse width (850–1300 μs)
* Smoothing, speed limiting, and movement threshold filtering
* Rubber-band pitch effect via variable-speed resampling
* Simultaneous output to two playback devices
* Integration with AI system for voice synchronization

**Neck.h/.cpp** - Neck servo controller
* Smooth servo motion with configurable speed limits
* Left/right turn controls with angle boundaries
* Recenter function to return to neutral position
* Continuous update processing for smooth movement

**Wings.h/.cpp** - Wing servo controller with cooldown
* Configurable up/down angles per wing
* Blocking flap sequence with a hold delay
* 2-second cooldown enforced internally
* Integration with random controller for autonomous flapping

## src/ai/ - AI Integration Components

Contains all artificial intelligence and voice processing:

**AIVoice.h/.cpp** - AI voice conversation system
* Python-based AI backend (taro_ai.py) for natural language processing
* Real-time voice interaction with conversation memory
* Auto mode for autonomous conversation initiation
* Integration with mouth movement for realistic speech animation
* Configurable AI personality and response patterns

**taro_ai.py** - Python AI backend
* Local Whisper.cpp integration for speech recognition
* Llama.cpp server for language model processing
* Piper text-to-speech for voice synthesis
* Conversation memory and context management
* Audio device coordination and processing

## src/audio/ - Audio Processing Components

Contains all audio handling and device management:

**Audio.h/.cpp** - Audio capture and playback management
* Centralized audio device management
* Multi-threaded audio capture and playback
* Device configuration and error handling
* Integration with both mouth movement and AI voice output
* ALSA interface abstraction for audio hardware

## src/control/ - Control System Components

Contains all user interface and autonomous behavior control:

**TaroUI.h/.cpp** - Terminal UI and input handling
* Terminal-based UI for live status and debugging
* Keyboard input capture and command processing
* Real-time display of system state (wing cooldown, mouth opening, head position)
* AI status and random controller activity visualization

**RandomController.h/.cpp** - Autonomous movement controller
* Configurable activity levels for movement frequency
* Random head movements and wing flaps
* Smooth transitions between movements
* Can be used independently or with AI auto mode

## src/i2c/ - Hardware Interface Components

Contains all low-level hardware communication:

**PCA9685.h/.cpp** - I2C PWM servo driver
* Direct I2C register access via `/dev/i2c-1`
* Configurable PWM frequency (default 50 Hz for servos)
* Servo control via angle (`setServoAngle`) or pulse width (`setServoPulse`)
* This implementation avoids external libraries and communicates directly with the hardware

## AI Setup

The AI system uses local models for privacy and offline operation:

**Required Local Models:**
1. **Whisper.cpp** - Speech recognition
   ```bash
   git clone https://github.com/ggerganov/whisper.cpp
   cd whisper.cpp && make
   wget https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin
   ```

2. **Llama.cpp** - Language model server
   ```bash
   git clone https://github.com/ggerganov/llama.cpp
   cd llama.cpp && make
   # Download or create your model as models/taro.gguf
   ```

3. **Piper** - Text-to-speech
   ```bash
   # Install Piper and voice model
   wget https://github.com/rhasspy/piper/releases/latest/download/piper_linux_x86_64.tar.gz
   tar xzf piper_linux_x86_64.tar.gz
   wget https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/en/en_US/lessac/medium/en_US-lessac-medium.onnx
   ```

**Model paths are configured in `src/ai/taro_ai.py`:**
- Whisper model: `~/whisper.cpp/models/ggml-tiny.en.bin`
- Llama model: `~/models/taro.gguf`
- Piper voice: `~/piper-voices/en_US-lessac-medium.onnx`

## Notes and Warnings

* Do not power servos directly from the Raspberry Pi 5V rail
* Always share ground between the Pi and the servo power supply
* Incorrect I2C wiring or address selection will cause startup failure
* Audio latency depends on USB audio hardware quality
* AI functionality requires local models (Whisper, Llama.cpp, Piper) - no internet connection needed after setup
* If the program crashes without quitting cleanly, a stale process may hold audio devices open — use `fuser /dev/snd/*` to identify and `kill <pid>` to remove it
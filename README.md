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
* Simultaneous audio playback to two output devices with rubber-band pitch effect
* Smooth servo motion with speed limiting and dead zones
* Wing flap control with cooldown timer
* Manual keyboard controls for wings and head movement
* Terminal-based UI for live status and debugging
* Modular C++ design with reusable PCA9685, Mouth, Wings, and TaroUI components


## Project Structure

```
assets/
  concept_art/          Concept art and diagrams
  promotional_material/ Photos of Taro in the wild
include/                Public headers
  Mouth.h               Audio-driven mouth servo controller
  Neck.h                Neck servo controller
  PCA9685.h             PWM driver interface
  Wings.h               Wing servo controller with cooldown
  TaroUI.h              Terminal UI
legacy/                 Older C implementations
src/                    Main C++ source files
  main.cpp              Entry point
  AMouth.cpp            Audio processing thread + mouth control
  Neck.cpp              Neck servo logic
  PCA9685.cpp           I2C PWM implementation
  Wings.cpp             Wing flap logic
  TaroUI.cpp            Terminal rendering and input setup
test/                   Experimental and test code
Makefile                Build configuration
README.md               Project documentation
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

* `E` — Flap wings (2 second cooldown)
* `A` — Turn head left
* `D` — Turn head right
* `R` — Recenter head
* `Q` — Quit program

The terminal UI updates in real time showing wing cooldown, mouth opening, and head position.


## Mouth System

The `Mouth` class runs in its own thread and performs:

* ALSA audio capture at 48 kHz, mono, 1024-frame buffers
* Average amplitude analysis per frame
* Mapping amplitude to servo pulse width (850–1300 μs)
* Smoothing, speed limiting, and movement threshold filtering
* Rubber-band pitch effect via variable-speed resampling
* Simultaneous output to two playback devices

Here is a visualization of how audio is converted to mouth movement:

<img src="assets/concept_art/program_cycle.png" alt="Program Cycle" height="200">


## Wing System

The `Wings` class manages two wing servos with:

* Configurable up/down angles per wing
* Blocking flap sequence with a hold delay
* 2-second cooldown enforced internally — `flapWings()` returns `false` if called too early


## PCA9685 Servo Driver

The `PCA9685` class provides:

* Direct I2C register access via `/dev/i2c-1`
* Configurable PWM frequency (default 50 Hz for servos)
* Servo control via angle (`setServoAngle`) or pulse width (`setServoPulse`)

This implementation avoids external libraries and communicates directly with the hardware.


## Notes and Warnings

* Do not power servos directly from the Raspberry Pi 5V rail
* Always share ground between the Pi and the servo power supply
* Incorrect I2C wiring or address selection will cause startup failure
* Audio latency depends on USB audio hardware quality
* If the program crashes without quitting cleanly, a stale process may hold audio devices open — use `fuser /dev/snd/*` to identify and `kill <pid>` to remove it
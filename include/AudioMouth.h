#ifndef AUDIOMOUTH_H
#define AUDIOMOUTH_H

#include "PCA9685.h"
#include <alsa/asoundlib.h>
#include <atomic>
#include <thread>
#include <cstdint>

class AudioMouth {
private:
    PCA9685* pwm;
    uint8_t channel;
    std::thread audioThread;
    std::atomic<bool> running;

    /* ---------- Audio settings ---------- */
    static constexpr int SAMPLE_RATE = 48000;
    static constexpr int CHANNELS = 1;
    static constexpr int FRAMES = 1024;

    const char* DEVICE_INPUT  = "plughw:CARD=Device,DEV=0";
    const char* DEVICE_OUTPUT = "plughw:CARD=UACDemoV10,DEV=0";

    /* ---------- Servo settings ---------- */
    static constexpr uint16_t SERVO_MIN_PULSE = 1000;  // closed
    static constexpr uint16_t SERVO_MAX_PULSE = 1500;  // open
    static constexpr int SOUND_MIN_THRESHOLD = 100;
    static constexpr double SMOOTHING_FACTOR = 0.8;
    static constexpr int DEAD_ZONE = 300;
    static constexpr double SERVO_MOVEMENT_THRESHOLD = 5.0;
    static constexpr int SERVO_UPDATE_DELAY_US = 20000;
    static constexpr double MAX_SERVO_SPEED = 50.0;

    uint16_t prevServoPulse;

    void audioProcessingLoop();
    void moveServoBasedOnAmplitude(short* buffer, int size);

public:
    AudioMouth(PCA9685* pwmController, uint8_t servoChannel);
    ~AudioMouth();
    int getServoPulse() {return prevServoPulse;}

    void start();
    void stop();
};

#endif // AUDIOMOUTH_H

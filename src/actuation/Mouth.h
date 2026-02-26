#pragma once
#include "PCA9685.h"
#include "Audio.h"
#include <cstdint>
#include <algorithm>
#include <cmath>

#define MOUTH_SERVO_CHANNEL 3

class Mouth {
public:
    Mouth(PCA9685* pwmController);
    ~Mouth();
    void stop();
    int getServoPulse() const { return prevServoPulse; }

private:
    PCA9685* pwm;
    Audio audio;
    uint16_t prevServoPulse;

    static constexpr uint16_t SERVO_MIN_PULSE        = 850;
    static constexpr uint16_t SERVO_MAX_PULSE        = 1300;
    static constexpr int      SOUND_MIN_THRESHOLD    = 50;
    static constexpr double   SMOOTHING_FACTOR       = 0.8;
    static constexpr double   SERVO_MOVEMENT_THRESHOLD = 5.0;
    static constexpr int      SERVO_UPDATE_DELAY_US  = 20000;
    static constexpr double   MAX_SERVO_SPEED        = 50.0;

    void onAudioFrame(short* buffer, int frames);
};
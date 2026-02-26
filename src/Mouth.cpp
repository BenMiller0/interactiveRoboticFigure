#include "Mouth.h"
#include <cmath>
#include <unistd.h>

template <typename T>
static T clamp(T v, T lo, T hi) { return v < lo ? lo : v > hi ? hi : v; }

Mouth::Mouth(PCA9685* pwmController)
    : pwm(pwmController),
      audio([this](short* buf, int frames) { onAudioFrame(buf, frames); }),
      prevServoPulse(SERVO_MIN_PULSE) {
    pwm->setServoPulse(MOUTH_SERVO_CHANNEL, SERVO_MIN_PULSE);
}

Mouth::~Mouth() { stop(); }

void Mouth::stop() {
    audio.stop();
    pwm->setServoPulse(MOUTH_SERVO_CHANNEL, SERVO_MIN_PULSE);
}

void Mouth::onAudioFrame(short* buffer, int size) {
    double sum = 0.0;
    for (int i = 0; i < size; i++) sum += std::abs(buffer[i]);
    double avgAmplitude = sum / size;
    if (avgAmplitude < SOUND_MIN_THRESHOLD) return;

    double normalized = std::min((avgAmplitude / 32768.0) * 2.0, 1.0);
    uint16_t targetPulse = clamp<uint16_t>(
        SERVO_MIN_PULSE + normalized * (SERVO_MAX_PULSE - SERVO_MIN_PULSE),
        SERVO_MIN_PULSE, SERVO_MAX_PULSE);

    double delta    = clamp(static_cast<double>(targetPulse - prevServoPulse), -MAX_SERVO_SPEED, MAX_SERVO_SPEED);
    double smoothed = clamp(prevServoPulse + delta * SMOOTHING_FACTOR, (double)SERVO_MIN_PULSE, (double)SERVO_MAX_PULSE);

    if (std::fabs(smoothed - prevServoPulse) > SERVO_MOVEMENT_THRESHOLD) {
        prevServoPulse = static_cast<uint16_t>(smoothed);
        pwm->setServoPulse(MOUTH_SERVO_CHANNEL, prevServoPulse);
        usleep(SERVO_UPDATE_DELAY_US);
    }
}
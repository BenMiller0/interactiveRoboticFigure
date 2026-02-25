#include "Neck.h"
#include <cmath>
#include <algorithm>

Neck::Neck(PCA9685* pwmController)
    : pwm(pwmController),
      headCurrent(NECK_CENTER_PULSE),
      headTarget(NECK_CENTER_PULSE),
      recentering(false) {
    pwm->setServoPulse(NECK_CHANNEL, static_cast<uint16_t>(NECK_CENTER_PULSE));
}

void Neck::turnLeft() {
    if (recentering) return;
    headTarget = std::max(headTarget - NECK_STEP, NECK_MIN_PULSE);
}

void Neck::turnRight() {
    if (recentering) return;
    headTarget = std::min(headTarget + NECK_STEP, NECK_MAX_PULSE);
}

void Neck::recenter() {
    recentering = true;
    headTarget = NECK_CENTER_PULSE;
}

void Neck::update() {
    headCurrent += (headTarget - headCurrent) * NECK_SMOOTH;
    pwm->setServoPulse(NECK_CHANNEL, static_cast<uint16_t>(headCurrent));
    if (recentering && fabs(headCurrent - NECK_CENTER_PULSE) < 1.0) {
        headCurrent = NECK_CENTER_PULSE;
        recentering = false;
    }
}

uint16_t Neck::getServoPulse() const {
    return static_cast<uint16_t>(headCurrent);
}

bool Neck::isRecentering() const {
    return recentering;
}
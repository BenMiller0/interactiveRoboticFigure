#include "Wings.h"
#include "../i2c/PCA9685.h"
#include <unistd.h>

Wings::Wings(PCA9685& pwmController) : pwm(pwmController) {
    pwm.setServoAngle(WING_1_CHANNEL, WING_1_DOWN_ANGLE);
    pwm.setServoAngle(WING_2_CHANNEL, WING_2_DOWN_ANGLE);
    lastFlapTime = getCurrentTimeMs() - WING_FLAP_COOLDOWN_MS;
}

long long Wings::getCurrentTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}

bool Wings::isReady() const {
    return msSinceLastFlap() >= WING_FLAP_COOLDOWN_MS;
}

long long Wings::msSinceLastFlap() const {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    long long now = (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
    return now - lastFlapTime;
}

bool Wings::flapWings() {
    if (!isReady()) return false;

    lastFlapTime = getCurrentTimeMs();
    pwm.setServoAngle(WING_1_CHANNEL, WING_1_UP_ANGLE);
    pwm.setServoAngle(WING_2_CHANNEL, WING_2_UP_ANGLE);
    usleep(WING_UP_DELAY_US);
    pwm.setServoAngle(WING_1_CHANNEL, WING_1_DOWN_ANGLE);
    pwm.setServoAngle(WING_2_CHANNEL, WING_2_DOWN_ANGLE);
    return true;
}
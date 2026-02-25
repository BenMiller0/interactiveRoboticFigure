#pragma once
#include <PCA9685.h>
#include <sys/time.h>

#define WING_1_CHANNEL 0
#define WING_1_UP_ANGLE 160
#define WING_1_DOWN_ANGLE 60

#define WING_2_CHANNEL 1
#define WING_2_UP_ANGLE 80
#define WING_2_DOWN_ANGLE 180

#define WING_UP_DELAY_US 600000
#define WING_FLAP_COOLDOWN_MS 2000

class Wings {
private:
    PCA9685& pwm;
    long long lastFlapTime;

    long long getCurrentTimeMs();

public:
    Wings(PCA9685& pwmController);
    bool flapWings();       // returns true if flap was performed
    bool isReady() const;
    long long msSinceLastFlap() const;
};
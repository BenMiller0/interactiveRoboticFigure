#pragma once
#include "PCA9685.h"

#define NECK_CHANNEL 2
#define NECK_CENTER_PULSE 1500.0
#define NECK_MIN_PULSE 500.0
#define NECK_MAX_PULSE 2500.0
#define NECK_STEP 80.0
#define NECK_SMOOTH 0.2

class Neck {
private:
    PCA9685* pwm;
    double headCurrent;
    double headTarget;
    bool recentering;

public:
    Neck(PCA9685* pwmController);

    void setTarget(double pulse);
    void turnLeft();
    void turnRight();
    void recenter();
    void update();  // call once per loop iteration

    uint16_t getServoPulse() const;
    bool isRecentering() const;
};
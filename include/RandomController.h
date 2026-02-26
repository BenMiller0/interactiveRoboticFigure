#pragma once
#include "Neck.h"
#include "Wings.h"
#include <cstdlib>
#include <sys/time.h>

class RandomController {
public:
    RandomController(Neck& neck, Wings& wings);

    void setActive(bool active);
    bool isActive() const;
    void update();

    void increaseActivity();
    void decreaseActivity();
    int getActivityLevel() const;

private:
    Neck& neck;
    Wings& wings;
    bool active;
    int activityLevel;  // 1-10
    long long nextActionTime;

    long long getCurrentTimeMs();
    void doRandomAction();
    void scheduleNext();
};
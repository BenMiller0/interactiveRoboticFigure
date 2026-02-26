#include "RandomController.h"
#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#include <algorithm>

RandomController::RandomController(Neck& neck, Wings& wings)
    : neck(neck), wings(wings), active(false), activityLevel(5), nextActionTime(0) {
    std::srand(static_cast<unsigned>(time(nullptr)));
}

void RandomController::setActive(bool a) {
    active = a;
    nextActionTime = getCurrentTimeMs();
}

bool RandomController::isActive() const { return active; }
int RandomController::getActivityLevel() const { return activityLevel; }

void RandomController::increaseActivity() { activityLevel = std::min(activityLevel + 1, 10); }
void RandomController::decreaseActivity() { activityLevel = std::max(activityLevel - 1, 1);  }

long long RandomController::getCurrentTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}

void RandomController::scheduleNext() {
    // Level 1 → ~4000ms, Level 10 → ~300ms
    long long minMs  = 4000 - (activityLevel - 1) * 400;
    long long randMs = 1000 - (activityLevel - 1) * 80;
    if (randMs < 100) randMs = 100;
    nextActionTime = getCurrentTimeMs() + minMs + (std::rand() % randMs);
}

void RandomController::doRandomAction() {
    static const double positions[] = { 600.0, 900.0, 1500.0, 2100.0, 2400.0 };

    int action = std::rand() % 10;
    if (action < activityLevel / 3) {
        wings.flapWings();
    } else {
        neck.setTarget(positions[std::rand() % 5]);
    }
    scheduleNext();
}

void RandomController::update() {
    if (!active) return;
    if (getCurrentTimeMs() >= nextActionTime)
        doRandomAction();
}
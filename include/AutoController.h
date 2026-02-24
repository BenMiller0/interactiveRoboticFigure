#pragma once
#include "PCA9685.h"
#include <random>
#include <atomic>
#include <thread>
#include <chrono>

class AutoController {
public:
    AutoController(PCA9685* pwm);
    ~AutoController();

    void start(double startHeadPos = 1500.0);
    void stop();
    bool isRunning() const { return autoMode.load(); }
    double getCurrentHead() const { return headCurrent.load(); }

private:
    void loop();
    void flapWings();
    void moveHead(double target, double smoothing, int steps);
    void idlePause(int ms);

    PCA9685* pwm;
    std::atomic<bool> autoMode{false};
    std::atomic<double> headCurrent{1500.0};
    std::thread autoThread;

    std::mt19937 gen;

    // Head movement
    std::uniform_real_distribution<> headDist;      // pulse range
    std::uniform_real_distribution<> smoothDist;    // smoothing variation
    std::uniform_int_distribution<>  pauseDist;     // ms between moves

    // Wing flap timing
    std::uniform_int_distribution<>  flapChanceDist;   // roll per pause cycle
    std::uniform_int_distribution<>  flapCountDist;    // how many flaps in a burst
    std::uniform_int_distribution<>  flapGapDist;      // ms between flaps in burst

    static constexpr int   LOOP_STEP_US   = 10000;     // 10ms per tick
    static constexpr int   MOVE_STEPS     = 200;       // ticks spent moving
    static constexpr double HEAD_MIN      = 500.0;
    static constexpr double HEAD_MAX      = 2500.0;
    static constexpr long long FLAP_COOLDOWN_MS = 2000;

    long long lastFlapTimeMs = 0;
};
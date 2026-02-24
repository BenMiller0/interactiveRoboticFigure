#include "AutoController.h"
#include <unistd.h>
#include <cmath>
#include <sys/time.h>

// ── helpers ──────────────────────────────────────────────────────────────────

static long long nowMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (long long)tv.tv_sec * 1000 + (long long)tv.tv_usec / 1000;
}

// ── ctor/dtor ─────────────────────────────────────────────────────────────────

AutoController::AutoController(PCA9685* pwm)
    : pwm(pwm),
      gen(std::random_device{}()),
      headDist(HEAD_MIN, HEAD_MAX),
      smoothDist(0.05, 0.15),       // vary personality: sluggish ↔ snappy
      pauseDist(600, 3500),          // ms to sit still between moves
      flapChanceDist(1, 100),        // percentile roll
      flapCountDist(1, 3),           // 1–3 flaps per burst
      flapGapDist(300, 700)          // ms between flaps in a burst
{}

AutoController::~AutoController() {
    stop();
}

// ── public control ────────────────────────────────────────────────────────────

void AutoController::start(double startHeadPos) {
    if (autoMode.load()) return;
    headCurrent.store(startHeadPos);
    lastFlapTimeMs = nowMs() - FLAP_COOLDOWN_MS; // allow flap immediately
    autoMode.store(true);
    autoThread = std::thread(&AutoController::loop, this);
}

void AutoController::stop() {
    if (!autoMode.load()) return;
    autoMode.store(false);
    if (autoThread.joinable())
        autoThread.join();
}

// ── private helpers ───────────────────────────────────────────────────────────

void AutoController::flapWings() {
    long long now = nowMs();
    if (now - lastFlapTimeMs < FLAP_COOLDOWN_MS) return;
    lastFlapTimeMs = now;

    // Wing down
    pwm->setServoAngle(0, 240);
    pwm->setServoAngle(1, -60);
    usleep(500000);
    // Wing up
    pwm->setServoAngle(0, 60);
    pwm->setServoAngle(1, 120);
}

void AutoController::moveHead(double target, double smoothing, int steps) {
    double current = headCurrent.load();
    for (int i = 0; i < steps && autoMode.load(); i++) {
        current += (target - current) * smoothing;
        headCurrent.store(current);
        pwm->setServoPulse(2, static_cast<uint16_t>(current));
        usleep(LOOP_STEP_US);
    }
}

void AutoController::idlePause(int ms) {
    int ticks = ms / (LOOP_STEP_US / 1000);
    for (int i = 0; i < ticks && autoMode.load(); i++) {
        usleep(LOOP_STEP_US);
    }
}

// ── main loop ─────────────────────────────────────────────────────────────────

void AutoController::loop() {
    while (autoMode.load()) {

        // ── 1. Pick a personality for this move ──────────────────────────────
        //   Occasionally do a sharp snap to a new position (like a real bird),
        //   otherwise do a smooth gradual turn.
        bool snap = (flapChanceDist(gen) <= 20); // 20% chance of a snappy move
        double smoothing = snap ? 0.25 : smoothDist(gen);
        int    steps     = snap ? 60   : MOVE_STEPS;

        // ── 2. Choose a new head target ───────────────────────────────────────
        //   Bias toward center somewhat so it doesn't always stare sideways.
        double raw    = headDist(gen);
        double biased = raw * 0.75 + 1500.0 * 0.25;
        double target = std::max(HEAD_MIN, std::min(HEAD_MAX, biased));

        moveHead(target, smoothing, steps);

        // ── 3. Pause at this position ─────────────────────────────────────────
        int pauseMs = pauseDist(gen);

        // ── 4. Random wing flap decision ──────────────────────────────────────
        //   Roll the dice: 25% chance of a flap burst during this pause.
        if (flapChanceDist(gen) <= 25) {
            // Wait a little before flapping (looks more natural)
            int preDelayMs = std::uniform_int_distribution<>(100, pauseMs / 2)(gen);
            idlePause(preDelayMs);

            if (autoMode.load()) {
                int burstCount = flapCountDist(gen);
                for (int f = 0; f < burstCount && autoMode.load(); f++) {
                    flapWings();
                    if (f < burstCount - 1) {
                        // Gap between flaps in a burst
                        idlePause(flapGapDist(gen));
                    }
                }
                // Spend remainder of pause idle
                int remaining = pauseMs - preDelayMs - (burstCount * 500);
                if (remaining > 0) idlePause(remaining);
            }
        } else {
            idlePause(pauseMs);
        }

        // ── 5. Occasional double-take: quick small jerk then back ─────────────
        if (autoMode.load() && flapChanceDist(gen) <= 10) {
            double jerkTarget = headCurrent.load() + (flapChanceDist(gen) > 50 ? 200.0 : -200.0);
            jerkTarget = std::max(HEAD_MIN, std::min(HEAD_MAX, jerkTarget));
            moveHead(jerkTarget, 0.3, 30);
            moveHead(target,     0.15, 60);
        }
    }
}
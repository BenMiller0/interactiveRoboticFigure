#include "i2c/PCA9685.h"
#include "actuation/Mouth.h"
#include "actuation/Wings.h"
#include "actuation/Neck.h"
#include "control/TaroUI.h"
#include "control/RandomController.h"
#include "ai/AIVoice.h"
#include <unistd.h>

int main() {
    PCA9685 pwm;
    Neck neck(&pwm);
    Mouth mouth(&pwm);
    Wings wings(pwm);
    TaroUI ui;
    RandomController random(neck, wings);
    AIVoice ai;
    ai.start();

    char ch;
    bool running = true;
    bool aiAutoMode = false;
    AIState prevAIState = AIState::IDLE;

    while (running) {
        while (read(STDIN_FILENO, &ch, 1) > 0) {
            if      (ch == 'q' || ch == 'Q') { running = false; }
            else if (ch == 'x' || ch == 'X') { random.setActive(!random.isActive()); }
            else if (ch == 'i' || ch == 'I') {
                if (!aiAutoMode && ai.getState() == AIState::READY) {
                    mouth.pause();
                    ai.triggerListen();
                }
            }
            else if (ch == 'o' || ch == 'O') {
                if (!aiAutoMode) {
                    aiAutoMode = true;
                    random.setActive(true);
                    mouth.pause();
                    ai.triggerAutoOn();
                } else {
                    aiAutoMode = false;
                    random.setActive(false);
                    mouth.resume();
                    ai.triggerAutoOff();
                }
            }
            else if (random.isActive()) {
                if      (ch == 'a' || ch == 'A') { random.decreaseActivity(); }
                else if (ch == 'd' || ch == 'D') { random.increaseActivity(); }
            } else {
                if      (ch == 'e' || ch == 'E') { wings.flapWings(); }
                else if (ch == 'a' || ch == 'A') { neck.turnLeft(); }
                else if (ch == 'd' || ch == 'D') { neck.turnRight(); }
                else if (ch == 'r' || ch == 'R') { neck.recenter(); }
            }
        }

        // Handle AI state transitions
        AIState curAIState = ai.getState();

        if (curAIState == AIState::SPEAKING) {
            // Drive mouth servo from TTS audio amplitude
            mouth.setServoPulse(ai.getSpeakingAmplitude());
        } else if (prevAIState == AIState::SPEAKING && curAIState == AIState::READY) {
            if (!aiAutoMode) {
                mouth.resume();
            }
        }

        prevAIState = curAIState;

        neck.update();
        random.update();

        if (random.isActive()) {
            ui.update(neck.getServoPulse(), mouth.getServoPulse(), wings,
                      random.getActivityLevel(), curAIState);
        } else {
            ui.update(neck.getServoPulse(), mouth.getServoPulse(), wings, curAIState);
        }

        usleep(10000);
    }

    ui.shutdown();
    ai.stop();
    mouth.stop();
    return 0;
}
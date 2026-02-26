#include "PCA9685.h"
#include "Mouth.h"
#include "Wings.h"
#include "TaroUI.h"
#include "Neck.h"
#include "RandomController.h"
#include <unistd.h>

int main() {
    PCA9685 pwm;
    Neck neck(&pwm);
    Mouth mouth(&pwm);
    Wings wings(pwm);
    TaroUI ui;
    RandomController random(neck, wings);

    char ch;
    bool running = true;

    while (running) {
        while (read(STDIN_FILENO, &ch, 1) > 0) {
            if      (ch == 'x' || ch == 'X') { random.setActive(!random.isActive()); }
            else if (ch == 'q' || ch == 'Q') { running = false; }
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

        neck.update();
        random.update();

        if (random.isActive()) {
            ui.update(neck.getServoPulse(), mouth.getServoPulse(), wings, random.getActivityLevel());
        } else {
            ui.update(neck.getServoPulse(), mouth.getServoPulse(), wings);
        }

        usleep(10000);
    }

    ui.shutdown();
    mouth.stop();
    return 0;
}
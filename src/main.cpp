#include "PCA9685.h"
#include "Mouth.h"
#include "Wings.h"
#include "TaroUI.h"
#include "Neck.h"
#include <unistd.h>

int main() {
    PCA9685 pwm;
    Neck neck(&pwm);
    Mouth mouth(&pwm);
    Wings wings(pwm);
    TaroUI ui;

    char ch;
    bool running = true;

    while (running) {
        while (read(STDIN_FILENO, &ch, 1) > 0) {
            if      (ch == 'e' || ch == 'E') { wings.flapWings(); }
            else if (ch == 'a' || ch == 'A') { neck.turnLeft(); }
            else if (ch == 'd' || ch == 'D') { neck.turnRight(); }
            else if (ch == 'r' || ch == 'R') { neck.recenter(); }
            else if (ch == 'q' || ch == 'Q') { running = false; }
        }
        neck.update();
        ui.update(neck.getServoPulse(), mouth.getServoPulse(), wings);
        usleep(10000);
    }

    ui.shutdown();
    mouth.stop();
    return 0;
}
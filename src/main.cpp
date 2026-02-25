#include "PCA9685.h"
#include "AudioMouth.h"
#include "Wings.h"
#include "TaroUI.h"
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>
#include <cmath>

// ... (setNonBlockingInput and getCurrentTimeMs stay here, or move to a util if desired)

int main() {
    PCA9685 pwm;
    pwm.setServoPulse(2, 1500);

    AudioMouth mouth(&pwm);
    mouth.start();

    Wings wings(pwm);
    TaroUI ui;
    ui.init();

    char ch;
    bool running = true;
    bool recentering = false;

    double headCurrent = 1500.0;
    double headTarget  = 1500.0;
    const double HEAD_STEP   = 80.0;
    const double HEAD_SMOOTH = 0.2;

    ui.draw(static_cast<uint16_t>(headCurrent), mouth.getServoPulse(), wings);

    while (running) {
        while (read(STDIN_FILENO, &ch, 1) > 0) {
            if      (ch == 'e' || ch == 'E') { wings.flapWings(); }
            else if ((ch == 'a' || ch == 'A') && !recentering) { headTarget = std::max(headTarget - HEAD_STEP, 500.0); }
            else if ((ch == 'd' || ch == 'D') && !recentering) { headTarget = std::min(headTarget + HEAD_STEP, 2500.0); }
            else if  (ch == 'r' || ch == 'R') { recentering = true; headTarget = 1500.0; }
            else if  (ch == 'q' || ch == 'Q') { running = false; }
        }

        headCurrent += (headTarget - headCurrent) * HEAD_SMOOTH;
        pwm.setServoPulse(2, static_cast<uint16_t>(headCurrent));
        if (recentering && fabs(headCurrent - 1500.0) < 1.0) {
            headCurrent = 1500.0;
            recentering = false;
        }

        if (ui.needsDraw()) {
        ui.draw(static_cast<uint16_t>(headCurrent), mouth.getServoPulse(), wings);
    }

        usleep(10000);
    }

    ui.shutdown();
    mouth.stop();
    return 0;
}
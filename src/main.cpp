#include "PCA9685.h"
#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>

// Function to set terminal to non-blocking mode
void setNonBlockingInput(bool enable) {
    static struct termios oldt, newt;
    
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, 0);
    }
}

// Get current time in milliseconds
long long getCurrentTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}

// Single wing flap
void flapWings(PCA9685& pwm) {
    std::cout << "Flap!" << std::endl;
    
    const float UP_ANGLE = 240;
    const float DOWN_ANGLE = 60;
    const int MOVE_DELAY = 500000; // 500ms
    
    // Wings up - channel 0 goes UP, channel 1 goes in opposite direction
    pwm.setServoAngle(0, UP_ANGLE);
    pwm.setServoAngle(1, 180 - UP_ANGLE);
    usleep(MOVE_DELAY);
    
    // Wings down - channel 0 goes DOWN, channel 1 goes in opposite direction
    pwm.setServoAngle(0, DOWN_ANGLE);
    pwm.setServoAngle(1, 180 - DOWN_ANGLE);
}

int main() {
    std::cout << "Tea Animatronic Control" << std::endl;
    std::cout << "Press 'e' to flap wings" << std::endl;
    std::cout << "Hold 'a' to rotate head left, hold 'd' to rotate head right" << std::endl;
    std::cout << "Press 'q' to quit" << std::endl;
    
    PCA9685 pwm;
    
    // Set servos to initial positions
    pwm.setServoAngle(0, 60);      // Wing down
    pwm.setServoAngle(1, 120);     // Wing down (inverted)
    pwm.setServoPulse(2, 1500);    // Head center
    
    setNonBlockingInput(true);
    
    char ch;
    bool running = true;
    uint16_t headPulse = 1500;
    long long lastFlapTime = 0;
    const long long FLAP_COOLDOWN = 2000; // 2 seconds in milliseconds
    
    const int HEAD_SPEED = 40;
    const int UPDATE_DELAY = 10000;
    const uint16_t MIN_PULSE = 500;
    const uint16_t MAX_PULSE = 2500;
    
    while (running) {
        // Read all available characters
        while (read(STDIN_FILENO, &ch, 1) > 0) {
            if (ch == 'e' || ch == 'E') {
                long long currentTime = getCurrentTimeMs();
                if (currentTime - lastFlapTime >= FLAP_COOLDOWN) {
                    flapWings(pwm);
                    lastFlapTime = currentTime;
                } else {
                    long long timeLeft = (FLAP_COOLDOWN - (currentTime - lastFlapTime)) / 1000;
                    std::cout << "Cooldown active. Wait " << timeLeft << " more second(s)." << std::endl;
                }
            } else if (ch == 'a' || ch == 'A') {
                headPulse -= HEAD_SPEED;
                if (headPulse < MIN_PULSE) headPulse = MIN_PULSE;
                pwm.setServoPulse(2, headPulse);
            } else if (ch == 'd' || ch == 'D') {
                headPulse += HEAD_SPEED;
                if (headPulse > MAX_PULSE) headPulse = MAX_PULSE;
                pwm.setServoPulse(2, headPulse);
            } else if (ch == 'q' || ch == 'Q') {
                std::cout << "\nQuitting..." << std::endl;
                running = false;
            }
        }
        
        usleep(UPDATE_DELAY);
    }
    
    setNonBlockingInput(false);
    
    return 0;
}

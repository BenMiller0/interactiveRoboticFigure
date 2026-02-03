#include "PCA9685.h"
#include "AudioMouth.h"
#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sstream>
#include <cmath>

#define CLEAR "\033[2J\033[H"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define CYAN "\033[36m"
#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define MAGENTA "\033[35m"
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define DIM "\033[2m"

// --- Non-blocking input setup ---
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

// --- Time helper ---
long long getCurrentTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}

// --- Wings ---
void flapWings(PCA9685& pwm) {
    pwm.setServoAngle(0, 240);
    pwm.setServoAngle(1, -60);
    usleep(500000);
    pwm.setServoAngle(0, 60);
    pwm.setServoAngle(1, 120);
}

// --- Clamp helper (C++11 compatible) ---
template <typename T>
T clamp(T val, T minVal, T maxVal) {
    return (val < minVal) ? minVal : (val > maxVal) ? maxVal : val;
}

// --- Visual helpers ---
std::string getBar(uint16_t pulse, uint16_t min, uint16_t max, int width = 20) {
    int pos = ((pulse - min) * width) / (max - min);
    pos = clamp(pos, 0, width);
    std::string bar = "";
    for (int i = 0; i < width; i++) {
        if (i == pos) bar += "█";
        else if (i == width/2) bar += "┼";
        else bar += "─";
    }
    return bar;
}

std::string getMouthVisual(uint16_t pulse) {
    int opening = ((pulse - 1000) * 5) / 1000;
    opening = clamp(opening, 0, 5);
    switch(opening) {
        case 0: return "  ─────  ";
        case 1: return " ╱     ╲ ";
        case 2: return "╱       ╲";
        case 3: return "│       │";
        case 4: return "│ ︵   ︵ │";
        case 5: return "╲ ◡   ◡ ╱";
        default: return "  ─────  ";
    }
}

void draw(uint16_t head, uint16_t mouth, long long lastFlap, std::ostringstream& buf) {
    buf.str("");
    buf << "\033[H";
    
    buf << BOLD CYAN "╔════════════════════════════╗\n";
    buf << "║   🦅 Taro Controller 🦅    ║\n";
    buf << "╚════════════════════════════╝\n" RESET;
    
    long long ms = getCurrentTimeMs() - lastFlap;
    int pct = (ms * 100) / 2000;
    pct = std::min(pct, 100);
    
    buf << "\n " BOLD "WINGS" RESET "  ";
    if (pct >= 100) {
        buf << GREEN "✓ Ready      " RESET;
    } else {
        int bars = pct / 10;
        buf << RED "[";
        for (int i = 0; i < 10; i++)
            buf << (i < bars ? "█" : "░");
        buf << "] " << pct << "%" RESET;
    }
    buf << "             \n";
    
    buf << "\n " BOLD "MOUTH" RESET "  " MAGENTA << getMouthVisual(mouth) << RESET;
    buf << "  " << mouth << "μs\n";
    
    buf << "\n " BOLD "HEAD" RESET "   " << getBar(head, 500, 2500, 22) << "\n";
    buf << "        " DIM "←left" RESET "      " CYAN << head << "μs" RESET "      " DIM "right→" RESET "\n";
    
    buf << "\n " YELLOW "E" RESET "·Flap  " YELLOW "A" RESET "/" YELLOW "D" RESET "·Turn  " YELLOW "R" RESET "·Recenter  " YELLOW "Q" RESET "·Quit\n";
    
    std::cout << buf.str() << std::flush;
}

int main() {
    std::cout << CLEAR << HIDE_CURSOR;
    
    PCA9685 pwm;
    pwm.setServoAngle(0, 60);
    pwm.setServoAngle(1, 120);
    pwm.setServoPulse(2, 1500);
    
    AudioMouth mouth(&pwm, 3);
    mouth.start();
    
    setNonBlockingInput(true);
    
    char ch;
    bool running = true;
    bool recentering = false;
    long long lastFlapTime = getCurrentTimeMs() - 2000;
    long long lastDraw = 0;
    
    // --- Smooth head variables ---
    double headCurrent = 1500.0;
    double headTarget = 1500.0;
    const double HEAD_STEP = 80.0;    // speed increment for keys
    const double HEAD_SMOOTH = 0.2;   // fraction per loop
    
    std::ostringstream buf;
    draw(static_cast<uint16_t>(headCurrent), mouth.getServoPulse(), lastFlapTime, buf);
    
    while (running) {
        // --- Input handling ---
        while (read(STDIN_FILENO, &ch, 1) > 0) {
            if (ch == 'e' || ch == 'E') {
                long long now = getCurrentTimeMs();
                if (now - lastFlapTime >= 2000) {
                    flapWings(pwm);
                    lastFlapTime = now;
                }
            } else if ((ch == 'a' || ch == 'A') && !recentering) {
                headTarget -= HEAD_STEP;
                if (headTarget < 500) headTarget = 500;
            } else if ((ch == 'd' || ch == 'D') && !recentering) {
                headTarget += HEAD_STEP;
                if (headTarget > 2500) headTarget = 2500;
            } else if (ch == 'r' || ch == 'R') { // recenter head
                recentering = true;
                headTarget = 1500.0;
            } else if (ch == 'q' || ch == 'Q') { // quit
                running = false;
            }
        }
        
        // --- Smooth head update ---
        headCurrent += (headTarget - headCurrent) * HEAD_SMOOTH;
        pwm.setServoPulse(2, static_cast<uint16_t>(headCurrent));
        if (recentering && fabs(headCurrent - 1500.0) < 1.0) {
            headCurrent = 1500.0;
            recentering = false;
        }
        
        // --- Draw ---
        long long now = getCurrentTimeMs();
        if (now - lastDraw >= 100) {
            draw(static_cast<uint16_t>(headCurrent), mouth.getServoPulse(), lastFlapTime, buf);
            lastDraw = now;
        }
        
        usleep(10000);
    }
    
    std::cout << SHOW_CURSOR << CLEAR;
    mouth.stop();
    setNonBlockingInput(false);
    
    return 0;
}

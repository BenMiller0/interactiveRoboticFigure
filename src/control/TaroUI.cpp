#include "TaroUI.h"
#include <iostream>
#include <algorithm>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

static long long getCurrentTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}

static void setNonBlockingInput(bool enable) {
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

template <typename T>
static T clamp(T val, T minVal, T maxVal) {
    return (val < minVal) ? minVal : (val > maxVal) ? maxVal : val;
}

TaroUI::TaroUI() : lastDraw(0) {
    setNonBlockingInput(true);
    std::cout << CLEAR << HIDE_CURSOR << std::flush;
}

void TaroUI::shutdown() {
    std::cout << SHOW_CURSOR << CLEAR << std::flush;
    setNonBlockingInput(false);
}

bool TaroUI::needsDraw() {
    long long now = getCurrentTimeMs();
    if (now - lastDraw >= 100) {
        lastDraw = now;
        return true;
    }
    return false;
}

std::string TaroUI::getBar(uint16_t pulse, uint16_t min, uint16_t max, int width) {
    int pos = ((pulse - min) * width) / (max - min);
    pos = clamp(pos, 0, width);
    std::string bar;
    for (int i = 0; i < width; i++) {
        if (i == pos)            bar += "█";
        else if (i == width / 2) bar += "┼";
        else                     bar += "─";
    }
    return bar;
}

std::string TaroUI::getMouthVisual(uint16_t pulse) {
    int opening = ((pulse - 1000) * 5) / 1000;
    opening = clamp(opening, 0, 5);
    switch (opening) {
        case 0: return "  ─────  ";
        case 1: return " ╱     ╲ ";
        case 2: return "╱       ╲";
        case 3: return "│       │";
        case 4: return "│ ︵   ︵ │";
        case 5: return "╲ ◡   ◡ ╱";
        default: return "  ─────  ";
    }
}

void TaroUI::drawBase(uint16_t head, uint16_t mouth, const Wings& wings) {
    buf.str("");
    buf << "\033[H";

    buf << BOLD CYAN "╔════════════════════════════╗\n";
    buf <<           "║      Taro Controller       ║\n";
    buf <<           "╚════════════════════════════╝\n" RESET;

    int pct = (int)((wings.msSinceLastFlap() * 100LL) / WING_FLAP_COOLDOWN_MS);
    pct = std::min(pct, 100);
    pct = std::max(pct, 0);

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

    buf << "\n " BOLD "HEAD" RESET "   " << getBar(head, 500, 2500) << "\n";
    buf << "        " DIM "←left" RESET "      " CYAN << head << "μs" RESET "      " DIM "right→" RESET "\n";
}

// Normal mode
void TaroUI::update(uint16_t head, uint16_t mouth, const Wings& wings) {
    if (!needsDraw()) return;
    drawBase(head, mouth, wings);

    buf << "\n " YELLOW "E" RESET "·Flap  "
        << YELLOW "A" RESET "/" YELLOW "D" RESET "·Turn  "
        << YELLOW "R" RESET "·Recenter  "
        << YELLOW "X" RESET "·Auto Mode  "
        << YELLOW "Q" RESET "·Quit   \n";

    std::cout << buf.str() << std::flush;
}

// Auto mode
void TaroUI::update(uint16_t head, uint16_t mouth, const Wings& wings, int activityLevel) {
    if (!needsDraw()) return;
    drawBase(head, mouth, wings);

    buf << "\n " YELLOW "X" RESET "·Exit Auto  "
        << YELLOW "A" RESET "·Less  "
        << YELLOW "D" RESET "·More  "
        << "  Activity: ";
    for (int i = 1; i <= 10; i++)
        buf << (i <= activityLevel ? GREEN "█" RESET : DIM "░" RESET);
    buf << " " CYAN << activityLevel << "/10" RESET "   \n";

    std::cout << buf.str() << std::flush;
}
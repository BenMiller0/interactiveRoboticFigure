#pragma once
#include <sstream>
#include <string>
#include <cstdint>
#include "Wings.h"

#define CLEAR       "\033[2J\033[H"
#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"
#define CYAN        "\033[36m"
#define GREEN       "\033[32m"
#define RED         "\033[31m"
#define YELLOW      "\033[33m"
#define MAGENTA     "\033[35m"
#define RESET       "\033[0m"
#define BOLD        "\033[1m"
#define DIM         "\033[2m"

class TaroUI {
public:
    TaroUI();
    void init();
    void shutdown();
    bool needsDraw();
    void draw(uint16_t head, uint16_t mouth, const Wings& wings);

private:
    long long lastDraw;
    std::ostringstream buf;

    std::string getBar(uint16_t pulse, uint16_t min, uint16_t max, int width = 22);
    std::string getMouthVisual(uint16_t pulse);
};
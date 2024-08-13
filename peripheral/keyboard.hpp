#pragma once

#include <SDL.h>
#include <vector>
#include "../mcu/mcu.hpp"
#include "../config/config.hpp"

class keyboard {
private:
    mcu *mcu;
    struct config *config;
public:
    keyboard(class mcu *mcu, struct config *config, int w, int h);
    void process_event(const SDL_Event *e);
    void render(SDL_Renderer *renderer);
private:
    std::vector<uint8_t> held_buttons;
    int w;
    int h;
};

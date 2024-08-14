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
    void tick();
private:
    std::vector<uint8_t> held_buttons;
    bool mouse_held;
    uint8_t held_button_mouse;
    int w;
    int h;
    void _tick(uint8_t *ki, uint8_t kimask, uint8_t ko, uint8_t k);
};

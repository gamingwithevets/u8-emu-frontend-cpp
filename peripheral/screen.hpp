#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <cstdint>
#include "../config/config.hpp"
#include "../mcu/mcu.hpp"

struct statusbit {
    uint8_t idx;
    uint8_t bit;
};

class screen {
private:
    mcu *mcu;
public:
    struct config *config;
    SDL_Surface *display;
    int width;
    int height;
    int bytes_per_row;
    int bytes_per_row_real;
    bool cw_2bpp;
    bool cw_2bpp_toggle;
    uint8_t cw_screen_data[192][64]{};
    screen(class mcu *mcu, struct config *config);
    ~screen();
    void render(SDL_Renderer *renderer);
private:
    bool use_status_bar_image;
    SDL_Surface *status_bar;
    int sbar_hi;
    std::vector<statusbit> status_bar_bits;
};

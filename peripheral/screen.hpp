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
    uint8_t cw_screen_data[192*64]{};
    screen(class mcu *mcu);
    ~screen();
    SDL_Surface *get_surface(uint32_t background = 0);
    void render(SDL_Renderer *renderer);
    void save(const char *fname);
#if defined _WIN32 || defined __CYGWIN__
    bool render_clipboard();
#endif
    void reset();
private:
    bool use_status_bar_image;
    SDL_Surface *status_bar;
    int sbar_hi;
    std::vector<statusbit> status_bar_bits;
};

uint8_t draw_screen_cw(mcu *mcu, uint16_t addr, uint8_t val);

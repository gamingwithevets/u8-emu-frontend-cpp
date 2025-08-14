/*
    u8-emu-frontend-cpp
    Copyright (C) 2024-2025  GamingWithEvets Inc.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <cstdint>
#include "../config/config.hpp"
#include "../mcu/mcu.hpp"

struct statusbit {
    uint8_t idx;
    uint8_t bit;
};

class Screen {
private:
    MCU *mcu;
public:
    struct Config *config;
    SDL_Surface *display;
    int width;
    int height;
    int bytes_per_row;
    int bytes_per_row_real;
    bool cw_2bpp;
    bool cw_2bpp_toggle;
    uint8_t cw_screen_data[192*64]{};
    Screen(class MCU *mcu);
    ~Screen();
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

uint8_t draw_screen_cw(MCU *mcu, uint16_t addr, uint8_t val);

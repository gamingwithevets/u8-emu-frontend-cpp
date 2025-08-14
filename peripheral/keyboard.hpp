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

#include <cstdint>
#include <optional>
#include <SDL3/SDL.h>
#include <vector>
#include "../mcu/mcu.hpp"
#include "../config/config.hpp"

enum es_stop_type : uint8_t {
    ES_STOP_GETKEY = 1,
    ES_STOP_ACBREAK = 2,
    ES_STOP_DOOFF = 3,
    ES_STOP_DDOUT = 4,
    ES_STOP_QRCODE_IN = 5,
    ES_STOP_QRCODE_OUT = 6,
    ES_STOP_QRCODE_IN3 = 7,
    ES_STOP_ACBREAK2 = 8
};

struct es_stop_info {
    es_stop_type *ES_STOPTYPEADR;
    uint8_t *ES_KIADR;
    uint8_t *ES_KOADR;
    char *ES_QR_DATATOP_ADR;
    bool qr_active;
    char qr_url[200];
};

class Keyboard {
private:
    MCU *mcu;
    struct Config *config;
public:
    struct es_stop_info emu_kb;
    bool enable_keypress;
    Keyboard(class MCU *mcu, int w, int h);
    void process_event(SDL_Renderer *renderer, const SDL_Event *e);
    void render(SDL_Renderer *renderer);
    void tick();
    void tick_emu();
    std::optional<uint8_t> get_button();
private:
    std::vector<uint8_t> held_buttons;
    bool mouse_held;
    uint8_t held_button_mouse;
    int w;
    int h;
    void _tick(bool *reset, uint8_t *ki, uint8_t kimask, uint8_t ko, uint8_t k);
};

#pragma once

#include <cstdint>
#include <SDL.h>
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
    char *qr_url;
};

class keyboard {
private:
    mcu *mcu;
    struct config *config;
public:
    struct es_stop_info emu_kb;
    keyboard(class mcu *mcu, int w, int h);
    void process_event(const SDL_Event *e);
    void render(SDL_Renderer *renderer);
    void tick();
    void tick_emu();
private:
    std::vector<uint8_t> held_buttons;
    bool mouse_held;
    uint8_t held_button_mouse;
    bool enable_keypress;
    int w;
    int h;
    void _tick(bool *reset, uint8_t *ki, uint8_t kimask, uint8_t ko, uint8_t k);
};

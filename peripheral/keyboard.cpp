#include <SDL.h>
#include <cstdio>
#include <algorithm>
#include <map>
#include <optional>

#include "../mcu/mcu.hpp"
#include "../config/config.hpp"
#include "keyboard.hpp"

keyboard::keyboard(class mcu *mcu, int w, int h) {
    this->mcu = mcu;
    this->config = mcu->config;

    this->w = w;
    this->h = h;

    this->mouse_held = false;
    this->enable_keypress = true;

    // KI/KO
    // TODO: Actually implement KO filter
    if (this->config->hardware_id == HW_TI_MATHPRINT) {}
    else if (this->config->hardware_id != HW_ES) {
        register_sfr(0x41, 4, &default_write<0xff>);
        register_sfr(0x45, 1, &default_write<4>);
        register_sfr(0x46, 1, &default_write<0xff>);
        register_sfr(0x47, 1, &default_write<0x83>);

        // Port 0
        register_sfr(0x48, 5, &default_write<0xff>);
    } else if (this->config->is_5800p) {
        register_sfr(0x41, 5, &default_write<0xff>);
        register_sfr(0x47, 4, &default_write<0xff>);
        this->mcu->sfr[0x46] = 4;
    } else register_sfr(0x41, 10, &default_write<0xff>);

    if (!this->config->real_hardware) {
        switch (this->config->hardware_id) {
        case HW_SOLAR_II:
            this->emu_kb.ES_STOPTYPEADR = (es_stop_type *)&this->mcu->ram[0x800];
            this->emu_kb.ES_KIADR = &this->mcu->ram[0x801];
            this->emu_kb.ES_KOADR = &this->mcu->ram[0x802];
            break;
        case HW_CLASSWIZ_EX:
        case HW_CLASSWIZ_CW:
            if (this->config->sample) {
                this->emu_kb.ES_STOPTYPEADR = (es_stop_type *)&this->mcu->ram2[0x8e07];
                this->emu_kb.ES_KIADR = &this->mcu->ram2[0x8e05];
                this->emu_kb.ES_KOADR = &this->mcu->ram2[0x8e08];
            } else {
                this->emu_kb.ES_STOPTYPEADR = (es_stop_type *)&this->mcu->ram2[0x8e00];
                this->emu_kb.ES_KIADR = &this->mcu->ram2[0x8e01];
                this->emu_kb.ES_KOADR = &this->mcu->ram2[0x8e02];
            }
            this->emu_kb.ES_QR_DATATOP_ADR = (char *)&this->mcu->ram2[0xa800];
            break;
        case HW_TI_MATHPRINT: break;
        default:
            this->emu_kb.ES_STOPTYPEADR = (es_stop_type *)&this->mcu->ram[0xe00];
            this->emu_kb.ES_KIADR = &this->mcu->ram[0xe01];
            this->emu_kb.ES_KOADR = &this->mcu->ram[0xe02];
        }
    }
}

void keyboard::process_event(const SDL_Event *e) {
    SDL_Keycode key;
    switch (e->type) {
    case SDL_KEYDOWN:
        key = e->key.keysym.sym;
        for (const auto &[k, v] : this->config->keymap) {
            if (std::find(v.keys.begin(), v.keys.end(), key) != v.keys.end()) {
                this->held_buttons.push_back(k);
                break;
            }
        }
        break;
    case SDL_KEYUP:
        key = e->key.keysym.sym;
        for (const auto &[k, v] : this->config->keymap) {
            if (std::find(v.keys.begin(), v.keys.end(), key) != v.keys.end()) {
                std::vector<uint8_t>::iterator pos = std::find(this->held_buttons.begin(), this->held_buttons.end(), k);
                if (pos != this->held_buttons.end()) {
                    this->held_buttons.erase(pos);
                    break;
                }
            }
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        SDL_Point mousepos;
        mousepos.x = e->motion.x;
        mousepos.y = e->motion.y;
        for (const auto &[k, v] : this->config->keymap) {
            if (SDL_PointInRect(&mousepos, &v.rect)) {
                this->mouse_held = true;
                this->held_button_mouse = k;
            }
        }
        break;
    case SDL_MOUSEBUTTONUP:
        this->mouse_held = false;
        break;
    }

    const uint8_t* keystates = SDL_GetKeyboardState(NULL);
    for (int i = 0; i < SDL_NUM_SCANCODES; ++i) {
        if (keystates[i] != 0) return;
    }
    this->held_buttons.clear();
    this->enable_keypress = true;

}

void keyboard::render(SDL_Renderer *renderer) {
    SDL_Surface *tmp = SDL_CreateRGBSurface(0, this->w, this->h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    for (const auto &k : this->held_buttons) SDL_FillRect(tmp, &this->config->keymap[k].rect, 0xAA000000);
    if (this->mouse_held) SDL_FillRect(tmp, &this->config->keymap[this->held_button_mouse].rect, 0xAA000000);

    SDL_Texture* tmp2 = SDL_CreateTextureFromSurface(renderer, tmp);
    SDL_FreeSurface(tmp);

    SDL_Rect dest {0, 0, this->w, this->h};
    SDL_RenderCopy(renderer, tmp2, NULL, &dest);

    SDL_DestroyTexture(tmp2);
}

void keyboard::tick() {
    uint8_t ki = 0;
    uint8_t kimask = this->mcu->sfr[0x42];
    uint8_t ko = (this->config->hardware_id == HW_ES || (this->config->hardware_id == HW_ES_PLUS && this->config->old_esp)) ? (this->mcu->sfr[0x44] ^ 0xff) : this->mcu->sfr[0x46];
    bool reset = false;

    // TODO: use EXICON to determine how KI should be interpreted
    for (const auto &k : this->held_buttons) this->_tick(&reset, &ki, kimask, ko, k);
    if (this->mouse_held) this->_tick(&reset, &ki, kimask, ko, this->held_button_mouse);

    if (!reset) this->mcu->sfr[0x40] = ki ^ 0xff;
}

void keyboard::_tick(bool *reset, uint8_t *ki, uint8_t kimask, uint8_t ko, uint8_t k) {
    if (k != 0xff) {
        uint8_t ki_bit = k & 0xf;
        uint8_t ko_bit = k >> 4;
        if (ko & (1 << ko_bit)) {
            *ki |= (1 << ki_bit);
            if (kimask & (1 << ki_bit)) this->mcu->sfr[0x14] |= this->config->hardware_id == HW_SOLAR_II ? 1 : 2;
        }
    } else {
        this->mcu->reset();
        *reset = true;
    }
}

void keyboard::tick_emu() {
    std::optional<uint8_t> k;
    switch (*this->emu_kb.ES_STOPTYPEADR) {
    case ES_STOP_GETKEY:
        if (this->enable_keypress) {
            k = this->get_button();
            if (!k.has_value()) break;
            uint8_t ki_bit = k.value() & 0xf;
            uint8_t ko_bit = k.value() >> 4;
            printf("[ES_STOP_GETKEY] KI: %d, KO: %d\n", ki_bit, ko_bit);
            *this->emu_kb.ES_KIADR = 1 << ki_bit;
            *this->emu_kb.ES_KOADR = 1 << ko_bit;
            this->mcu->standby->stop_mode = false;
            this->enable_keypress = false;
        }
        break;
    case ES_STOP_ACBREAK:
    case ES_STOP_ACBREAK2:
        if (this->enable_keypress) k = this->get_button();
        if (!k.has_value()) {
            *this->emu_kb.ES_STOPTYPEADR = (es_stop_type)false;
            break;
        }
        *this->emu_kb.ES_STOPTYPEADR = (es_stop_type)(k.value() == 0x42);
        if (k.value() == 0x42) printf("[ES_STOP_ACBREAK] AC pressed!\n");
        break;
    case ES_STOP_QRCODE_IN:
    case ES_STOP_QRCODE_IN3:
        strcpy(this->emu_kb.qr_url, this->emu_kb.ES_QR_DATATOP_ADR);
        this->emu_kb.qr_active = true;
        printf("[ES_STOP_QRCODE_IN] QR code active\n");
        break;
    case ES_STOP_QRCODE_OUT:
        this->emu_kb.qr_active = false;
        printf("[ES_STOP_QRCODE_IN] QR code exit\n");
        break;
    }
}

std::optional<uint8_t> keyboard::get_button() {
    if (this->held_buttons.size()) return this->held_buttons.back();
    else if (this->mouse_held) return this->held_button_mouse;
    return std::nullopt;
}

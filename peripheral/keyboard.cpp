#include <SDL.h>
#include <cstdio>
#include <algorithm>
#include <map>

#include "../mcu/mcu.hpp"
#include "../config/config.hpp"
#include "keyboard.hpp"

keyboard::keyboard(class mcu *mcu, struct config *config, int w, int h) {
    this->mcu = mcu;
    this->config = config;

    this->w = w;
    this->h = h;


    // KI/KO
    // TODO: Actually implement KO filter
    register_sfr(0x41, 4, &default_write<0xff>);
    register_sfr(0x45, 1, &default_write<4>);
    register_sfr(0x46, 1, &default_write<0xff>);
    register_sfr(0x47, 1, &default_write<0x83>);

    // Port 0
    register_sfr(0x48, 5, &default_write<0xff>);
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
    uint8_t ko = this->mcu->sfr[0x46];

    // TODO: use EXICON to determine how KI should be interpreted
    for (const auto &k : this->held_buttons) this->_tick(&ki, kimask, ko, k);
    if (this->mouse_held) this->_tick(&ki, kimask, ko, this->held_button_mouse);

    this->mcu->sfr[0x40] = ki ^ 0xff;
}

void keyboard::_tick(uint8_t *ki, uint8_t kimask, uint8_t ko, uint8_t k) {
    if (k != 0xff) {
        uint8_t ki_bit = k & 0xf;
        uint8_t ko_bit = k >> 4;
        if (ko & (1 << ko_bit)) {
            *ki |= (1 << ki_bit);
            // Fake interrupt code! Need to remove later
            if (kimask & (1 << ki_bit)) {
                this->mcu->sfr[0x14] |= 2;
                this->mcu->standby->stop_mode = false;
            }
        }
    } else this->mcu->reset();
}

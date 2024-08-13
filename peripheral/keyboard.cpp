#include <SDL.h>
#include <cstdio>
#include <algorithm>
#include "../mcu/mcu.hpp"
#include "../config/config.hpp"
#include "keyboard.hpp"

keyboard::keyboard(class mcu *mcu, struct config *config, int w, int h) {
    this->mcu = mcu;
    this->config = config;

    this->w = w;
    this->h = h;

    // KI/KO
    register_sfr(0x41, 4, &default_write<0xff>);
    register_sfr(0x45, 1, &default_write<4>);
    register_sfr(0x46, 1, &default_write<0xff>);
    register_sfr(0x47, 1, &default_write<0x83>);

    // Port 0
    register_sfr(0x48, 5, &default_write<0xff>);
}

void keyboard::process_event(const SDL_Event *e) {
    SDL_Keycode key;
    SDL_Point mousepos;
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
        mousepos.x = e->motion.x;
        mousepos.y = e->motion.y;
        for (const auto &[k, v] : this->config->keymap) {
            if (SDL_PointInRect(&mousepos, &v.rect)) {
                this->held_buttons.push_back(k);
                break;
            }
        }
    case SDL_MOUSEBUTTONUP:
        mousepos.x = e->motion.x;
        mousepos.y = e->motion.y;
        for (const auto &[k, v] : this->config->keymap) {
            if (SDL_PointInRect(&mousepos, &v.rect)) {
                std::vector<uint8_t>::iterator pos = std::find(this->held_buttons.begin(), this->held_buttons.end(), k);
                if (pos != this->held_buttons.end()) {
                    this->held_buttons.erase(pos);
                    break;
                }
            }
        }
        break;
    }
}

void keyboard::render(SDL_Renderer *renderer) {
    SDL_Surface *tmp = SDL_CreateRGBSurface(0, this->w, this->h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    for (const auto &k : this->held_buttons) SDL_FillRect(tmp, &this->config->keymap[k].rect, 0xAA000000);

    SDL_Texture* tmp2 = SDL_CreateTextureFromSurface(renderer, tmp);
    SDL_FreeSurface(tmp);

    SDL_Rect dest {0, 0, this->w, this->h};
    SDL_RenderCopy(renderer, tmp2, NULL, &dest);

    SDL_DestroyTexture(tmp2);
}

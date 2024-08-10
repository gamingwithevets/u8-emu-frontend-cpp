#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cstdio>
#include <iostream>
#include <cmath>
#include "screen.hpp"
#include "../mcu/mcu.hpp"

screen *scrptr;

uint8_t draw_screen(mcu *mcu, uint16_t addr, uint8_t val) {
    printf("F%03XH = 0x%02X\n", addr, val);
    int y = (int)((addr - 0x800) / 0x10);
    int x = ((addr - 0x800) % 0x10) * 8;
    SDL_Rect rect;

    for (int i = 7; i >= 0; i--) {
        rect = {y, x+i, 1, 1};
        SDL_FillRect(scrptr->display, &rect, ((val & (1 << i)) != 0) ? 0xFF000000 : 0xFFD6E3D6);
    }

    return val;
}

screen::screen(struct config *config) {
    this->config = config;
    scrptr = this;

    this->status_bar = IMG_Load(this->config->status_bar_path.c_str());
    if (!this->status_bar) {
        std::cerr << "WARNING: Failed to load status bar image. SDL_image Error: " << IMG_GetError() << std::endl << "Using fallback status bar." << std::endl;
        this->use_status_bar_image = false;
    } else {
        this->sbar_hi = this->status_bar->h;
        this->use_status_bar_image = true;
    }

    this->display = SDL_CreateRGBSurface(0, 96, 32, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_FillRect(this->display, NULL, 0xFFD6E3D6);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    for (int i = 0; i < 32; i++)
        register_sfr(0x800+i*0x10, 0xc, &draw_screen);
}

screen::~screen() {
    SDL_FreeSurface(this->status_bar);
    SDL_FreeSurface(this->display);
}

void screen::render_screen(SDL_Renderer *renderer) {
    SDL_Surface *tmp = SDL_CreateRGBSurface(0, 96*this->config->pix_w, 32*this->config->pix_h+this->sbar_hi, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_Rect dispsrc, dispdest;
    if (this->use_status_bar_image) {
        SDL_Rect srcdest = {0, 0, this->status_bar->w, this->sbar_hi};
        SDL_BlitSurface(this->status_bar, &srcdest, tmp, &srcdest);
        dispsrc = {0, 1, 96, 31};
        dispdest = {this->config->screen_tl_w, this->config->screen_tl_h + this->sbar_hi, 96*this->config->pix_w, 31*this->config->pix_h};
    } else {
        dispsrc = {0, 0, 96, 32};
        dispdest = {this->config->screen_tl_w, this->config->screen_tl_h, 96*this->config->pix_w, 32*this->config->pix_h};
    }

    SDL_BlitSurface(this->display, &dispsrc, tmp, &dispdest);

    SDL_Texture* tmp2 = SDL_CreateTextureFromSurface(renderer, tmp);
    SDL_FreeSurface(tmp);

    SDL_Rect dest {this->config->screen_tl_w, this->config->screen_tl_h, 96*this->config->pix_w, 32*this->config->pix_h+this->sbar_hi};
    SDL_RenderCopy(renderer, tmp2, NULL, &dest);

    SDL_DestroyTexture(tmp2);
}

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cstdio>
#include <iostream>
#include <cmath>
#include "screen.hpp"
#include "../mcu/mcu.hpp"

const uint32_t WHITE_COLOR = 0;
const uint32_t GRAY1_COLOR = 0x55FFFFFF;
const uint32_t GRAY2_COLOR = 0xAAFFFFFF;
const uint32_t BLACK_COLOR = 0xFF000000;

uint8_t draw_screen(mcu *mcu, uint16_t addr, uint8_t val) {
    int y = (int)((addr - 0x800) / mcu->screen->bytes_per_row_real);
    int x = ((addr - 0x800) % mcu->screen->bytes_per_row_real) * 8;
    SDL_Rect rect;

    for (int i = 7; i >= 0; i--) {
        int j = ~i&7;
        rect = {(x+j) * mcu->screen->config->pix_w, y * mcu->screen->config->pix_h, mcu->screen->config->pix_w, mcu->screen->config->pix_h};
        if (mcu->screen->cw_2bpp) {
            if (!(val & (1 << i))) {
                if (SDL_MUSTLOCK(mcu->screen->display)) SDL_LockSurface(mcu->screen->display);
                uint32_t *pixels = (uint32_t *)mcu->screen->display->pixels;
                uint32_t *cur_pixcolor = &pixels[(x+j)*mcu->screen->config->pix_w+y*mcu->screen->config->pix_h];
                if (!mcu->screen->cw_2bpp_toggle) SDL_FillRect(mcu->screen->display, &rect, (*cur_pixcolor == BLACK_COLOR || *cur_pixcolor == GRAY2_COLOR) ? GRAY2_COLOR : WHITE_COLOR);
                else SDL_FillRect(mcu->screen->display, &rect, (*cur_pixcolor == BLACK_COLOR || *cur_pixcolor == GRAY1_COLOR) ? GRAY1_COLOR : WHITE_COLOR);
                if (SDL_MUSTLOCK(mcu->screen->display)) SDL_UnlockSurface(mcu->screen->display);
            } else SDL_FillRect(mcu->screen->display, &rect, mcu->screen->cw_2bpp_toggle ? GRAY2_COLOR : GRAY1_COLOR);
        }
        else SDL_FillRect(mcu->screen->display, &rect, (val & (1 << i)) ? BLACK_COLOR : WHITE_COLOR);
    }

    return val;
}

uint8_t screen_select(mcu *mcu, uint16_t addr, uint8_t val) {
    mcu->screen->cw_2bpp_toggle = (bool)(val & 4);
    return val;
}

screen::screen(class mcu *mcu, struct config *config) {
    this->mcu = mcu;
    this->config = config;

    switch (this->config->hardware_id) {
    case HW_CLASSWIZ_EX:
        this->width = 192;
        this->height = 64;
        this->bytes_per_row = 0x18;
        this->bytes_per_row_real = 0x20;
        this->cw_2bpp = false;

        this->status_bar_bits.push_back({0x00, 0}); // [S]
        this->status_bar_bits.push_back({0x01, 0}); // [A]
        this->status_bar_bits.push_back({0x02, 0}); // M
        this->status_bar_bits.push_back({0x03, 0}); // ->[x]
        this->status_bar_bits.push_back({0x05, 0}); // √[]/
        this->status_bar_bits.push_back({0x06, 0}); // [D]
        this->status_bar_bits.push_back({0x07, 0}); // [R]
        this->status_bar_bits.push_back({0x08, 0}); // [G]
        this->status_bar_bits.push_back({0x09, 0}); // FIX
        this->status_bar_bits.push_back({0x0a, 0}); // SCI
        this->status_bar_bits.push_back({0x0b, 0}); // 𝐄
        this->status_bar_bits.push_back({0x0c, 0}); // 𝒊
        this->status_bar_bits.push_back({0x0d, 0}); // ∠
        this->status_bar_bits.push_back({0x0e, 0}); // ⇩
        this->status_bar_bits.push_back({0x0f, 0}); // ◀
        this->status_bar_bits.push_back({0x11, 0}); // ▼
        this->status_bar_bits.push_back({0x12, 0}); // ▲
        this->status_bar_bits.push_back({0x13, 0}); // ▶
        this->status_bar_bits.push_back({0x15, 0}); // ⏸
        this->status_bar_bits.push_back({0x16, 0}); // ☼

        register_sfr(0x30, 1, &default_write<0x7f>);
        register_sfr(0x31, 1, &default_write<3>);

        break;
    case HW_CLASSWIZ_CW:
        this->width = 192;
        this->height = 64;
        this->bytes_per_row = 0x18;
        this->bytes_per_row_real = 0x20;
        this->cw_2bpp = true;

        this->status_bar_bits.push_back({0x01, 0}); // [S]
        this->status_bar_bits.push_back({0x03, 0}); // √[]/
        this->status_bar_bits.push_back({0x04, 0}); // [D]
        this->status_bar_bits.push_back({0x05, 0}); // [R]
        this->status_bar_bits.push_back({0x06, 0}); // [G]
        this->status_bar_bits.push_back({0x07, 0}); // FIX
        this->status_bar_bits.push_back({0x08, 0}); // SCI
        this->status_bar_bits.push_back({0x0a, 0}); // 𝐄
        this->status_bar_bits.push_back({0x0b, 0}); // 𝒊
        this->status_bar_bits.push_back({0x0c, 0}); // ∠
        this->status_bar_bits.push_back({0x0d, 0}); // ⇩
        this->status_bar_bits.push_back({0x0e, 0}); // ⇩
        this->status_bar_bits.push_back({0x10, 0}); // ◀
        this->status_bar_bits.push_back({0x11, 0}); // ▼
        this->status_bar_bits.push_back({0x12, 0}); // ▲
        this->status_bar_bits.push_back({0x13, 0}); // ▶
        this->status_bar_bits.push_back({0x15, 0}); // ⏸
        this->status_bar_bits.push_back({0x16, 0}); // ☼
        // Graph Light only
        this->status_bar_bits.push_back({0x09, 0}); // f(𝑥)
        this->status_bar_bits.push_back({0x0f, 0}); // g(𝑥)

        register_sfr(0x30, 1, &default_write<0x7f>);
        register_sfr(0x31, 1, &default_write<0xff>);
        register_sfr(0x37, 1, &screen_select);

        break;
    default:
        this->width = 96;
        this->height = 32;
        this->bytes_per_row = 0xc;
        this->bytes_per_row_real = 0x10;
        this->cw_2bpp = false;

        this->status_bar_bits.push_back({0x0, 4}); // [S]
        this->status_bar_bits.push_back({0x0, 2}); // [A]
        this->status_bar_bits.push_back({0x1, 4}); // M
        this->status_bar_bits.push_back({0x1, 1}); // STO
        this->status_bar_bits.push_back({0x2, 6}); // RCL
        this->status_bar_bits.push_back({0x3, 6}); // STAT   | SD
        this->status_bar_bits.push_back({0x4, 7}); // CMPLX  | REG  | 360
        this->status_bar_bits.push_back({0x5, 6}); // MAT    | FMLA | SI
        if (this->config->hardware_id == 2 && this->config->is_5800p)
             this->status_bar_bits.push_back({0x5, 4}); //   | PRGM
        this->status_bar_bits.push_back({0x5, 1}); // VCT    | END  | DMY
        this->status_bar_bits.push_back({0x7, 5}); // [D]
        this->status_bar_bits.push_back({0x7, 1}); // [R]
        this->status_bar_bits.push_back({0x8, 4}); // [G]
        this->status_bar_bits.push_back({0x8, 0}); // FIX
        this->status_bar_bits.push_back({0x9, 5}); // SCI
        this->status_bar_bits.push_back({0xa, 6}); // Math
        this->status_bar_bits.push_back({0xa, 3}); // ▼
        this->status_bar_bits.push_back({0xb, 7}); // ▲
        this->status_bar_bits.push_back({0xb, 4}); // Disp

        register_sfr(0x30, 1, &default_write<0x7f>);
        register_sfr(0x31, 1, &default_write<7>);

        break;
    }

    this->status_bar = IMG_Load(this->config->status_bar_path.c_str());
    if (!this->status_bar) {
        std::cerr << "WARNING: Failed to load status bar image. SDL_image Error: " << IMG_GetError() << std::endl << "Using fallback status bar." << std::endl;
        this->use_status_bar_image = false;
    } else {
        this->sbar_hi = this->status_bar->h;
        this->use_status_bar_image = true;
    }

    this->display = SDL_CreateRGBSurface(0, this->width * this->config->pix_w, this->height * this->config->pix_h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_FillRect(this->display, NULL, WHITE_COLOR);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    for (int i = 0; i < this->height; i++)
        register_sfr(0x800+i*this->bytes_per_row_real, this->bytes_per_row, &draw_screen);
}

screen::~screen() {
    SDL_FreeSurface(this->status_bar);
    SDL_FreeSurface(this->display);
}

void screen::render(SDL_Renderer *renderer) {
    SDL_Surface *tmp = SDL_CreateRGBSurface(0, this->width*this->config->pix_w, this->height*this->config->pix_h+this->sbar_hi, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_Rect dispsrc, dispdest;
    if (this->use_status_bar_image) {
        if (this->mcu->sfr[0x31] == 5 || this->mcu->sfr[0x31] == 6)
            for (int i = 0; i < this->config->status_bar_crops.size(); i++) {
                SDL_Rect srcdest = this->config->status_bar_crops[i];
                statusbit sbb = this->status_bar_bits[i];
                if (this->mcu->sfr[0x800+sbb.idx] & (1 << sbb.bit)) SDL_BlitSurface(this->status_bar, &srcdest, tmp, &srcdest);
            }
        dispsrc = {0, this->config->pix_h, this->width*this->config->pix_w, (this->height-1)*this->config->pix_h};
        dispdest = {0, this->sbar_hi, this->width*this->config->pix_w, (this->height-1)*this->config->pix_h};
    } else {
        if (this->mcu->sfr[0x31] == 5 || this->mcu->sfr[0x31] == 6) {
            dispsrc = {0, 0, this->config->pix_w, this->config->pix_h};
            dispdest = {0, 0, this->config->pix_w, this->config->pix_h};
            SDL_BlitSurface(this->display, &dispsrc, tmp, &dispdest);
        }
        dispsrc = {0, this->sbar_hi, this->width*this->config->pix_w, (this->height-1)*this->config->pix_h};
        dispdest = {0, this->sbar_hi, this->width*this->config->pix_w, (this->height-1)*this->config->pix_h};
    }

    if (this->mcu->sfr[0x31] == 5) SDL_BlitSurface(this->display, &dispsrc, tmp, &dispdest);

    SDL_Texture* tmp2 = SDL_CreateTextureFromSurface(renderer, tmp);
    SDL_FreeSurface(tmp);

    SDL_Rect dest {this->config->screen_tl_w, this->config->screen_tl_h, this->width*this->config->pix_w, this->height*this->config->pix_h+this->sbar_hi};
    SDL_RenderCopy(renderer, tmp2, NULL, &dest);

    SDL_DestroyTexture(tmp2);
}

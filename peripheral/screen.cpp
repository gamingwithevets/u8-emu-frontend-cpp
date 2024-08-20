#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cmath>

#if defined _WIN32 || defined __CYGWIN__
#include <windows.h>
#include <wingdi.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#endif

#include "screen.hpp"
#include "../mcu/mcu.hpp"

const uint32_t NO_COLOR = 0;
const uint32_t WHITE_COLOR = 0xFFFFFFFF;
const uint32_t WHITE_COLOR_CASIO = 0xFFD6E3D6;
const uint32_t GRAY1_COLOR = 0xFFAAAAAA;
const uint32_t GRAY2_COLOR = 0xFF555555;
const uint32_t BLACK_COLOR = 0xFF000000;

// https://stackoverflow.com/a/4790496/24301572
const char *getTmpDir(void) {
    char *tmpdir;

    if ((tmpdir = getenv("TEMP")) != NULL)   return tmpdir;
    if ((tmpdir = getenv("TMP")) != NULL)    return tmpdir;
    if ((tmpdir = getenv("TMPDIR")) != NULL) return tmpdir;

    return "/tmp";
}
const char *tmpdir = getTmpDir();

uint8_t draw_screen(mcu *mcu, uint16_t addr, uint8_t val) {
    int y = (int)((addr - 0x800) / mcu->screen->bytes_per_row_real);
    int x = ((addr - 0x800) % mcu->screen->bytes_per_row_real) * 8;
    SDL_Rect rect;
    int j;

    for (int i = 7; i >= 0; i--) {
        j = ~i & 7;
        rect = {(x+j) * mcu->config->pix_w, y * mcu->config->pix_h, mcu->config->pix_w, mcu->config->pix_h};
        SDL_FillRect(mcu->screen->display, &rect, (val & (1 << i)) ? BLACK_COLOR : NO_COLOR);
    }
    return val;
}

uint8_t draw_screen_cw(mcu *mcu, uint16_t addr, uint8_t val) {
    int y = (int)((addr - 0x800) / mcu->screen->bytes_per_row_real);
    int x = ((addr - 0x800) % mcu->screen->bytes_per_row_real) * 8;
    SDL_Rect rect;
    uint8_t *data;
    int j;

    for (int i = 7; i >= 0; i--) {
        j = ~i & 7;
        data = &mcu->screen->cw_screen_data[y*192+x+j];
        rect = {(x+j) * mcu->config->pix_w, y * mcu->config->pix_h, mcu->config->pix_w, mcu->config->pix_h};
        if (!(val & (1 << i))) {
            if (mcu->screen->cw_2bpp_toggle) SDL_FillRect(mcu->screen->display, &rect, (*data == 3 || *data == 1) ? GRAY1_COLOR : NO_COLOR);
            else SDL_FillRect(mcu->screen->display, &rect, (*data == 3 || *data == 2) ? GRAY2_COLOR : NO_COLOR);
            *data &= ~(mcu->screen->cw_2bpp_toggle ? 2 : 1);
        } else {
            if (mcu->screen->cw_2bpp_toggle) SDL_FillRect(mcu->screen->display, &rect, (*data == 0 || *data == 2) ? GRAY2_COLOR : BLACK_COLOR);
            else SDL_FillRect(mcu->screen->display, &rect, (*data == 0 || *data == 1) ? GRAY1_COLOR : BLACK_COLOR);
            *data |= mcu->screen->cw_2bpp_toggle ? 2 : 1;
        }
    }

    if (SDL_MUSTLOCK(mcu->screen->display)) SDL_UnlockSurface(mcu->screen->display);
    return val;
}

uint8_t screen_select(mcu *mcu, uint16_t addr, uint8_t val) {
    mcu->screen->cw_2bpp_toggle = (bool)(val & 4);
    return val;
}

screen::screen(class mcu *mcu) {
    this->mcu = mcu;
    this->config = mcu->config;

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
        register_sfr(0x31, 1, &default_write<0xff>);

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
    case HW_TI_MATHPRINT:
        this->width = 192;
        this->height = 64;
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
    SDL_FillRect(this->display, NULL, NO_COLOR);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    if (this->config->hardware_id != HW_TI_MATHPRINT)
        for (int i = 0; i < this->height; i++) register_sfr(0x800+i*this->bytes_per_row_real, this->bytes_per_row, this->cw_2bpp ? &draw_screen_cw : &draw_screen);
}

screen::~screen() {
    SDL_FreeSurface(this->status_bar);
    SDL_FreeSurface(this->display);
}

SDL_Surface *screen::get_surface(uint32_t background) {
    if (this->config->hardware_id == HW_TI_MATHPRINT) {
        SDL_Surface *tmp = SDL_CreateRGBSurface(0, 192*this->config->pix_w, 64*this->config->pix_h+this->sbar_hi, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        SDL_FillRect(tmp, NULL, background);
        if (!this->mcu->ti_status_bar_addr) return tmp;
        if (this->use_status_bar_image) {
            uint32_t data = (this->mcu->ram[this->mcu->ti_status_bar_addr+2] << 16) | (this->mcu->ram[this->mcu->ti_status_bar_addr+1] << 8) | this->mcu->ram[this->mcu->ti_status_bar_addr];
            for (int i = 0; i < this->config->status_bar_crops.size(); i++) {
                SDL_Rect srcdest = this->config->status_bar_crops[i];
                if (data & (1 << i)) SDL_BlitSurface(this->status_bar, &srcdest, tmp, &srcdest);
            }
        }

        if (!this->mcu->ti_screen_addr) return tmp;
        if (this->mcu->ti_screen_changed) {
            SDL_FillRect(this->display, NULL, NO_COLOR);
            for (int y = 0; y < 192; y++) {
                int yinv = ~y & 191;
                for (int x = 0; x < 8; x++) {
                    SDL_Rect k;
                    for (int i = 7; i >= 0; i--) {
                        k = {y * this->config->pix_h, (x*8+i) * this->config->pix_w, this->config->pix_w, this->config->pix_h};
                        if (this->mcu->ram[this->mcu->ti_screen_addr+y*8+x] & (1 << i)) SDL_FillRect(this->display, &k, BLACK_COLOR);
                    }
                }
            }
            this->mcu->ti_screen_changed = false;
        }

        SDL_Rect dispsrc {0, this->config->pix_h, 192*this->config->pix_w, 64*this->config->pix_h};
        SDL_Rect dispdest {0, this->sbar_hi, 192*this->config->pix_w, 64*this->config->pix_h};
        SDL_BlitSurface(this->display, &dispsrc, tmp, &dispdest);

        return tmp;
    } else {
        SDL_Surface *tmp = SDL_CreateRGBSurface(0, this->width*this->config->pix_w, this->height*this->config->pix_h+this->sbar_hi, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        SDL_FillRect(tmp, NULL, background);
        SDL_Rect dispsrc, dispdest;
        if (this->use_status_bar_image) {
            if ((this->mcu->sfr[0x31] & 0xf) == 5 || (this->mcu->sfr[0x31] & 0xf) == 6)
                for (int i = 0; i < this->config->status_bar_crops.size(); i++) {
                    SDL_Rect srcdest = this->config->status_bar_crops[i];
                    statusbit sbb = this->status_bar_bits[i];
                    if (this->mcu->sfr[0x800+sbb.idx] & (1 << sbb.bit)) SDL_BlitSurface(this->status_bar, &srcdest, tmp, &srcdest);
                }
            dispsrc = {0, this->config->pix_h, this->width*this->config->pix_w, (this->height-1)*this->config->pix_h};
            dispdest = {0, this->sbar_hi, this->width*this->config->pix_w, (this->height-1)*this->config->pix_h};
        } else {
            if ((this->mcu->sfr[0x31] & 0xf) == 5 || (this->mcu->sfr[0x31] & 0xf) == 6) {
                dispsrc = {0, 0, this->config->pix_w, this->config->pix_h};
                dispdest = {0, 0, this->config->pix_w, this->config->pix_h};
                SDL_BlitSurface(this->display, &dispsrc, tmp, &dispdest);
            }
            dispsrc = {0, this->sbar_hi, this->width*this->config->pix_w, (this->height-1)*this->config->pix_h};
            dispdest = {0, this->sbar_hi, this->width*this->config->pix_w, (this->height-1)*this->config->pix_h};
        }

        if ((this->mcu->sfr[0x31] & 0xf) == 5) SDL_BlitSurface(this->display, &dispsrc, tmp, &dispdest);

        return tmp;
    }
}

void screen::render(SDL_Renderer *renderer) {
    SDL_Surface* tmp = this->get_surface();

    SDL_Texture* tmp2 = SDL_CreateTextureFromSurface(renderer, tmp);
    SDL_FreeSurface(tmp);

    SDL_Rect dest;
    if (this->config->hardware_id == HW_TI_MATHPRINT) dest = {this->config->screen_tl_w, this->config->screen_tl_h, 192*this->config->pix_w, 64*this->config->pix_h+this->sbar_hi};
    else dest = {this->config->screen_tl_w, this->config->screen_tl_h, this->width*this->config->pix_w, this->height*this->config->pix_h+this->sbar_hi};
    SDL_RenderCopy(renderer, tmp2, NULL, &dest);

    SDL_DestroyTexture(tmp2);
}

void screen::save(const char *fname) {
    SDL_Surface* tmp = this->get_surface(this->config->hardware_id == HW_TI_MATHPRINT ? WHITE_COLOR : WHITE_COLOR_CASIO);
    IMG_SavePNG(tmp, fname);
}

// not generated with ChatGPT i swear
#if defined _WIN32 || defined __CYGWIN__
bool CopyPNGToClipboard(const char* filePath) {
    // Load the PNG file using stb_image
    int width, height, channels;
    unsigned char* data = stbi_load(filePath, &width, &height, &channels, 4);

    if (data == NULL) {
        std::cerr << "Failed to load PNG file." << std::endl;
        return false;
    }

    // Create a compatible device context and bitmap
    HDC hdc = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);

    if (hBitmap == NULL) {
        std::cerr << "Failed to create bitmap." << std::endl;
        stbi_image_free(data);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdc);
        return false;
    }

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Negative to indicate top-down bitmap
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    // Set the bitmap bits
    if (SetDIBits(hdcMem, hBitmap, 0, height, data, &bmi, DIB_RGB_COLORS) == 0) {
        std::cerr << "Failed to set bitmap bits." << std::endl;
        stbi_image_free(data);
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdc);
        return false;
    }

    // Clean up
    stbi_image_free(data);
    SelectObject(hdcMem, hOldBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdc);

    // Open the clipboard
    if (!OpenClipboard(NULL)) {
        std::cerr << "Failed to open clipboard." << std::endl;
        DeleteObject(hBitmap);
        return false;
    }

    // Empty the clipboard
    EmptyClipboard();

    // Set the bitmap to the clipboard
    if (SetClipboardData(CF_BITMAP, hBitmap) == NULL) {
        std::cerr << "Failed to set clipboard data." << std::endl;
        CloseClipboard();
        DeleteObject(hBitmap);
        return false;
    }

    // Close the clipboard
    CloseClipboard();

    // Clean up
    DeleteObject(hBitmap);

    return true;
}

bool screen::render_clipboard() {
    char fname[260]; strcpy(fname, tmpdir);
    strcat(fname, "/image.png");
    this->save(fname);
    bool success = CopyPNGToClipboard(fname);
    remove(fname);
    return success;
}
#endif

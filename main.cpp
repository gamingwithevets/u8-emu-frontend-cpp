#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "mcu/mcu.hpp"
#include "config/config.hpp"
extern "C" {
#include "u8_emu/src/core/core.h"
}

const int screen_tl_w = 58;
const int screen_tl_h = 132;
const int DISPLAY_WIDTH = 96;
const int DISPLAY_HEIGHT = 31;
const int pix = 3;

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }
    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Surface* interface_sf = IMG_Load("images/interface_esp_991esp.png");
    if (interface_sf == nullptr) {
        std::cerr << "Unable to load interface image! SDL_image Error: " << IMG_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    int w = interface_sf->w;
    int h = interface_sf->h;

    SDL_Window* window = SDL_CreateWindow("u8-emu-frontend-cpp-test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(interface_sf);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_FreeSurface(interface_sf);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Texture* interface = SDL_CreateTextureFromSurface(renderer, interface_sf);
    SDL_FreeSurface(interface_sf);

    SDL_Surface* status_bar_sf = IMG_Load("images/interface_es_bar.png");
    if (!status_bar_sf) {
        std::cerr << "Unable to load status bar image! SDL_image Error: " << IMG_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return -1;
    }
    int sbar_wd = status_bar_sf->w;
    int sbar_hi = status_bar_sf->h;
    SDL_Texture* status_bar = SDL_CreateTextureFromSurface(renderer, status_bar_sf);
    SDL_FreeSurface(status_bar_sf);

    SDL_Surface* displaySurface = SDL_CreateRGBSurface(0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_FillRect(displaySurface, NULL, SDL_MapRGB(displaySurface->format, 214, 227, 214));

    SDL_Texture* display = SDL_CreateTextureFromSurface(renderer, displaySurface);
    SDL_FreeSurface(displaySurface);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    SDL_Rect status_bar_dest {screen_tl_w, screen_tl_h, sbar_wd, sbar_hi};
    SDL_Rect dispsrc {0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT};
    SDL_Rect dispdest {screen_tl_w, screen_tl_h + sbar_hi, DISPLAY_WIDTH*pix, DISPLAY_HEIGHT*pix};

    bool quit = false;
    SDL_Event e;

    u8_core core;
    config config = {0};

    // Test config!!!!
    config.rom_file = "roms/GY454XE .bin";
    config.hardware_id = 3;
    config.real_hardware = true;

    uint8_t *rom = (uint8_t *)malloc(0x100000);
    uint8_t *flash = nullptr;

    FILE *f = fopen(config.rom_file.c_str(), "rb");
    if (!f) {
        std::cout << "Cannot open ROM!" << std::endl;
        return -1;
    }
    fread(rom, sizeof(uint8_t), 0x20000, f);

    std::cout << "Initializing emulated MCU." << std::endl;
    mcu mcu(&core, &config, rom, flash, 0x80000, 0xe00);

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        mcu.core_step();

        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, interface, NULL, NULL);
        SDL_RenderCopy(renderer, status_bar, NULL, &status_bar_dest);
        SDL_RenderCopy(renderer, display, &dispsrc, &dispdest);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(display);
    SDL_DestroyTexture(interface);
    SDL_DestroyTexture(status_bar);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}

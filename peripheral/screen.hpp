#ifndef SCREEN
#define SCREEN

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include "../config/config.hpp"
#include "../mcu/mcu.hpp"

class screen {
private:
    struct config *config;
public:
    SDL_Surface *display;
    screen(struct config *config);
    ~screen();
    void render_screen(SDL_Renderer *renderer);
private:
    bool use_status_bar_image;
    SDL_Surface *status_bar;
    int sbar_hi;
};

#endif

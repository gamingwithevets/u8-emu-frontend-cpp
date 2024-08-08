#ifndef CONFIG
#define CONFIG
#include <SDL2/SDL.h>
#include <cstdint>
#include <iostream>
#include <vector>
#include <map>

struct keydata {
    SDL_Rect rect;
    std::vector<std::string> keys;
};

struct config {
    char header[30];
/* Header data:
  47 65 6E 73 68 69 74 20 63 6F 6E 66 69 67 75
  72 61 74 69 6F 6E 20 66 69 6C 65 20 76 36 39
*/
    // Emulator config
    std::string rom_file;
    std::string flash_rom_file;
    std::vector<std::string> labels;
    int hardware_id;
    bool real_hardware;
    bool sample;
    bool is_5800p;
    bool old_esp;
    uint8_t pd_value;
    // Window config
    std::string status_bar_path;
    std::string interface_path;
    std::string w_name;
    int screen_tl_w;
    int screen_tl_h;
    int pix_w;
    int pix_h;
    SDL_Color pix_color;
    std::vector<SDL_Rect> status_bar_crops;
    // Keyboard
    std::vector<std::map<uint8_t, keydata>> keymap;
};
#endif

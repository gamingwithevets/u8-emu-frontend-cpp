#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <SDL.h>
#include "binary.hpp"

struct keydata {
    SDL_Rect rect{};
    std::vector<SDL_Keycode> keys;
    void Write(std::ostream &os) const {
        Binary::Write(os, rect);
        Binary::Write(os, keys);
    }
    void Read(std::istream &is) {
        Binary::Read(is, rect);
        Binary::Read(is, keys);
    }
};

struct color_info {
    uint8_t r, g, b;
};

enum hardware_id : int {
    HW_SOLAR_II = 0,
    HW_ES = 2,
    HW_ES_PLUS = 3,
    HW_CLASSWIZ_EX = 4,
    HW_CLASSWIZ_CW = 5,
    HW_TI_MATHPRINT = 6
};

struct config {
    const std::string header = "Genshit configuration file v69";

    // Emulator config
    std::string rom_file;
    std::string flash_rom_file;
    hardware_id hardware_id{};
    bool real_hardware{};
    bool sample{};
    bool is_5800p{};
    bool old_esp{};
    uint8_t pd_value{};
    // Window config
    std::string status_bar_path;
    std::string interface_path;
    std::string w_name;
    int screen_tl_w{};
    int screen_tl_h{};
    int pix_w{};
    int pix_h{};
    color_info pix_color{};
    std::vector<SDL_Rect> status_bar_crops;
    // Keyboard
    std::map<uint8_t, keydata> keymap;

    int width;
    int height;
    std::string ram;

    void Write(std::ostream &os) const {
        Binary::Write(os, header);

        Binary::Write(os, rom_file);
        Binary::Write(os, flash_rom_file);
        Binary::Write(os, hardware_id);
        Binary::Write(os, real_hardware);
        Binary::Write(os, sample);
        Binary::Write(os, is_5800p);
        Binary::Write(os, old_esp);
        Binary::Write(os, pd_value);

        Binary::Write(os, status_bar_path);
        Binary::Write(os, interface_path);
        Binary::Write(os, w_name);
        Binary::Write(os, screen_tl_w);
        Binary::Write(os, screen_tl_h);
        Binary::Write(os, pix_w);
        Binary::Write(os, pix_h);
        Binary::Write(os, pix_color);
        Binary::Write(os, status_bar_crops);

        Binary::Write(os, keymap);

        Binary::Write(os, width);
        Binary::Write(os, height);
        Binary::Write(os, ram);
    }
    void Read(std::istream &is) {
        std::string unused;
        Binary::Read(is, unused);

        Binary::Read(is, rom_file);
        Binary::Read(is, flash_rom_file);
        Binary::Read(is, hardware_id);
        Binary::Read(is, real_hardware);
        Binary::Read(is, sample);
        Binary::Read(is, is_5800p);
        Binary::Read(is, old_esp);
        Binary::Read(is, pd_value);

        Binary::Read(is, status_bar_path);
        Binary::Read(is, interface_path);
        Binary::Read(is, w_name);
        Binary::Read(is, screen_tl_w);
        Binary::Read(is, screen_tl_h);
        Binary::Read(is, pix_w);
        Binary::Read(is, pix_h);
        Binary::Read(is, pix_color);
        Binary::Read(is, status_bar_crops);

        Binary::Read(is, keymap);

        Binary::Read(is, width);
        Binary::Read(is, height);
        Binary::Read(is, ram);
    }
};

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <SDL.h>
#include "config.hpp"

void readString(std::ifstream& ifs, std::string& str) {
    size_t size;
    ifs.read(reinterpret_cast<char*>(&size), sizeof(size));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read string size");
    }
    str.resize(size);
    ifs.read(&str[0], size);
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read string data");
    }
}

void readSDL_Rect(std::ifstream& ifs, SDL_Rect& rect) {
    ifs.read(reinterpret_cast<char*>(&rect), sizeof(rect));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read SDL_Rect data");
    }
}

void readKeydata(std::ifstream& ifs, keydata& kd) {
    readSDL_Rect(ifs, kd.rect);
    size_t keys_size;
    ifs.read(reinterpret_cast<char*>(&keys_size), sizeof(keys_size));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read keys vector size");
    }
    kd.keys.resize(keys_size);
    ifs.read(reinterpret_cast<char*>(kd.keys.data()), keys_size * sizeof(SDL_Keycode));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read keys vector data");
    }
}

void deserialize(config& cfg, const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Could not open file for reading");
    }

    ifs.read(cfg.header, sizeof(cfg.header));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read header");
    }

    readString(ifs, cfg.rom_file);
    readString(ifs, cfg.flash_rom_file);
    ifs.read(reinterpret_cast<char*>(&cfg.hardware_id), sizeof(cfg.hardware_id));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read hardware_id");
    }
    ifs.read(reinterpret_cast<char*>(&cfg.real_hardware), sizeof(cfg.real_hardware));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read real_hardware");
    }
    ifs.read(reinterpret_cast<char*>(&cfg.sample), sizeof(cfg.sample));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read sample");
    }
    ifs.read(reinterpret_cast<char*>(&cfg.is_5800p), sizeof(cfg.is_5800p));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read is_5800p");
    }
    ifs.read(reinterpret_cast<char*>(&cfg.old_esp), sizeof(cfg.old_esp));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read old_esp");
    }
    ifs.read(reinterpret_cast<char*>(&cfg.pd_value), sizeof(cfg.pd_value));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read pd_value");
    }
    readString(ifs, cfg.status_bar_path);
    readString(ifs, cfg.interface_path);
    readString(ifs, cfg.w_name);
    ifs.read(reinterpret_cast<char*>(&cfg.screen_tl_w), sizeof(cfg.screen_tl_w));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read screen_tl_w");
    }
    ifs.read(reinterpret_cast<char*>(&cfg.screen_tl_h), sizeof(cfg.screen_tl_h));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read screen_tl_h");
    }
    ifs.read(reinterpret_cast<char*>(&cfg.pix_w), sizeof(cfg.pix_w));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read pix_w");
    }
    ifs.read(reinterpret_cast<char*>(&cfg.pix_h), sizeof(cfg.pix_h));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read pix_h");
    }
    ifs.read(reinterpret_cast<char*>(&cfg.pix_color), sizeof(cfg.pix_color));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read pix_color");
    }

    size_t status_bar_crops_size;
    ifs.read(reinterpret_cast<char*>(&status_bar_crops_size), sizeof(status_bar_crops_size));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read status_bar_crops vector size");
    }
    cfg.status_bar_crops.resize(status_bar_crops_size);
    for (auto& rect : cfg.status_bar_crops) {
        readSDL_Rect(ifs, rect);
    }

    size_t keymap_size;
    ifs.read(reinterpret_cast<char*>(&keymap_size), sizeof(keymap_size));
    if (ifs.fail()) {
        throw std::runtime_error("Failed to read keymap size");
    }
    for (size_t i = 0; i < keymap_size; ++i) {
        uint8_t key;
        keydata kd;
        ifs.read(reinterpret_cast<char*>(&key), sizeof(key));
        if (ifs.fail()) {
            throw std::runtime_error("Failed to read keymap key");
        }
        readKeydata(ifs, kd);
        cfg.keymap[key] = kd;
    }
}

/*
    u8-emu-frontend-cpp
    Copyright (C) 2024-2025  GamingWithEvets Inc.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

#include <map>
#include <vector>
#include <array>
#include <cstdint>
#include <atomic>
#include <mutex>
#include "../config/config.hpp"
#include "datalabels.hpp"
#include "../peripheral/standby.hpp"
#include "../peripheral/wdt.hpp"
#include "../peripheral/interrupts.hpp"
#include "../peripheral/timer.hpp"
#include "../peripheral/ltb.hpp"
#include "../peripheral/keyboard.hpp"
#include "../peripheral/battery.hpp"
#include "../peripheral/bcd.hpp"
#include "../peripheral/screen.hpp"
extern "C" {
#include "../u8_emu/src/core/core.h"
}
#include "../imgui/imgui.h"

template <uint8_t mask>
static uint8_t default_write(MCU *mcu, uint16_t addr, uint8_t val) {
    return val & mask;
}

double get_time();

typedef struct {
    uint32_t func_addr;
    uint16_t er0;
    uint16_t er2;
    uint32_t return_addr;
    uint16_t return_addr_ptr;
    int_callstack interrupt;
} call_stack_data;

typedef struct {
    int read;
    int write;
} wanted_sfrs_data;

class MCU {
public:
    struct Config *config;

    dlabels *labels;
    std::map<uint32_t, wanted_sfrs_data> wanted_sfrs;
    std::mutex wanted_sfrs_mutex;

    // Peripherals
    Standby *standby;
    WDT *wdt;
    Interrupts *interrupts;
    SFRTimer *timer;
    LTB *ltb;
    Keyboard *keyboard;
    Battery *battery;
    BCD *bcd;
    class Screen *screen;

    struct u8_core *core;
	int flash_mode;
    uint8_t *rom;
    uint8_t *flash;
    uint8_t *ram;
    uint16_t ramstart;
    uint8_t *sfr;
    uint8_t *ram2;
	uint8_t (*sfr_write[0x1000])(MCU*, uint16_t, uint8_t);
	std::vector<call_stack_data> call_stack;
	std::mutex call_stack_mutex;
    double ips, ips_start;
    unsigned int ips_ctr;
    std::atomic<int> cps_multiplier;
    bool paused;

    uint16_t ti_screen_addr;
    uint16_t ti_status_bar_addr;
    bool ti_screen_changed;

	MCU(struct u8_core *core, struct Config *config, uint8_t *rom, uint8_t *flash, uint8_t *ram, int ramstart, int ramsize, int w, int h);
	~MCU();
	void core_step();
	void raise_int(std::string interrupt_name);
	void reset();
};

void core_step_loop(std::atomic<bool>& stop);
void register_sfr(uint16_t addr, uint16_t len, uint8_t (*callback)(MCU*, uint16_t, uint8_t));
ImU8 read_sfr_im(const ImU8*, size_t addr, void *);
void write_sfr_im(ImU8*, size_t addr, ImU8 val, void *);

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
#include "../peripheral/screen.hpp"
extern "C" {
#include "../u8_emu/src/core/core.h"
}
#include "../imgui/imgui.h"

template <uint8_t mask>
static uint8_t default_write(mcu *mcu, uint16_t addr, uint8_t val) {
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

class mcu {
public:
    struct config *config;

    dlabels *labels;
    std::map<uint32_t, wanted_sfrs_data> wanted_sfrs;
    std::mutex wanted_sfrs_mutex;

    // Peripherals
    standby *standby;
    wdt *wdt;
    interrupts *interrupts;
    sfrtimer *timer;
    ltb *ltb;
    keyboard *keyboard;
    battery *battery;
    class bcd *bcd;
    class screen *screen;

    struct u8_core *core;
	int flash_mode;
    uint8_t *rom;
    uint8_t *flash;
    uint8_t *ram;
    uint16_t ramstart;
    uint8_t *sfr;
    uint8_t *ram2;
	uint8_t (*sfr_write[0x1000])(mcu*, uint16_t, uint8_t);
	std::vector<call_stack_data> call_stack;
	std::mutex call_stack_mutex;
    double ips, ips_start;
    unsigned int ips_ctr;
    std::atomic<int> cps_multiplier;
    bool paused;

    uint16_t ti_screen_addr;
    uint16_t ti_status_bar_addr;
    bool ti_screen_changed;

	mcu(struct u8_core *core, struct config *config, uint8_t *rom, uint8_t *flash, uint8_t *ram, int ramstart, int ramsize, int w, int h);
	~mcu();
	void core_step();
	void raise_int(std::string interrupt_name);
	void reset();
};

void core_step_loop(std::atomic<bool>& stop);
void register_sfr(uint16_t addr, uint16_t len, uint8_t (*callback)(mcu*, uint16_t, uint8_t));
ImU8 read_sfr_im(const ImU8*, size_t addr, void *);
void write_sfr_im(ImU8*, size_t addr, ImU8 val, void *);

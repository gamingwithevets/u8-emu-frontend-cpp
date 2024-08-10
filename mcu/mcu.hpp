#ifndef MCU
#define MCU
#include <cstdint>
#include <atomic>
#include "../config/config.hpp"
extern "C" {
#include "../u8_emu/src/core/core.h"
}

class mcu {
private:
    struct config *config;
public:
    struct u8_core *core;
	int flash_mode;
    uint8_t *rom;
    uint8_t *flash;
    uint8_t *ram;
    uint8_t *sfr;
    uint8_t *ram2;
	uint8_t (*sfr_write[0x1000])(mcu*, uint16_t, uint8_t);
public:
	mcu(struct u8_core *core, struct config *config, uint8_t *rom, uint8_t *flash, int ramstart, int ramsize);
	~mcu();
	void core_step();
	void core_step_loop(std::atomic<bool>& stop);
};

uint8_t default_write(mcu *mcu, uint16_t addr, uint8_t val);
void register_sfr(uint16_t addr, uint16_t len, uint8_t (*callback)(mcu*, uint16_t, uint8_t));

#endif

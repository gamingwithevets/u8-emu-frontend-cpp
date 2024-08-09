#ifndef MCU
#define MCU
#include <cstdint>
#include "../config/config.hpp"
extern "C" {
#include "../u8_emu/src/core/core.h"
}

class mcu {
public:
    struct u8_core *core;
    struct config *config;
	int flash_mode;
    uint8_t *rom;
    uint8_t *flash;
    uint8_t *ram;
    uint8_t *sfr;
    uint8_t *ram2;
	uint8_t (*sfr_write[0x1000])(mcu*, uint16_t, uint8_t);

	mcu(struct u8_core *core, struct config *config, uint8_t *rom, uint8_t *flash, int ramstart, int ramsize);
	void core_step(void);
};

#endif

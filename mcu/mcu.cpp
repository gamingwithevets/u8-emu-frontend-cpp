#include <cstdint>
extern "C" {
#include "../u8-emu/src/core/core.h"
}


class mcu {
public:
    struct u8_core *core;

    uint8_t *rom;
    uint8_t *flash;
    uint8_t *ram;
    uint8_t *emu_seg;
    uint8_t (*sfr_write[0x1000])(mcu, uint16_t, uint8_t);
};

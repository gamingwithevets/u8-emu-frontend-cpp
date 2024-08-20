#include <cstdint>
#include "../mcu/mcu.hpp"
#include "ltb.hpp"

ltb::ltb(class mcu *mcu) {
    this->mcu = mcu;
    this->timer = new class timer(128);

    register_sfr(0x60, 1, &default_write<0xff>);
}

void ltb::tick() {
    this->timer->tick();
    uint8_t c0 = this->mcu->sfr[0x60];
    uint8_t c1 = c0 + 1;
    if ((c0 ^ c1) & 0xc0) this->mcu->standby->stop_mode = false;
    this->mcu->sfr[0x60] = c1;
}

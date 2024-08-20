#include <cstdint>
#include "../mcu/mcu.hpp"
#include "ltb.hpp"

ltb::ltb(class mcu *mcu) {
    this->mcu = mcu;
    this->timer = new class timer(128);

    register_sfr(0x20, 4, &default_write<0xff>);
    register_sfr(0x24, 1, &default_write<0xf>);
    register_sfr(0x25, 1, &default_write<1>);
}

void ltb::tick() {
    this->timer->tick();
    if (this->mcu->sfr[0x25]) {
        uint16_t counter = (this->mcu->sfr[0x23] << 8) | this->mcu->sfr[0x22];
        uint16_t target = (this->mcu->sfr[0x21] << 8) | this->mcu->sfr[0x20];

        counter += this->timer->ticks;
        this->mcu->sfr[0x22] = (uint8_t)counter;
        this->mcu->sfr[0x23] = (uint8_t)(counter >> 8);

        if (counter >= target && this->mcu->standby->stop_mode) this->mcu->sfr[0x14] |= 0x20;
    }
}

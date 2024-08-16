#include "battery.hpp"
#include "../config/config.hpp"
#include "../mcu/mcu.hpp"

uint8_t bldcon(mcu *mcu, uint16_t addr, uint8_t val) {
    val &= 7;
    // Placeholder. Need to implement interrupts first.
    return (mcu->sfr[0xd0] == 3 && mcu->sfr[0xd2] == 0 && val == 5) ? 6 : val;
}

battery::battery(struct config *config) {
    this->config = config;

    if (this->config->hardware_id == HW_CLASSWIZ_EX || this->config->hardware_id == HW_CLASSWIZ_CW) {
        register_sfr(0xd0, 1, &default_write<0x1f>);
        register_sfr(0xd1, 1, &bldcon);
        register_sfr(0xd2, 1, &default_write<0x3f>);
    }
}

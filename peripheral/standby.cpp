#include <cstdio>
#include <cstdint>
#include "../mcu/mcu.hpp"
#include "standby.hpp"

uint8_t stpacp(mcu *mcu, uint16_t addr, uint8_t val) {
    if (mcu->standby->stop_accept[0]) {
        if ((val & 0xf0) == 0xa0) mcu->standby->stop_accept[1] = true;
        else mcu->standby->stop_accept[0] = false;
    } else if ((val & 0xf0) == 0x50) mcu->standby->stop_accept[0] = true;
    return 0;
}

uint8_t sbycon(mcu *mcu, uint16_t addr, uint8_t val) {
    if (val & (1 << 1)) {
        if (mcu->standby->stop_accept[0] && mcu->standby->stop_accept[1]) {
            mcu->standby->stop_mode = true;
            mcu->standby->stop_accept[0] = false;
            mcu->standby->stop_accept[1] = false;
            mcu->sfr[0x22] = 0;
            mcu->sfr[0x23] = 0;
            if (!mcu->config->real_hardware) {
                *mcu->keyboard->emu_kb.ES_KIADR = 0;
                *mcu->keyboard->emu_kb.ES_KOADR = 0;
            }
        }
    }
    return 0;
}

standby::standby() {
    this->stop_accept[0] = false;
    this->stop_accept[1] = false;
    this->stop_mode = false;

    register_sfr(8, 1, stpacp);
    register_sfr(9, 1, sbycon);
}

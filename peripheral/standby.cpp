#include <cstdint>
#include "../mcu/mcu.hpp"
#include "standby.hpp"

standby *sbyptr;

uint8_t stpacp(mcu *, uint16_t addr, uint8_t val) {
    if (sbyptr->stop_accept[0]) {
        if ((val & 0xf0) == 0xa0) sbyptr->stop_accept[1] = true;
        else sbyptr->stop_accept[0] = false;
    } else if ((val & 0xf0) == 0x50) sbyptr->stop_accept[0] = true;
    return 0;
}

uint8_t sbycon(mcu *mcu, uint16_t addr, uint8_t val) {
    if (val & (1 << 1)) {
        if (sbyptr->stop_accept[0] && sbyptr->stop_accept[1]) {
            sbyptr->stop_mode = true;
            sbyptr->stop_accept[0] = false;
            sbyptr->stop_accept[1] = false;
            mcu->sfr[0x22] = 0;
            mcu->sfr[0x23] = 0;
        }
    }
    return 0;
}

standby::standby() {
    sbyptr = this;
    this->stop_accept[0] = false;
    this->stop_accept[1] = false;
    this->stop_mode = false;

    register_sfr(8, 1, stpacp);
    register_sfr(9, 1, sbycon);
}

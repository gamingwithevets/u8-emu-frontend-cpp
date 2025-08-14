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
#include <cstdio>
#include <cstdint>
#include "../mcu/mcu.hpp"
#include "standby.hpp"

uint8_t stpacp(MCU *mcu, uint16_t addr, uint8_t val) {
    if (mcu->standby->stop_accept[0]) {
        if ((val & 0xf0) == 0xa0) mcu->standby->stop_accept[1] = true;
        else mcu->standby->stop_accept[0] = false;
    } else if ((val & 0xf0) == 0x50) mcu->standby->stop_accept[0] = true;
    return 0;
}

uint8_t sbycon(MCU *mcu, uint16_t addr, uint8_t val) {
    if (val & (1 << 1)) {
        if (mcu->standby->stop_accept[0] && mcu->standby->stop_accept[1]) {
            mcu->standby->stop_mode = true;
            mcu->paused = true;
            mcu->standby->stop_accept[0] = false;
            mcu->standby->stop_accept[1] = false;
            mcu->sfr[0x22] = 0;
            mcu->sfr[0x23] = 0;
            if (!mcu->config->real_hardware && mcu->config->hardware_id != HW_TI_MATHPRINT) {
                *mcu->keyboard->emu_kb.ES_KIADR = 0;
                *mcu->keyboard->emu_kb.ES_KOADR = 0;
            }
        }
    }
    return 0;
}

Standby::Standby() {
    this->stop_accept[0] = false;
    this->stop_accept[1] = false;
    this->stop_mode = false;

    register_sfr(8, 1, stpacp);
    register_sfr(9, 1, sbycon);
}

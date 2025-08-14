/*
    u8-emu-frontend-cpp LTB emulation
    Copyright (C) 2024  Xyzstk
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
#include <cstdint>
#include "../mcu/mcu.hpp"
#include "ltb.hpp"

LTB::LTB(class MCU *mcu) {
    this->mcu = mcu;
    this->timer = new class Timer(128);

    register_sfr(0x60, 1, &default_write<0xff>);
}

void LTB::tick() {
    this->timer->tick();
    uint8_t c0 = this->mcu->sfr[0x60];
    uint8_t c1 = c0 + 1;
    if ((c0 ^ c1) & 0xc0) this->mcu->standby->stop_mode = false;
    this->mcu->sfr[0x60] = c1;
}

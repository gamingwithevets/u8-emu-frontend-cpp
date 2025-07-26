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

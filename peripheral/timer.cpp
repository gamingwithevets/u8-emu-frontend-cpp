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
#include <cstdint>
#include "../mcu/mcu.hpp"
#include "timer.hpp"

Timer::Timer(double tps) {
    this->time_scale = 1;
    this->passed_time = 0;
    this->tps = tps;
    this->last_time = get_time() * 1e9;
}

void Timer::tick() {
    uint64_t now = get_time() * 1e9;
    uint64_t passed_ns = now - this->last_time;
    this->last_time = now;
    if (passed_ns < 0) passed_ns = 0;
    if (passed_ns > 1e9) passed_ns = 1e9;

    this->passed_time += passed_ns * this->time_scale * this->tps / 1e9;
    this->ticks = (int)this->passed_time;
    if (this->ticks > 100) this->ticks = 100;
    this->passed_time -= this->ticks;
}

SFRTimer::SFRTimer(class MCU *mcu) {
    this->mcu = mcu;
    this->timer = new class Timer(10000);

    if (this->mcu->config->hardware_id == HW_TI_MATHPRINT) {
        register_sfr(0x300, 2, &default_write<0xff>);
        register_sfr(0x310, 2, &default_write<0xff>);

    } else {
        register_sfr(0x20, 4, &default_write<0xff>);
        register_sfr(0x24, 1, &default_write<0xf>);
        register_sfr(0x25, 1, &default_write<1>);
    }
}

void SFRTimer::tick() {
    this->timer->tick();
    uint16_t counter, target;

    if (this->mcu->config->hardware_id == HW_TI_MATHPRINT) {
        counter = (this->mcu->sfr[0x301] << 8) | this->mcu->sfr[0x300];
        target = (this->mcu->sfr[0x311] << 8) | this->mcu->sfr[0x310];
        counter += this->timer->ticks;
        this->mcu->sfr[0x310] = (uint8_t)counter;
        this->mcu->sfr[0x311] = (uint8_t)(counter >> 8);
        if (counter >= target && this->mcu->standby->stop_mode) this->mcu->sfr[0x1d] |= 1;

    } else {
        if (this->mcu->sfr[0x25]) {
            uint16_t counter = (this->mcu->sfr[0x23] << 8) | this->mcu->sfr[0x22];
            uint16_t target = (this->mcu->sfr[0x21] << 8) | this->mcu->sfr[0x20];

            counter += this->timer->ticks;
            this->mcu->sfr[0x22] = (uint8_t)counter;
            this->mcu->sfr[0x23] = (uint8_t)(counter >> 8);

            if (counter >= target && this->mcu->standby->stop_mode) this->mcu->sfr[0x14] |= this->mcu->config->hardware_id == HW_SOLAR_II ? 2 : 0x20;
        }
    }
}

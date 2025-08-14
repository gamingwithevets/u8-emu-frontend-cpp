/*
    u8-emu-frontend-cpp Watchdog timer emulation
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
#pragma once

#include <cstdint>

#include "../mcu/mcu.hpp"
#include "../config/config.hpp"

class WDT {
    class MCU *mcu;
    struct config *config;

public:
    bool wdp;

    uint8_t wdtcon;
    size_t wdt_count;
    bool overflow_count;

    WDT(class MCU *mcu);
    void reset();
    void tick();
};

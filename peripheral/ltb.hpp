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
#pragma once

#include "../mcu/mcu.hpp"
#include "timer.hpp"

class LTB {
private:
    class MCU *mcu;
    class Timer *timer;
public:
    LTB(class MCU *mcu);
    void tick();
};

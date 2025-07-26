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
#pragma once

#include "../mcu/mcu.hpp"
#include "standby.hpp"

class timer {
private:
    double tps;
    uint64_t last_time;
public:
    int ticks;
    double time_scale;
    double passed_time;
    timer(double tps);
    void tick();
};

class sfrtimer {
private:
    class mcu *mcu;
    standby *standby;
    class timer *timer;
public:
    sfrtimer(class mcu *mcu);
    void tick();
};

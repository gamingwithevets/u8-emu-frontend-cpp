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

#include <iostream>
#include <map>
#include <cstdint>
#include <optional>

#include "../mcu/mcu.hpp"
#include "../config/config.hpp"

typedef struct {
    uint16_t vector_adrs;
    uint16_t ie_adrs;
    uint8_t  ie_bit;
    uint16_t irq_adrs;
    uint8_t  irq_bit;
} intr_data;

typedef struct {
    std::string interrupt_name;
    bool nmi;
} int_callstack;

class interrupts {
public:
    class mcu *mcu;
    struct config *config;
    std::map<std::string, intr_data> intr_tbl;
    char int_timer;
    interrupts(class mcu *mcu);
    int_callstack tick();
    std::optional<std::string> find_int(uint16_t vector_adrs);
};

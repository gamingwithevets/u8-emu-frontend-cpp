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
#include "mcu.hpp"

typedef struct {
    uint16_t addr;
    uint16_t len;
    std::string name;
    std::string desc;
} dldata;

typedef struct {
    uint16_t addr;
    uint16_t bbits;
    uint16_t wbits;
    std::string bsymbol0;
    std::string bsymbol1;
    std::string wsymbol;
    std::string name;
    std::string desc;
} sfrdata;

class dlabels {
    std::vector<dldata> ramlabels;
    std::vector<sfrdata> sfrlabels;
    std::vector<dldata> ram2labels;
public:
    dlabels(class mcu *mcu);
    void get_name(int type, uint16_t addr, dldata *data);
    void get_sfr_name(uint16_t addr, sfrdata *data);
};

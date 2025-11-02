/*
    u8-emu-frontend-cpp  disassembly window
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
#include <map>
#include <vector>
#include "../disas/disas.hpp"

inline int max_row = 0;
inline int cur_col = 0;
inline int first_col = 0;
inline bool need_roll = false;
inline std::vector<CodeElem> codes;
inline std::map<uint32_t, uint8_t> break_points;
inline uint32_t pc_cache = 0;
inline uint32_t selected_addr = -1;

CodeElem LookUp(uint32_t offset, int* idx);
void JumpTo(uint32_t offset);
bool TryTrigBP(uint8_t seg, uint16_t offset);
void drawdisas();

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

#include <iostream>
#include <algorithm>
#include "../imgui/imgui.h"
#include "../disas/disas.hpp"
#include "disasdisp.hpp"

CodeElem LookUp(uint32_t offset, int* idx) {
	auto it = std::find_if(
		codes.begin(), codes.end(), [&](const CodeElem& a) {
			return a.offset == offset && !a.is_label;
		});
	if (it == codes.end()) {
		it = codes.begin();
	}
	if (idx)
		*idx = it - codes.begin();
	return {.offset = it->offset};
}

void JumpTo(uint32_t offset) {
	int idx = 0;
	LookUp(offset, &idx);
	cur_col = idx+5;
	need_roll = true;
}

bool TryTrigBP(uint8_t seg, uint16_t offset) {
    for (auto it = break_points.begin(); it != break_points.end(); it++) {
        if (it->second == 1) {
            CodeElem e = codes[it->first];
            if (e.offset == (seg << 16) + offset) {
                break_points[it->first] = 2;
                cur_col = it->first + 5;
                need_roll = true;
                return true;
            }
        }
    }
    return false;
}

void drawdisas() {
	ImGuiListClipper c;
	c.Begin(max_row, ImGui::GetTextLineHeight());
	while (c.Step()) {
		first_col = c.DisplayStart;
		for (int line_i = c.DisplayStart; line_i < c.DisplayEnd; line_i++) {
			CodeElem e = codes[line_i];
			auto it = break_points.find(line_i);
			auto bb = it == break_points.end();
			if (!e.is_label) {
				if (e.offset == pc_cache) {
					ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), " > ");
				}
				else {
					if (bb) {
						ImGui::Text("   ");
						if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
							break_points[line_i] = 1;
						}
					}
					else {
						if (it->second == 1) {
							ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), " x ");
							if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
								break_points.erase(line_i);
							}
						}
						else {
							break_points.erase(line_i);
							ImGui::Text("   ");
						}
					}
				}
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0, 1.0, 0.0, 1.0), "%X:%04XH", (e.offset >> 16) & 0xf, e.offset & 0xfffe);
				ImGui::SameLine();
			}
			ImGui::PushID(line_i);
			if (ImGui::Selectable(e.srcbuf)) {
				if (e.xref_operand)
					JumpTo(e.xref_operand);
			}
			ImGui::PopID();
		}
	}
	if (need_roll) {
		float v = (float)cur_col / max_row * ImGui::GetScrollMaxY();
		auto origv = ImGui::GetScrollY();
		if (v < origv || (v - origv > (ImGui::GetWindowHeight() - 200))) {
			ImGui::SetScrollY(v);
		}
		need_roll = false;
		selected_addr = codes[cur_col].offset;
	}
}

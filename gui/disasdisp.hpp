#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include "../imgui/imgui.h"
#include "../disas/disas.hpp"

int max_row = 0;
int cur_col = 0;
int first_col = 0;
bool need_roll = false;
std::vector<CodeElem> codes;
std::map<int, uint8_t> break_points;
uint32_t pc_cache = 0;
uint32_t selected_addr = -1;

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
	// printf("jumpto:seg%d\n",seg);
	LookUp(offset, &idx);
	cur_col = idx+5;
	need_roll = true;
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

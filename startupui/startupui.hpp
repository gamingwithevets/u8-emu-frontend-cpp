#pragma once
#include <iostream>
#include "../imgui/imgui.h"

class UIWindow {
public:
	UIWindow(const char* name) : name(name) {}
	const char* name{};
	bool open = true;
	ImVec2 inital_size{800, 800};
	ImGuiWindowFlags flags{};
	virtual void Render() {
		if (!open)
			return;
		ImGui::SetNextWindowSize(inital_size, ImGuiCond_FirstUseEver);
		if (ImGui::Begin(name, &open, flags)) {
			RenderCore();
		}
		ImGui::End();
	}
	virtual void RenderCore() = 0;
};
constexpr ImGuiTableFlags pretty_table = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Reorderable ;

std::string sui_loop();
char get_pmode(uint8_t value);

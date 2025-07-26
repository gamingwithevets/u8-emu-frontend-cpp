#pragma once
#include <iostream>
#include "../imgui/imgui.h"


constexpr ImGuiTableFlags pretty_table = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Reorderable;

std::string sui_loop();
char get_pmode(uint8_t value);

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
#include "settings.hpp"

#include <cstdio>
#include <cstring>
#include <sstream>
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

static const char *languages[] = {
    "English",
    "Vietnamese"
};

const char* Lang_ToString(Language lang) {
    int index = (int)lang;
    if (index >= 0 && index < (int)Language::Count)
        return languages[index];
    return "English";
}

Language ParseLang(const char *str) {
    for (int i = 0; i < (int)Language::Count; ++i) {
        if (strcmp(str, languages[i]) == 0)
            return static_cast<Language>(i);
    }
    return Language::English;
}

settings g_settings;

static const char *SettingsTypeName = "u8-emu-frontend-cpp";

static void* Settings_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name) {
    return (strcmp(name, "Settings") == 0) ? &g_settings : nullptr;
}

static void Settings_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line) {
    settings* s = (settings*)entry;
    char key[64], value[256];

    if (sscanf(line, "%63[^=]=%255s", key, value) == 2) {
        if (strcmp(key, "Language") == 0) s->lang = ParseLang(value);
    }
}

static void Settings_WriteAll(ImGuiContext*, ImGuiSettingsHandler*, ImGuiTextBuffer* out_buf) {
    const settings& s = g_settings;
    out_buf->appendf("[%s][Settings]\n", SettingsTypeName);
    out_buf->appendf("Language=%s\n", Lang_ToString(s.lang));
    out_buf->appendf("\n");
}

void register_settings_handler() {
    ImGuiSettingsHandler handler;
    handler.TypeName = SettingsTypeName;
    handler.TypeHash = ImHashStr(SettingsTypeName);
    handler.ReadOpenFn = Settings_ReadOpen;
    handler.ReadLineFn = Settings_ReadLine;
    handler.WriteAllFn = Settings_WriteAll;
    ImGui::GetCurrentContext()->SettingsHandlers.push_back(handler);
}

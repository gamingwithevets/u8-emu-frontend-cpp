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

#include <map>
#include <vector>
#include "settings.hpp"
#include "../imgui/imgui.h"

const std::vector<std::string> lang_names = {
    "English",
    "Tiếng Việt"
};

enum StringID {
    s_startupui_modelselect,
    s_startupui_desc,
    s_startupui_recent,
    s_startupui_all,
    s_startupui_show,
    s_startupui_show_real,
    s_startupui_show_emu,
    s_startupui_found0,
    s_startupui_found1,
    s_startupui_notfound,
    s_startupui_window_title,
    s_startupui_cfg_path,
    s_startupui_rom_version,
    s_startupui_rom_checksum,
    s_startupui_type,
    s_startupui_emu_tag,
    s_language,
    s_options,
    s_options_mcu,
    s_options_mcu_reset,
    s_options_mcu_pmode,
    s_options_mcu_pause,
    s_options_mcu_step,
    s_options_mcu_resetfull,
    s_options_interface,
    s_options_other,
    s_options_other_copyclip,
    s_options_other_copyclip_success,
    s_options_other_copyclip_fail,
    s_options_about,
    s_options_mcu_cps,
};

const std::map<Language, std::vector<std::string>> loc = {
    {
        Language::English, {
            "Model Select",
            "Choose a model to run.",
            "Recently Used",
            "All",
            "Show:",
            "Real",
            "Emulator",
            "Found ",
            " model(s).",
            "No models found.",
            "Window title",
            "Config file path",
            "ROM version",
            "ROM checksum",
            "Type",
            " (E)",
            "Language",
            "Options",
            "MCU Control",
            "Reset core",
            "P mode",
            "Pause/Single-step",
            "Step",
            "Wipe RAM and reset",
            "Interface",
            "Other",
            "Copy screen to clipboard",
            "Screen copied to clipboard!",
            "Oops! An error occurred.",
            "About",
            "Cycles per second (KiHz, set to -1 for no speed cap)",
        }
    },
    {
        Language::Vietnamese, {
            "Chọn model",
            "Vui lòng chọn một model để sử dụng.",
            "Sử dụng gần đây",
            "Tất cả",
            "Hiển thị:",
            "Thật",
            "Giả lập",
            "Đã tìm thấy ",
            " model.",
            "Không tìm thấy model.",
            "Tiêu đề cửa sổ",
            "Đường dẫn tệp cấu hình",
            "Phiên bản ROM",
            "Giá trị tổng kiểm",
            "Loại",
            " (GL)",
            "Ngôn ngữ",
            "Cài đặt",
            "Vi điều khiển",
            "Khởi động lại",
            "Chế độ P",
            "Tạm dừng/Bước đơn",
            "Bước",
            "Xóa RAM và khởi động lại",
            "Giao diện",
            "Khác",
            "Sao chép màn hình vào bảng ghi tạm",
            "Sao chép thành công!",
            "Đã xảy ra lỗi khi sao chép",
            "Về giả lập (tiếng Anh)",
            "Chu kỳ trên giây (KiHz, kéo xuống -1 để tắt giới hạn tốc độ)",
        }
    },
};

inline const char *get_langname(Language lang) {
    return lang_names.at((int)lang).c_str();
}

inline const char *get_strloc(StringID id) {
    Language lang = id < loc.at(g_settings.lang).size() ? g_settings.lang : Language::English;
    return loc.at(lang).at(id).c_str();
}

inline ImFont *set_font() {
    ImGuiIO &io = ImGui::GetIO();
    return io.Fonts->AddFontFromFileTTF("fonts/consola.ttf", 13);
}

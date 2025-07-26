#include <SDL3/SDL.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3_image/SDL_image.h>
#include <array>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <bitset>
#include <cstdint>

#include <sys/stat.h>

#include "startupui.hpp"
#include "../config/config.hpp"
#include "../config/settings.hpp"
#include "../config/lang.hpp"
#include "rominfo.hpp"
#include "../global.hpp"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_sdl3.h"
#include "../imgui/imgui_impl_sdlrenderer3.h"

inline SDL_Window* window;
inline SDL_Renderer* renderer;

std::map<hardware_id, std::string> hwid_names = {
    {HW_SOLAR_II, "SOLAR II"},
    {HW_ES, "ES"},
    {HW_ES_PLUS, "ES PLUS"},
    {HW_CLASSWIZ_EX, "ClassWiz EX"},
    {HW_CLASSWIZ_CW, "ClassWiz CW"},
    {HW_TI_MATHPRINT, "TI MathPrint"},
};
const std::string esp2_str = "ES PLUS 2nd edition";
const std::string fx5800p_str = "ES (fx-5800P)";

// not generated with ChatGPT
char get_pmode(uint8_t value) {
    if (value == 0) {
        return '-'; // Return '-' if the value is 0
    }

    // Check if more than one bit is set
    std::bitset<8> bits(value);
    if (bits.count() > 1) {
        return '?'; // Return '?' if multiple bits are set
    }

    // Find the index of the rightmost set bit
    for (uint8_t i = 0; i < 8; ++i) {
        if (value & (1 << i)) {
            return static_cast<char>(i + 0x30);
        }
    }

    return '-'; // Fallback return, although it should never reach here
}

class StartupUI {
public:
    struct Model {
    public:
        std::filesystem::path path;
        std::string name;
        std::string version;
        std::string type;
        std::string id;
        std::string checksum;
        std::string checksum2;
        std::string sum_ok;
        bool realhw;
        uint8_t pd_value;
        bool show_sum = true;
    };

    bool ok;
    std::vector<Model> models;
    std::filesystem::path selected_path{};
    std::vector<std::string> recently_used{};
    char search_txt[200]{};
    const char* current_filter = "##";
    bool show_real = true;
    bool show_emu = true;

    StartupUI() {
        struct stat info;
        if (!stat("configs", &info)) {
            if (info.st_mode & S_IFDIR) goto notfail;
            else goto fail;
        } else
fail:
            this->ok = false;
            std::cout << "No 'configs' directory found. Please read the README for more details." << std::endl << "Press any key to exit." << std::endl;
            std::cin.ignore();
            return;
notfail:
        this->ok = true;
        for (auto& dir : std::filesystem::directory_iterator("configs")) {
            if (dir.is_regular_file() && ends_with(dir.path().string(), std::string(".bin"))) {
                auto config = dir.path().string();
                std::ifstream ifs(config, std::ios::in | std::ios::binary);
                if (!ifs) {
                    std::cout << dir.path().string() << ": Could not open config file." << std::endl;
                    continue;
                }
                struct config mi{};
                Binary::Read(ifs, mi);
                ifs.close();
                Model mod{};
                mod.path = dir;
                mod.name = mi.w_name;
                mod.realhw = mi.real_hardware;
                mod.type = hwid_names.find(mi.hardware_id) == hwid_names.end() ? "Unknown" : hwid_names[mi.hardware_id];
                std::ifstream ifs2(mi.rom_file, std::ios::in | std::ios::binary);
                if (!ifs2) {
                    std::cout << dir.path().string() << ": Could not open ROM." << std::endl;
                    continue;
                }
                std::vector<byte> rom{std::istreambuf_iterator<char>{ifs2.rdbuf()}, std::istreambuf_iterator<char>{}};
                ifs2.close();
                std::vector<byte> flash;
                if (!mi.flash_rom_file.empty()) {
                    std::ifstream ifs3(mi.flash_rom_file, std::ios::in | std::ios::binary);
                    if (ifs3) flash = {std::istreambuf_iterator<char>{ifs3.rdbuf()}, std::istreambuf_iterator<char>{}};
                    ifs3.close();
                }
                auto ri = rom_info(rom, flash, mi.real_hardware);
                if (ri.type != 0) {
                    switch (ri.type) {
                    case RomInfo::ES:
                        mod.type = hwid_names[HW_ES];
                        break;
                    case RomInfo::ESP:
                        mod.type = hwid_names[HW_ES_PLUS];
                        break;
                    case RomInfo::ESP2nd:
                        mod.type = esp2_str;
                        break;
                    case RomInfo::CWX:
                        mod.type = hwid_names[HW_CLASSWIZ_EX];
                        break;
                    case RomInfo::CWII:
                        mod.type = hwid_names[HW_CLASSWIZ_CW];
                        break;
                    case RomInfo::Fx5800p:
                        mod.type = fx5800p_str;
                        break;
                    case RomInfo::TI:
                        mod.type = hwid_names[HW_TI_MATHPRINT];
                        break;
                    }
                }
                mod.version = ri.ver;
                mod.pd_value = mi.pd_value;
                if (ri.ok) {
                    mod.checksum = _tohex(ri.real_sum, 4);
                    mod.checksum2 = _tohex(ri.desired_sum, 4);
                    mod.sum_ok = ri.real_sum == ri.desired_sum ? "OK" : "NG";
                    mod.id = _tohex(*(unsigned long long*)ri.cid, 8);
                }
                else {
                    mod.show_sum = false;
                }
                // SDL_Surface* loaded_surface = IMG_Load((dir.path() / mi.interface_path).string().c_str());
                // mod.thumbnail_t = scale_texture_region_uniform(renderer2, loaded_surface, &mi.sprites["rsd_interface"].src, 100, 200);
                models.push_back(mod);
            }
        }
    }

    void Render() {
        auto io = ImGui::GetIO();
        ImGui::SetNextWindowSize({io.DisplaySize.x, io.DisplaySize.y});
        ImGui::SetNextWindowPos({});
        char label[200];
        sprintf(label, "%s###Model Select", get_strloc(s_startupui_modelselect));
        ImGui::Begin(label, 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
        ImGui::Text(get_strloc(s_startupui_desc));
        ImGui::SameLine(ImGui::GetWindowWidth() - 300);
        ImGui::Text(get_strloc(s_language));
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::BeginCombo("##langselect", get_langname(g_settings.lang))) {
            for (int n = 0; n < (int)Language::Count; n++) {
                const bool is_selected = (g_settings.lang == (Language)n);
                if (ImGui::Selectable(get_langname((Language)n), is_selected)) g_settings.lang = (Language)n;
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::Separator();
        ImGui::Text(get_strloc(s_startupui_recent));
        if (ImGui::BeginTable("Recent", 5, pretty_table)) {
            RenderHeaders();
            auto i = 114;
            for (auto& s : recently_used) {
                auto iter = std::find_if(models.begin(), models.end(), [&](const Model& x) {
                    return x.path == s;
                });
                if (iter != models.end()) {
                    auto& model = *iter;
                    RenderModel(model, i);
                }
            }
            ImGui::EndTable();
        }
        char all_label[50];
        sprintf(all_label, "%s###All", get_strloc(s_startupui_all));
        if (ImGui::CollapsingHeader(all_label)) {
            ImGui::SetNextItemWidth(200);
            ImGui::InputText("##search", search_txt, 200);
            ImGui::SameLine();
            const char* items[] = {"##", hwid_names[HW_ES].c_str(), fx5800p_str.c_str(), hwid_names[HW_ES_PLUS].c_str(), esp2_str.c_str(), hwid_names[HW_CLASSWIZ_EX].c_str(), hwid_names[HW_CLASSWIZ_CW].c_str()};
            ImGui::SetNextItemWidth(80);
            if (ImGui::BeginCombo("##cb", current_filter)) {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
                    bool is_selected = (current_filter == items[n]);
                    if (ImGui::Selectable(items[n], is_selected)) current_filter = items[n];
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::Text(get_strloc(s_startupui_show));
            ImGui::SameLine();
            ImGui::Checkbox(get_strloc(s_startupui_show_real), &show_real);
            ImGui::SameLine();
            ImGui::Checkbox(get_strloc(s_startupui_show_emu), &show_emu);
            auto i = 200;
            if (ImGui::BeginTable("All", 5, pretty_table)) {
                RenderHeaders();
                for (auto& model : models) {
                    bool matches_filter = (!strcmp(current_filter, "##")) || (current_filter == model.type);
                    bool matches_search = (stristr(model.name.c_str(), search_txt) != nullptr || stristr(model.version.c_str(), search_txt) != nullptr);
                    bool matches_type = false;
                    if (show_real && model.realhw) matches_type = true;
                    else if (show_emu && !model.realhw) matches_type = true;
                    if (matches_filter && matches_search && matches_type) {
                        RenderModel(model, i);
                    }
                }
                ImGui::EndTable();
            }
            if (i > 200) ImGui::Text("%s%d%s", get_strloc(s_startupui_found0), i-200, get_strloc(s_startupui_found1));
            else ImGui::Text(get_strloc(s_startupui_notfound));
        }
        ImGui::End();
    }
    void RenderHeaders() {
        ImGui::TableSetupColumn(get_strloc(s_startupui_window_title), ImGuiTableColumnFlags_WidthStretch, 200);
        ImGui::TableSetupColumn(get_strloc(s_startupui_cfg_path), ImGuiTableColumnFlags_WidthStretch, 130);
        ImGui::TableSetupColumn(get_strloc(s_startupui_rom_version), ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn(get_strloc(s_startupui_rom_checksum), ImGuiTableColumnFlags_WidthFixed, 130);
        ImGui::TableSetupColumn(get_strloc(s_startupui_type), ImGuiTableColumnFlags_WidthFixed, 170);
        ImGui::TableHeadersRow();
    }
    void RenderModel(const Model& model, int& i) {
        ImGui::TableNextRow();
        ImGui::PushID(i++);
        ImGui::TableNextColumn();
        if (ImGui::Selectable(model.name.c_str())) {
            selected_path = model.path;
            auto iter = std::find_if(recently_used.begin(), recently_used.end(),
                [&](auto& x) {
                    return x == model.path.string();
                });
            if (iter != recently_used.end())
                recently_used.erase(iter);
            recently_used.insert(recently_used.begin(), model.path.string());
            if (recently_used.size() > 5) {
                recently_used.resize(5);
            }
        }
        ImGui::TableNextColumn();
        ImGui::Text(model.path.string().c_str());
        ImGui::TableNextColumn();
        if (model.version.size()) {
            if (model.type == hwid_names[HW_ES] || model.type == fx5800p_str) ImGui::Text("%s (P%c)", model.version.c_str(), get_pmode(model.pd_value));
            else if (model.type == hwid_names[HW_TI_MATHPRINT]) ImGui::Text(model.version.c_str());
            else ImGui::Text("%s %s%s", model.version.substr(0, 6).c_str(), !strcmp(model.type.c_str(),  hwid_names[HW_CLASSWIZ_CW].c_str()) ? "V." : "Ver", model.version.substr(6, 7).c_str());
        } else ImGui::Text("-");
        ImGui::TableNextColumn();
        if (model.realhw && model.show_sum)
            if (model.checksum == model.checksum2) ImGui::Text("%s %s", model.checksum.c_str(), model.sum_ok.c_str());
            else ImGui::Text("%s (%s) %s", model.checksum.c_str(), model.checksum2.c_str(), model.sum_ok.c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%s%s", model.type.c_str(), model.realhw ? "" : get_strloc(s_startupui_emu_tag));
        ImGui::PopID();
    }
};

std::string sui_loop() {
	StartupUI ui;
	if (!ui.ok) return {};
	{
		std::ifstream ifs1{"recent.bin", std::ifstream::binary};
		if (ifs1)
			Binary::Read(ifs1, ui.recently_used);
	}
	window = SDL_CreateWindow("u8-emu-frontend-cpp", 1280, 720, SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, NULL);
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	if (renderer == nullptr) {
		SDL_Log("Error creating SDL_Renderer!");
		return "";
	}
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.WantCaptureKeyboard = true;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImFont *font = set_font();
	ImGui::StyleColorsDark();

	register_settings_handler();

	ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);
	while (1) {
		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
		ImGui::PushFont(font);
		ui.Render();
		ImGui::PopFont();
		ImGui::EndFrame();
		ImGui::Render();
		SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
		SDL_RenderPresent(renderer);
		SDL_Event event;
		if (!SDL_PollEvent(&event)) continue;
		bool brk = false;
		switch (event.type) {
            case SDL_EVENT_QUIT:
                brk = true;
                break;
            default:
                ImGui_ImplSDL3_ProcessEvent(&event);
                break;
		}
        if (brk) break;
		if (!ui.selected_path.empty()) break;
	}
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	std::ofstream ofs{"recent.bin", std::ofstream::binary};
	if (ofs) Binary::Write(ofs, ui.recently_used);
	else std::cout << "[Startup] Could not write to recent.bin." << std::endl;
	return ui.selected_path.string();
}

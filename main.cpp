#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <thread>
#include <atomic>

#include "mcu/mcu.hpp"
#include "config/config.hpp"
#include "config/binary.hpp"
#include "startupui/startupui.hpp"
extern "C" {
#include "u8_emu/src/core/core.h"
#include "nxu8_disas/src/lib/lib_nxu8.h"
}
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "imgui/imgui_memory_editor.h"

//#define CONFIGDEBUG

const std::map<SDL_Scancode, SDL_Keycode> shift_keycombos = {
    {SDL_SCANCODE_1, SDLK_EXCLAIM},
    {SDL_SCANCODE_2, SDLK_AT},
    {SDL_SCANCODE_3, SDLK_HASH},
    {SDL_SCANCODE_4, SDLK_DOLLAR},
    {SDL_SCANCODE_5, SDLK_PERCENT},
    {SDL_SCANCODE_6, SDLK_CARET},
    {SDL_SCANCODE_7, SDLK_AMPERSAND},
    {SDL_SCANCODE_8, SDLK_ASTERISK},
    {SDL_SCANCODE_9, SDLK_LEFTPAREN},
    {SDL_SCANCODE_0, SDLK_RIGHTPAREN},
    {SDL_SCANCODE_MINUS, SDLK_UNDERSCORE},
    {SDL_SCANCODE_EQUALS, SDLK_PLUS},
    {SDL_SCANCODE_SEMICOLON, SDLK_COLON},
    {SDL_SCANCODE_APOSTROPHE, SDLK_QUOTEDBL},
    {SDL_SCANCODE_COMMA, SDLK_LESS},
    {SDL_SCANCODE_PERIOD, SDLK_GREATER},
    {SDL_SCANCODE_SLASH, SDLK_QUESTION},
};

void convert_shift(SDL_Event &event) {
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
            auto it = shift_keycombos.find(event.key.keysym.scancode);
            if (it != shift_keycombos.end()) {
                event.key.keysym.sym = it->second;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::string path;
    if (argc < 2) {
        path = sui_loop();
        if (path.empty()) return 0;
    } else if (argc > 2) {
        std::cerr << "Too many arguments" << std::endl;
        return 0;
    }
    else path = std::string(argv[1]);

    std::ifstream is(path.c_str(), std::ifstream::binary);
    if (!is) {
        std::cerr << "Error loading config file '" << path << "': " << strerror(errno) << std::endl;
        return -1;
    }
    config config{};
    try {
        Binary::Read(is, config);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config file '" << path << "': " << e.what() << std::endl;
        return -1;
    }

#ifdef CONFIGDEBUG
    std::cout << "===== DEBUG =====" << std::endl;
    std::cout << "rom_file: " << config.rom_file << std::endl;
    std::cout << "flash_rom_file: " << config.flash_rom_file << std::endl;
    std::cout << "hardware_id: " << config.hardware_id << std::endl;
    std::cout << "real_hardware: " << (config.real_hardware ? "True" : "False") << std::endl;
    std::cout << "sample: " << (config.sample ? "True" : "False") << std::endl;
    std::cout << "is_5800p: " << (config.is_5800p ? "True" : "False") << std::endl;
    std::cout << "old_esp: " << (config.old_esp ? "True" : "False") << std::endl;
    std::cout << "pd_value: " << +config.pd_value << std::endl;
    std::cout << "status_bar_path: " << config.status_bar_path << std::endl;
    std::cout << "interface_path: " << config.interface_path << std::endl;
    std::cout << "w_name: " << config.w_name << std::endl;
    std::cout << "screen_tl_w: " << config.screen_tl_w << std::endl;
    std::cout << "screen_tl_h: " << config.screen_tl_h << std::endl;
    std::cout << "pix_w: " << config.pix_w << std::endl;
    std::cout << "pix_h: " << config.pix_h << std::endl;
    std::cout << "pix_color: (" << +config.pix_color.r << ", " << +config.pix_color.g << ", " << +config.pix_color.b << ")" << std::endl;
    std::cout << "status_bar_crops:" << std::endl;
    for (auto const &i : config.status_bar_crops)
        std::cout << "  (" << i.x << ", " << i.y << ", " << i.w << ", " << i.h << ")" << std::endl;
    std::cout << "keymap:" << std::endl;
    for (auto const &[k, v] : config.keymap) {
        std::cout << "  " << std::hex << std::showbase << +k << std::dec << ":"
                  << "    (" << v.rect.x << ", " << v.rect.y << ", " << v.rect.w << ", " << v.rect.h << ")";
        for (auto const &i : v.keys)
            if (i != SDLK_UNKNOWN) std::cout << ", " << SDL_GetKeyName(i);

        std::cout << std::endl;
    }
    std::cin.ignore();
    std::cin.get();
    return 0;
#endif

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL. SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }
    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
        std::cerr << "Failed to initialize SDL_image. SDL_image Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Window* window2 = SDL_CreateWindow("Debugger", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, SDL_WINDOW_SHOWN);
    if (!window2) {
        std::cerr << "Failed to create debugger window. SDL_Error: " << SDL_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer2 = SDL_CreateRenderer(window2, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer2) {
        std::cerr << "Failed to create debugger renderer. SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window2);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Surface* interface_sf = IMG_Load(config.interface_path.c_str());
    if (interface_sf == nullptr) {
        std::cerr << "Failed to load interface image. SDL_image Error: " << IMG_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    int w = interface_sf->w;
    int h = interface_sf->h;

    SDL_Window* window = SDL_CreateWindow(!config.w_name.empty() ? config.w_name.c_str() : "u8-emu-frontend-cpp", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Failed to create window. SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window2);
        SDL_FreeSurface(interface_sf);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create renderer. SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window2);
        SDL_DestroyWindow(window);
        SDL_FreeSurface(interface_sf);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Texture* interface = SDL_CreateTextureFromSurface(renderer, interface_sf);
    SDL_FreeSurface(interface_sf);

    u8_core core{};

    uint8_t *rom = (uint8_t *)malloc(0x100000);
    memset((void *)rom, 0xff, 0x100000);
    FILE *f = fopen(config.rom_file.c_str(), "rb");
    if (!f) {
        std::cout << "Cannot open ROM!" << std::endl;
        return -1;
    }
    fread(rom, sizeof(uint8_t), 0x20000, f);
    fclose(f);

    std::cout << "Generating disassembly..." << std::endl;
    struct nxu8_decoder *decoder = nxu8_init_decoder(0x20000, rom);
    std::map<uint32_t, std::string> disas;
    uint32_t addr = 0;
	while (addr < decoder->buf_sz) {
        char tmp[30];
        int len;
		struct nxu8_instr *instr = nxu8_decode_instr(decoder, addr);
		if (instr != NULL) {
			sprintf(tmp, "%s", instr->assembly);
			len = instr->len;
		} else {
			uint16_t val = nxu8_read16(decoder, addr);
			sprintf(tmp, "DW %04XH", val);
			len = 2;
		}
        std::string tmp2(tmp);
		disas[addr] = tmp2;
		addr += len;
	}
    std::cout << "Done!" << std::endl;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window2, renderer2);
    ImGui_ImplSDLRenderer2_Init(renderer2);

    int ramstart, ramsize;
    switch (config.hardware_id) {
    case HW_SOLAR_II:
        ramstart = 0xe000;
        ramsize = 0x1000;
        break;
    case HW_CLASSWIZ_EX:
        ramstart = 0xd000;
        ramsize = 0x2000;
        break;
    case HW_CLASSWIZ_CW:
        ramstart = 0x9000;
        ramsize = 0x6000;
    case HW_TI_MATHPRINT:
        ramstart = 0xb000;
        ramsize = 0x4000;
    default:
        ramstart = 0x8000;
        ramsize = config.real_hardware ? 0xe00 : 0x7000;
        break;
    }
    mcu mcu(&core, &config, rom, NULL, ramstart, ramsize, w, h);

    bool quit = false;
    bool single_step = false;
    std::atomic<bool> stop = false;
    static MemoryEditor ramedit;
    static MemoryEditor sfredit;
    sfredit.ReadFn = &read_sfr_im;
    sfredit.WriteFn = &write_sfr_im;

    bool set_p[8];

    const char *memselect[] = {"Main RAM", "SFR region"};
    static int memselect_idx = 0;

    unsigned int a, b;
    double fps;

    std::thread cs_thread(core_step_loop, std::ref(stop));
    SDL_Event e;
    while (!quit) {
        a = SDL_GetTicks();

        while (SDL_PollEvent(&e) != 0) {
            convert_shift(e);
            if (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) mcu.keyboard->process_event(&e);
            if (SDL_GetWindowFlags(window2) & SDL_WINDOW_INPUT_FOCUS) ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) quit = true;
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) quit = true;
        }

        if (single_step && !stop.load())
            if (cs_thread.joinable()) {
                stop = true;
                cs_thread.join();
            }
        if (!single_step && stop.load()) cs_thread = std::thread(core_step_loop, std::ref(stop));

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Register Display", NULL, 0);
        ImGui::Text("General registers:");
        if (ImGui::BeginTable("gp0", 8)) {
            for (int i = 0; i < 8; i++)
                ImGui::TableSetupColumn(("R" + std::to_string(i)).c_str());
            ImGui::TableHeadersRow();
            ImGui::TableNextRow();
            for (int i = 0; i < 8; i++)
            {
                ImGui::TableSetColumnIndex(i);
                ImGui::Text("%02X", mcu.core->regs.gp[i]);
            }
            ImGui::EndTable();
        }
        if (ImGui::BeginTable("gp1", 8)) {
            for (int i = 0; i < 8; i++)
                ImGui::TableSetupColumn(("R" + std::to_string(i+8)).c_str());
            ImGui::TableHeadersRow();
            ImGui::TableNextRow();
            for (int i = 0; i < 8; i++)
            {
                ImGui::TableSetColumnIndex(i);
                ImGui::Text("%02X", mcu.core->regs.gp[i+8]);
            }
            ImGui::EndTable();
        }
        ImGui::Text("\nControl registers:");
        if (ImGui::BeginTable("ctrl", 2, ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Register");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("CSR:PC");
            ImGui::TableNextColumn();
            ImGui::Text("%X:%04XH", mcu.core->regs.csr, mcu.core->regs.pc);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("SP");
            ImGui::TableNextColumn();
            ImGui::Text("%04XH", mcu.core->regs.sp);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("DSR:EA");
            ImGui::TableNextColumn();
            ImGui::Text("%02X:%04XH", mcu.core->regs.dsr, mcu.core->regs.ea);

            ImGui::EndTable();
        }
        ImGui::Text("\n");
        if (ImGui::BeginTable("psw", 2, ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("PSW");
            char val[2]; sprintf(val, "%02X", mcu.core->regs.psw);
            ImGui::TableSetupColumn(val);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("C");
            ImGui::TableNextColumn();
            ImGui::Text("[%s]", (mcu.core->regs.psw & (1 << 7)) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Z");
            ImGui::TableNextColumn();
            ImGui::Text("[%s]", (mcu.core->regs.psw & (1 << 6)) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("S");
            ImGui::TableNextColumn();
            ImGui::Text("[%s]", (mcu.core->regs.psw & (1 << 5)) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("OV");
            ImGui::TableNextColumn();
            ImGui::Text("[%s]", (mcu.core->regs.psw & (1 << 4)) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("MIE");
            ImGui::TableNextColumn();
            ImGui::Text("[%s]", (mcu.core->regs.psw & (1 << 3)) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("HC");
            ImGui::TableNextColumn();
            ImGui::Text("[%s]", (mcu.core->regs.psw & (1 << 2)) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("ELEVEL");
            ImGui::TableNextColumn();
            ImGui::Text("%d", mcu.core->regs.psw & 3);

            ImGui::EndTable();
        }
        ImGui::Text("\nBackup registers:");
        if (ImGui::BeginTable("backup", 2, ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Register");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("LCSR:LR");
            ImGui::TableNextColumn();
            ImGui::Text("%X:%04XH", mcu.core->regs.lcsr, mcu.core->regs.lr);

            for (int i = 0; i < 2; i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("ECSR%d:ELR%d", i+1, i+1);
                ImGui::TableNextColumn();
                ImGui::Text("%X:%04XH", mcu.core->regs.ecsr[i], mcu.core->regs.elr[i]);
            }

            for (int i = 0; i < 2; i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("EPSW%d", i+1);
                ImGui::TableNextColumn();
                ImGui::Text("%02X", mcu.core->regs.epsw[i]);
            }

            ImGui::EndTable();
        }
        ImGui::Text("\nOther information:");
        if (ImGui::BeginTable("other", 2, ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Description");
            ImGui::TableSetupColumn("State/Value");
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("STOP acceptor state");
            ImGui::TableNextColumn();
            ImGui::Text("1 [%s]  2 [%s]", (mcu.standby->stop_accept[0]) ? "x" : " ", (mcu.standby->stop_accept[1]) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("STOP mode");
            ImGui::TableNextColumn();
            ImGui::Text("[%s]", (mcu.standby->stop_mode) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("SDL frames per second");
            ImGui::TableNextColumn();
            ImGui::Text("%.1f FPS", fps);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Instructions per second");
            ImGui::TableNextColumn();
            if (single_step) ImGui::Text("[Single-step enabled.]");
            else if (mcu.standby->stop_mode) ImGui::Text("[In STOP mode.]");
            else ImGui::Text("%.1f IPS", mcu.ips);

            ImGui::EndTable();
        }
        ImGui::End();

        ImGui::Begin("Disassembly", NULL, 0);
        if (ImGui::BeginTable("disas", 2)) {
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Instruction");
            ImGui::TableHeadersRow();
            for (const auto& [k, v] : disas) {
                ImVec4 color = (k == ((mcu.core->regs.csr << 16) | (mcu.core->regs.pc))) ? (ImVec4)ImColor(255, 216, 0) : (ImVec4)ImColor(255, 255, 255);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(color, "%X:%04XH", k >> 16, k & 0xffff);
                ImGui::TableNextColumn();
                ImGui::TextColored(color, v.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::End();

        ImGui::Begin("Call Stack Display", NULL, 0);
        if (ImGui::BeginTable("callstack", 4)) {
            ImGui::TableSetupColumn(NULL);
            ImGui::TableSetupColumn("Function address");
            ImGui::TableSetupColumn("Return address");
            ImGui::TableSetupColumn("LR pushed at");
            ImGui::TableHeadersRow();
            for (int i = mcu.call_stack.size() - 1; i >= 0; i--) {
                int j = abs((int)mcu.call_stack.size() - i - 1);
                auto v = mcu.call_stack[i];
                uint32_t return_addr_real = read_mem_data(mcu.core, 0, v.return_addr_ptr, 4) & 0xfffff;
                ImVec4 color = (!j) ? (ImVec4)ImColor(255, 216, 0) : (ImVec4)ImColor(255, 255, 255);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(color, "#%d", j);
                ImGui::TableNextColumn();
                ImGui::TextColored(color, "%X:%04XH", v.func_addr >> 16, v.func_addr & 0xffff);
                ImGui::TableNextColumn();
                if (v.return_addr_ptr) {
                    ImGui::TextColored(color, "%X:%04XH (%X:%04XH)", return_addr_real >> 16, return_addr_real & 0xffff, v.return_addr >> 16, v.return_addr & 0xffff);
                    ImGui::TableNextColumn();
                    ImGui::TextColored(color, "%04XH", v.return_addr_ptr);
                } else ImGui::TextColored(color, "%X:%04XH", v.return_addr >> 16, v.return_addr & 0xffff);
            }
            ImGui::EndTable();
        }
        ImGui::End();

        ImGui::Begin("Hex Editor", NULL, 0);
        const char* preview = memselect[memselect_idx];
        if (ImGui::BeginCombo("Region", preview)) {
            for (int n = 0; n < IM_ARRAYSIZE(memselect); n++) {
                const bool is_selected = (memselect_idx == n);
                if (ImGui::Selectable(memselect[n], is_selected))
                    memselect_idx = n;
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (memselect_idx == 0) ramedit.DrawContents((void *)mcu.ram, ramsize, ramstart);
        else if (memselect_idx == 1) sfredit.DrawContents((void *)mcu.sfr, 0x1000, 0xf000);
        ImGui::End();

        ImGui::Begin("Options", NULL, 0);
        ImGui::Text("P mode");
        for (int i = 7; i >= 0; i--) {
            ImGui::SameLine();
            ImGui::Checkbox("##", &set_p[i]);
        }

        ImGui::Checkbox("Pause/Single-step", &single_step);
        if (single_step) {
            if (ImGui::Button("Step")) mcu.core_step();
        }
        ImGui::End();

        ImGui::Render();

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, interface, NULL, NULL);
        mcu.screen->render(renderer);
        mcu.keyboard->render(renderer);
        SDL_RenderPresent(renderer);

        SDL_RenderClear(renderer2);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer2);
        SDL_RenderPresent(renderer2);

        b = SDL_GetTicks() - a;
        fps = (b > 0) ? 1000.0f / b : 0.0f;
    }

    if (!single_step && cs_thread.joinable()) {
        stop = true;
        cs_thread.join();
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    mcu.~mcu();
    free((void *)rom);
    SDL_DestroyTexture(interface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyRenderer(renderer2);
    SDL_DestroyWindow(window);
    SDL_DestroyWindow(window2);
    IMG_Quit();
    SDL_Quit();

    std::exit(0);
}

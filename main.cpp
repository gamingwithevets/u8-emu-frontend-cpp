#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <thread>
#include <atomic>
#include <optional>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "mcu/mcu.hpp"
#include "config/config.hpp"
#include "config/binary.hpp"
#include "startupui/startupui.hpp"
#include "labeltool/labeltool.hpp"
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
            if (it != shift_keycombos.end()) event.key.keysym.sym = it->second;
        }
    }
}

std::map<uint32_t, std::string> disassemble(size_t romsize, uint8_t *rom, uint32_t start_addr) {
    struct nxu8_decoder *decoder = nxu8_init_decoder(romsize, rom);
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
        disas[addr+start_addr] = tmp2;
        addr += len;
    }
    return disas;
}

// Find the nearest number in a map that is less than or equal to n
std::optional<uint32_t> nearest_num(const std::map<uint32_t, Label>& labels, uint32_t n) {
    std::optional<uint32_t> result = std::nullopt;
    for (const auto& [key, value] : labels) {
        if (key <= n) {
            if (!result.has_value() || key > result.value()) {
                result = key;
            }
        }
    }
    return result;
}

// Get the instruction label for the given address
std::optional<std::string> get_instruction_label(std::map<uint32_t, Label>& labels, uint32_t addr) {
    auto near_opt = nearest_num(labels, addr);
    if (!near_opt.has_value()) return std::nullopt;
    uint32_t near = near_opt.value();

    if ((near >> 16) != (addr >> 16)) return std::nullopt;

    Label label = labels[near];
    uint32_t offset = addr - near;

    std::stringstream offset_str;
    if (offset > 9) offset_str << std::showbase << std::hex << offset;
    else offset_str << offset;

    std::string result = label.is_func ? label.name : labels[label.parent_addr].name;
    if (offset != 0) result += "+" + offset_str.str();

    return result;
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

    int w = config.width ? config.width : interface_sf->w;
    int h = config.height ? config.height : interface_sf->h;

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

    struct u8_core core{};

    uint8_t *rom = (uint8_t *)malloc((config.hardware_id == HW_ES && config.is_5800p) ? 0x80000 : 0x100000);
    memset((void *)rom, (config.real_hardware || config.hardware_id == HW_TI_MATHPRINT) ? 0xff : 0, (config.hardware_id == HW_ES && config.is_5800p) ? 0x80000 : 0x100000);
    FILE *f = fopen(config.rom_file.c_str(), "rb");
    if (!f) {
        std::cerr << "ERROR: Cannot open ROM:" << strerror(errno) << std::endl;
        return -1;
    }
    fseek(f, 0, SEEK_END);
    int romsize = ftell(f);
    rewind(f);
    fread(rom, sizeof(uint8_t), romsize, f);
    fclose(f);
    auto romdisas = disassemble(romsize, rom, 0);

    uint8_t *flash = NULL;
    std::map<uint32_t, std::string> flashdisas;
    if (config.hardware_id == HW_ES && config.is_5800p) {
        flash = (uint8_t *)malloc(0x80000);
        memset((void *)flash, 0xff, sizeof(flash));
        FILE *f = fopen(config.flash_rom_file.c_str(), "rb");
        if (!f) {
            std::cerr << "ERROR: Cannot open flash ROM:" << strerror(errno) << std::endl;
            return -1;
        }
        fseek(f, 0, SEEK_END);
        int flashsize = ftell(f);
        rewind(f);
        fread(flash, sizeof(uint8_t), flashsize, f);
        fclose(f);
        flashdisas = disassemble(0x40000, (uint8_t *)(flash + 0x40000), 0xc0000);
    }

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
        break;
    case HW_TI_MATHPRINT:
        ramstart = 0xb000;
        ramsize = 0x4000;
        break;
    default:
        ramstart = 0x8000;
        ramsize = config.real_hardware ? 0xe00 : 0x7000;
        break;
    }

    uint8_t *ram = NULL;
    if (!config.ram.empty()) {
        FILE *f = fopen(config.ram.c_str(), "rb");
        if (f) {
            ram = (uint8_t *)malloc(ramsize);
            srand(time(NULL));
            for (size_t i = 0; i < ramsize; i++) ram[i] = rand();
            fread(ram, sizeof(uint8_t), ramsize, f);
            fclose(f);
        } else std::cerr << "WARNING: cannot load RAM data: " << strerror(errno) << std::endl;
    }

    std::map<uint32_t, Label> labels;
    std::map<uint32_t, std::string> data_labels;
    std::map<std::string, std::string> data_bit_labels;
    for (const auto &labelfile : config.labels) {
        if (labelfile.empty()) continue;
        std::ifstream is(labelfile.c_str());
        if (!is) {
            std::cerr << "WARNING: Cannot load label file '" << path << "': " << strerror(errno) << std::endl;
            continue;
        }
        load_labels(is, 0, &labels, &data_labels, &data_bit_labels);
    }
    uint16_t start = read_mem_code(&core, 0, 2, 2);
    uint16_t brk = read_mem_code(&core, 0, 4, 2);
    if (labels.find(start) == labels.end()) labels[start] = {"start", true};
    if (labels.find(brk) == labels.end()) labels[brk] = {"brk", true};

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window2, renderer2);
    ImGui_ImplSDLRenderer2_Init(renderer2);

    uint8_t pd_value;
    if (config.hardware_id != HW_TI_MATHPRINT) pd_value = config.pd_value;
    mcu mcu(&core, &config, rom, flash, ram, ramstart, ramsize, w, h);

    bool quit = false;
    bool single_step = false;
    std::atomic<bool> stop = false;
    static MemoryEditor ramedit;
    static MemoryEditor ram2edit;
    static MemoryEditor sfredit;
    sfredit.ReadFn = &read_sfr_im;
    sfredit.WriteFn = &write_sfr_im;

    bool set_p[8];
    for (int i = 0; i < 8; i++) set_p[i] = pd_value & (1 << i);

    std::vector<std::string> memselect = {"Main RAM", "SFR region"};
    if (config.hardware_id == HW_ES && config.is_5800p) memselect.push_back("PRAM");
    else if ((config.hardware_id == HW_CLASSWIZ_EX || config.hardware_id == HW_CLASSWIZ_CW) && !config.real_hardware) memselect.push_back("Emulator RAM");
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
        /*if (stop.load() && cs_thread.joinable()) {
            cs_thread.join();
            single_step = true;
        }*/

        if (config.hardware_id != HW_TI_MATHPRINT) {
            for (int i = 0; i < 8; i++) {
                if (set_p[i]) pd_value |= (1 << i);
                else pd_value &= ~(1 << i);
            }
            mcu.sfr[0x50] = pd_value;
        }

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
                ImGui::Text("%02X", core.regs.gp[i]);
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
                ImGui::Text("%02X", core.regs.gp[i+8]);
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
            auto a = get_instruction_label(labels, (core.regs.csr << 16) | core.regs.pc);
            if (a.has_value()) ImGui::Text("%s (%X:%04XH)", a.value().c_str(), core.regs.csr, core.regs.pc);
            else ImGui::Text("%X:%04XH", core.regs.csr, core.regs.pc);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("SP");
            ImGui::TableNextColumn();
            ImGui::Text("%04XH", core.regs.sp);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("DSR:EA");
            ImGui::TableNextColumn();
            ImGui::Text("%02X:%04XH", core.regs.dsr, core.regs.ea);

            ImGui::EndTable();
        }
        ImGui::Text("\n");
        if (ImGui::BeginTable("psw", 2, ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("PSW");
            char val[2]; sprintf(val, "%02X", core.regs.psw);
            ImGui::TableSetupColumn(val);
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("C");
            ImGui::TableNextColumn();
            ImGui::Text("[%s]", (core.regs.psw & (1 << 7)) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Z");
            ImGui::TableNextColumn();
            ImGui::Text("[%s]", (core.regs.psw & (1 << 6)) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("S");
            ImGui::TableNextColumn();
            ImGui::Text("[%s]", (core.regs.psw & (1 << 5)) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("OV");
            ImGui::TableNextColumn();
            ImGui::Text("[%s]", (core.regs.psw & (1 << 4)) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("MIE");
            ImGui::TableNextColumn();
            ImGui::Text("[%s]", (core.regs.psw & (1 << 3)) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("HC");
            ImGui::TableNextColumn();
            ImGui::Text("[%s]", (core.regs.psw & (1 << 2)) ? "x" : " ");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("ELEVEL");
            ImGui::TableNextColumn();
            ImGui::Text("%d", core.regs.psw & 3);

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
            ImGui::Text("%X:%04XH", core.regs.lcsr, core.regs.lr);

            for (int i = 0; i < 3; i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("ECSR%d:ELR%d", i+1, i+1);
                ImGui::TableNextColumn();
                ImGui::Text("%X:%04XH", core.regs.ecsr[i], core.regs.elr[i]);
            }

            for (int i = 0; i < 3; i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("EPSW%d", i+1);
                ImGui::TableNextColumn();
                ImGui::Text("%02X", core.regs.epsw[i]);
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
        ImGui::Text("Internal ROM");
        if (ImGui::BeginTable("romdisas", 2)) {
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Instruction");
            ImGui::TableHeadersRow();
            for (const auto& [k, v] : romdisas) {
                ImVec4 color = (k == ((core.regs.csr << 16) | (core.regs.pc))) ? (ImVec4)ImColor(255, 216, 0) : (ImVec4)ImColor(255, 255, 255);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(color, "%X:%04XH", k >> 16, k & 0xffff);
                ImGui::TableNextColumn();
                ImGui::TextColored(color, v.c_str());
            }
            ImGui::EndTable();
        }
        if (config.hardware_id == HW_ES && config.is_5800p) {
            ImGui::Text("Flash ROM");
            if (ImGui::BeginTable("flashdisas", 2)) {
                ImGui::TableSetupColumn("Address");
                ImGui::TableSetupColumn("Instruction");
                ImGui::TableHeadersRow();
                for (const auto& [k, v] : flashdisas) {
                    ImVec4 color = (k == ((core.regs.csr << 16) | (core.regs.pc))) ? (ImVec4)ImColor(255, 216, 0) : (ImVec4)ImColor(255, 255, 255);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(color, "%X:%04XH", k >> 16, k & 0xffff);
                    ImGui::TableNextColumn();
                    ImGui::TextColored(color, v.c_str());
                }
                ImGui::EndTable();
            }
        }
        ImGui::End();

        ImGui::Begin("Call Stack Display", NULL, 0);
        if (ImGui::BeginTable("callstack", 6)) {
            ImGui::TableSetupColumn(NULL, ImGuiTableColumnFlags_WidthFixed, 20);
            ImGui::TableSetupColumn("Function address");
            ImGui::TableSetupColumn("ER0", ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableSetupColumn("ER2", ImGuiTableColumnFlags_WidthFixed, 50);
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
                auto a = get_instruction_label(labels, v.func_addr);
                if (a.has_value()) {
                    ImGui::TextColored(color, a.value().c_str());
                    ImGui::SetItemTooltip("%X:%04XH", v.func_addr >> 16, v.func_addr & 0xffff);
                } else ImGui::TextColored(color, "%X:%04XH", v.func_addr >> 16, v.func_addr & 0xffff);
                ImGui::TableNextColumn();
                if (!ends_with(v.interrupt.interrupt_name, "INT")) {
                    ImGui::TextColored(color, "%04XH", v.er0);
                    ImGui::TableNextColumn();
                    ImGui::TextColored(color, "%04XH", v.er2);
                } else ImGui::TableNextColumn();
                ImGui::TableNextColumn();
                if (v.return_addr_ptr && return_addr_real != v.return_addr) {
                    ImGui::TextColored(color, "%X:%04XH (%X:%04XH)", return_addr_real >> 16, return_addr_real & 0xffff, v.return_addr >> 16, v.return_addr & 0xffff);
                    ImGui::TableNextColumn();
                    ImGui::TextColored(color, "%04XH", v.return_addr_ptr);
                } else {
                    auto b = get_instruction_label(labels, v.return_addr);
                    if (b.has_value()) {
                        ImGui::TextColored(color, b.value().c_str());
                        ImGui::SetItemTooltip("%X:%04XH", v.return_addr >> 16, v.return_addr & 0xffff);
                    } else ImGui::TextColored(color, "%X:%04XH", v.return_addr >> 16, v.return_addr & 0xffff);
                }
                ImGui::TableNextColumn();
                if (v.return_addr_ptr) ImGui::TextColored(color, "%04XH", v.return_addr_ptr);
                if (!v.interrupt.interrupt_name.empty()) {
                    ImGui::TableNextColumn();
                    ImGui::TextColored(color, "[%s: %s]", v.interrupt.nmi ? "NMI" : "MI", v.interrupt.interrupt_name.c_str());
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();

        ImGui::Begin("Hex Editor", NULL, 0);
        const char* preview = memselect[memselect_idx].c_str();
        if (ImGui::BeginCombo("##", preview)) {
            for (int n = 0; n < memselect.size(); n++) {
                const bool is_selected = (memselect_idx == n);
                if (ImGui::Selectable(memselect[n].c_str(), is_selected))
                    memselect_idx = n;
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (memselect_idx == 0) ramedit.DrawContents((void *)mcu.ram, ramsize, ramstart);
        else if (memselect_idx == 1) sfredit.DrawContents((void *)mcu.sfr, 0x1000, 0xf000);
        else if (memselect_idx == 2) ram2edit.DrawContents((void *)mcu.ram2, config.hardware_id == HW_ES && config.is_5800p ? 0x8000 : 0x10000,
                                                          config.hardware_id == HW_CLASSWIZ_CW ? 0x80000 : 0x40000);
        ImGui::End();

        ImGui::Begin("Options", NULL, 0);
        if (ImGui::TreeNode("MCU Control")) {
            if (ImGui::Button("Reset core")) mcu.reset();
            if (config.hardware_id == HW_ES) {
                ImGui::Text("P mode");
                ImGui::Spacing();
                for (int i = 7; i >= 0; i--) {
                    char a[3]; sprintf(a, "##%d", i);
                    ImGui::SameLine();
                    ImGui::Checkbox(a, &set_p[i]);
                }
                ImGui::SameLine();
                ImGui::Text("P%c", get_pmode(pd_value));
                ImGui::Text("  7   6   5   4   3   2  1   0");
            } else if (config.hardware_id == HW_SOLAR_II) {
                ImGui::Text("P mode");
                ImGui::Spacing();
                for (int i = 2; i >= 0; i--) {
                    char a[3]; sprintf(a, "##%d", i);
                    ImGui::SameLine();
                    ImGui::Checkbox(a, &set_p[i]);
                }
                ImGui::SameLine();
                ImGui::Text("P%d%d%d", set_p[2], set_p[1], set_p[0]);
                ImGui::Text("  2   1   0");
            }
            ImGui::Checkbox("Pause/Single-step", &single_step);
            if (single_step) {
                if (ImGui::Button("Step")) mcu.core_step();
            }
            ImGui::TreePop();
            ImGui::Spacing();
        }
        if (ImGui::TreeNode("Other")) {
#if defined _WIN32 || defined __CYGWIN__
            if (ImGui::Button("Copy screen to clipboard")) {
                ImGui::OpenPopup(mcu.screen->render_clipboard() ? "copyclip_success" : "copyclip_fail");
            }
            if (ImGui::BeginPopup("copyclip_success")) {
                ImGui::Text("Screen copied to clipboard!");
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup("copyclip_fail")) {
                ImGui::Text("Oops! An error occurred.");
                ImGui::EndPopup();
            }
#endif
            ImGui::TreePop();
            ImGui::Spacing();
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

    {
        FILE *f = fopen(config.ram.c_str(), "wb");
        if (f) {
            fwrite(ram, sizeof(uint8_t), ramsize, f);
            fclose(f);
        } else std::cerr << "WARNING: cannot save RAM data: " << strerror(errno) << std::endl;
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

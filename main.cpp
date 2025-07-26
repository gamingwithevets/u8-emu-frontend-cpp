#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_revision.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <thread>
#include <atomic>
#include <optional>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <unordered_set>
#include <format>
#include <csignal>
#include <unistd.h>
#include <mutex>

#ifdef __linux__
#include <execinfo.h>
#elif defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#include <dbghelp.h>
#ifdef interface
#undef interface
#endif
#ifdef near
#undef near
#endif
#endif

#include "mcu/mcu.hpp"
#include "mcu/datalabels.hpp"
#include "config/config.hpp"
#include "config/settings.hpp"
#include "config/lang.hpp"
#include "config/binary.hpp"
#include "gui/startupui.hpp"
#include "labeltool/labeltool.hpp"
#include "disas/disas.hpp"
#include "global.hpp"
#include "gui/disasdisp.hpp"
extern "C" {
#include "u8_emu/src/core/core.h"
}
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlrenderer3.h"
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
    {SDL_SCANCODE_APOSTROPHE, SDLK_DBLAPOSTROPHE},
    {SDL_SCANCODE_COMMA, SDLK_LESS},
    {SDL_SCANCODE_PERIOD, SDLK_GREATER},
    {SDL_SCANCODE_SLASH, SDLK_QUESTION},
};

void convert_shift(SDL_Event &event) {
    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        const bool *keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
            auto it = shift_keycombos.find(event.key.scancode);
            if (it != shift_keycombos.end()) event.key.key = it->second;
        }
    }
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

std::string get_instruction_label2(std::map<uint32_t, Label>& labels, uint8_t csr, uint16_t pc) {
    std::string addr_fmt = std::format("{:X}:{:04X}H", csr, pc);
    auto label = get_instruction_label(labels, (csr << 16) | pc);
    if (label.has_value()) return label.value() + " (" + addr_fmt + ")";
    else return addr_fmt;
}

void print_stacktrace() {
#ifdef __linux__
    void* array[20];
    int size = backtrace(array, 20);
    char** messages = backtrace_symbols(array, size);

    std::cerr << "Stacktrace:\n";
    for (int i = 0; i < size; ++i) {
        std::cerr << "  " << messages[i] << "\n";
    }

    free(messages);
#elif defined(_WIN32) || defined(__CYGWIN__)
    void* stack[62];
    unsigned short frames;
    SYMBOL_INFO* symbol;
    HANDLE process;

    process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);

    frames = CaptureStackBackTrace(0, 62, stack, NULL);
    symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    std::cerr << "Stack trace:\n";
    for (int i = 0; i < frames; i++) {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        std::cerr << "  " << i << ": " << symbol->Name << " - " << std::hex << std::showbase << symbol->Address << "\n";
    }

    free(symbol);
#endif
    return;
}

void crash_handler(int sig) {
    __debugbreak();
    std::cerr << "Got signal " << sig << ".\n";
    print_stacktrace();
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

void flush_ram(uint8_t *ram, int ramsize, bool real_hw) {
    if (real_hw) {
        srand(time(NULL));
        for (size_t i = 0; i < ramsize; i++) ram[i] = rand();
    } else memset(ram, 0, ramsize);
    return;
}

int main(int argc, char* argv[]) {
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);

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

    SDL_Window* window2 = SDL_CreateWindow("Debugger", 1920, 1080, SDL_WINDOW_RESIZABLE);
    if (!window2) {
        std::cerr << "Failed to create debugger window. SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer2 = SDL_CreateRenderer(window2, NULL);
    if (!renderer2) {
        std::cerr << "Failed to create debugger renderer. SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window2);
        SDL_Quit();
        return -1;
    }

    int w, h;
    SDL_Surface* interface_sf = IMG_Load(config.interface_path.c_str());
    if (interface_sf) {
        w = config.width ? config.width : interface_sf->w;
        h = config.height ? config.height : interface_sf->h;
    } else {
        std::cerr << "WARNING: Failed to load interface image. SDL_Error: " << SDL_GetError() << std::endl;
        w = config.width;
        h = config.height;
    }

    SDL_Window* window = SDL_CreateWindow(!config.w_name.empty() ? config.w_name.c_str() : "u8-emu-frontend-cpp", w, h, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Failed to create window. SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window2);
        if (interface_sf) SDL_DestroySurface(interface_sf);
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        std::cerr << "Failed to create renderer. SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window2);
        SDL_DestroyWindow(window);
        SDL_DestroySurface(interface_sf);
        SDL_Quit();
        return -1;
    }
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_SetRenderLogicalPresentation(renderer, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    SDL_Texture* interface = nullptr;
    if (interface_sf != nullptr) {
        interface = SDL_CreateTextureFromSurface(renderer, interface_sf);
        SDL_DestroySurface(interface_sf);
    }

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

    std::map<uint32_t, Label> labels;
    for (const auto &labelfile : config.labels) {
        if (labelfile.empty()) continue;
        std::ifstream is(labelfile.c_str());
        if (!is) {
            std::cerr << "WARNING: Cannot load label file '" << labelfile << "': " << strerror(errno) << std::endl;
            continue;
        }
        load_labels(is, 0, &labels);
    }
    uint16_t start = (rom[3] << 8) | rom[2];
    uint16_t brk = (rom[5] << 8) | rom[4];
    if (labels.find(start) == labels.end()) labels[start] = {"$$start_up", true};
    if (labels.find(brk) == labels.end()) labels[brk] = {"$$brk_reset", true};

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
        //flashdisas = disassemble(0x40000, (uint8_t *)(flash + 0x40000), 0xc0000);
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

    uint8_t *ram = (uint8_t *)malloc(ramsize);
    flush_ram(ram, ramsize, config.real_hardware);
    if (!config.ram.empty()) {
        FILE *f = fopen(config.ram.c_str(), "rb");
        if (f) {
            fread(ram, sizeof(uint8_t), ramsize, f);
            fclose(f);
        } else std::cerr << "WARNING: cannot load RAM data: " << strerror(errno) << std::endl;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingWithShift = false;
    ImFont *font = set_font();

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window2, renderer2);
    ImGui_ImplSDLRenderer3_Init(renderer2);

    register_settings_handler();

    uint8_t pd_value;
    if (config.hardware_id != HW_TI_MATHPRINT) pd_value = config.pd_value;
    mcu mcu(&core, &config, rom, flash, ram, ramstart, ramsize, w, h);

    bool quit = false;
    bool single_step = false;
    std::atomic<bool> stop = false;
    static MemoryEditor ramedit{};
    static MemoryEditor sfredit{};
    ramedit.OptShowOptions = false;
    sfredit.OptShowOptions = false;
    sfredit.ReadFn = &read_sfr_im;
    sfredit.WriteFn = &write_sfr_im;

    bool set_p[8];
    for (int i = 0; i < 8; i++) set_p[i] = pd_value & (1 << i);

    std::vector<std::string> memselect = {"Main RAM", "SFR region"};
    if (config.hardware_id == HW_ES && config.is_5800p) memselect.push_back("PRAM");
    else if ((config.hardware_id == HW_CLASSWIZ_EX || config.hardware_id == HW_CLASSWIZ_CW) && !config.real_hardware) memselect.push_back("Emulator RAM");
    static int memselect_idx = 0;
    static uint16_t addr = 0xffff;
    static uint32_t addrbase = 0;
    static char labeltext[1000];

    unsigned int a, b;
    double fps;

    printf("[CEMSVCDisas] Disassembling\n");

    uint8_t *beg = rom;
    uint8_t *before = rom;
    uint8_t *cur = beg;
    std::vector<int> swis{};
    while ((cur-beg) < romsize) {
        auto pc = cur - beg;
        std::stringstream ss{};
        decode(ss, cur, pc, mcu.interrupts);
        if (!ss.str().rfind("SWI #")) swis.push_back(std::stoi(&ss.str()[5], NULL));
        int siz = cur - before;
        CodeElem ce{};
        switch (siz) {
        case 2:
            sprintf_s(ce.srcbuf, "%04X          ", (*(uint16_t*)before));
            break;
        case 4:
            sprintf_s(ce.srcbuf, "%04X%04X      ", (*(uint16_t*)before), ((uint16_t*)before)[1]);
            break;
        case 6:
            sprintf_s(ce.srcbuf, "%04X%04X%04X  ", (*(uint16_t*)before), ((uint16_t*)before)[1], ((uint16_t*)before)[2]);
            break;
        default:
            strcpy(ce.srcbuf, "              ");
            break;
        }
        ce.offset = pc;
        auto s = ss.str();
        auto a = strchr(s.c_str(), '/');
        if (a) ce.xref_operand = strtol(a+1, NULL, 16);
        strcpy(ce.srcbuf + 14, s.c_str());
        codes.push_back(ce);
        before = cur;
    }

    printf("[CEMSVCDisas] Generating SWI address table\n");
    for (CodeElem& ce : codes) {
        if (ce.offset < 0x80 || ce.offset > 0xfe) continue;
        auto id = (ce.offset - 0x80) >> 1;
        if (std::find(swis.begin(), swis.end(), id) != swis.end()) {
            auto addr = (*(uint16_t*)(rom+ce.offset));
            sprintf(ce.srcbuf+14, "SWI #%d /%04XH", id, addr);
            LABEL_FUNCTION(addr);
            ce.xref_operand = addr;
        }
    }

    printf("[CEMSVCDisas] Linking labels\n");
    std::optional<int> last_label{};
    std::unordered_set<int> quick_find{};
    for (auto& ce : codes) {
        quick_find.emplace(ce.offset);
    }

    for (auto& lb : p_labels) {
        CodeElem ce{};
        auto iter = quick_find.find(lb.first);
        if (iter == quick_find.end()) continue;
        ce.is_label = true;
        if (lb.second) {
            auto iter = labels.find(lb.first);
            char symb[50];
            if (iter == labels.end()) sprintf(symb, "_f_%05X", lb.first);
            else {
                auto name = iter->second.name;
                if (ends_with(name, "u8") || name[0] == '_' || name[0] == '$') strcpy_s(symb, name.c_str());
                else {
                    symb[0] = '_';
                    strcpy_s(&symb[1], 49, name.c_str());
                }
            }
            strcpy_s(ce.srcbuf, symb);
            ce.offset = 0;
            labels[lb.first] = {std::string(symb), true};
            last_label = lb.first;
        }
        else {
            char symb[50];
            sprintf(symb, "_$j_%05x", lb.first);
            strcpy_s(ce.srcbuf, symb);
            ce.offset = 0;
            labels[lb.first] = Label{std::string(symb), false, last_label.has_value() ? last_label.value() : 0};
        }
    }
    printf("[CEMSVCDisas] Applying labels\n");
    std::vector<CodeElem> finals;
    finals.reserve(codes.size() + labels.size());
    for (auto& ce : codes) {
        if (labels.find(ce.offset) != labels.end()) {
            CodeElem ce2{};
            ce2.is_label = true;
            strcpy_s(ce2.srcbuf, (labels[ce.offset].name + ":").c_str());
            ce2.offset = 0;
            finals.push_back(ce2);
        }
        if (ce.xref_operand) {
            auto a = strchr(ce.srcbuf, '/');
            if (labels.find(ce.xref_operand) != labels.end()) strcpy(a, labels[ce.xref_operand].name.c_str());
            else sprintf(a, "%X:%04XH", ce.xref_operand >> 16, ce.xref_operand & 0xfffe);
        }
        finals.push_back(ce);
    }
    codes = std::move(finals);
    printf("[CEMSVCDisas] Done!\n");
    max_row = codes.size();

    std::thread cs_thread(core_step_loop, std::ref(stop));
    SDL_Event e;
    while (!quit) {
        a = SDL_GetTicks();

        while (SDL_PollEvent(&e) != 0) {
            convert_shift(e);
            if (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) mcu.keyboard->process_event(renderer, &e);
            if (SDL_GetWindowFlags(window2) & SDL_WINDOW_INPUT_FOCUS) ImGui_ImplSDL3_ProcessEvent(&e);
            if (e.type == SDL_EVENT_QUIT) quit = true;
            else if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) quit = true;
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

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::PushFont(font);

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
            if (ImGui::Selectable(get_instruction_label2(labels, core.regs.csr, core.regs.pc).c_str()))
                JumpTo((mcu.core->regs.csr << 16) | mcu.core->regs.pc);

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
            else if (mcu.paused) ImGui::Text("[Execution paused.]");
            else ImGui::Text("%.1f IPS", mcu.ips);

            ImGui::EndTable();
        }
        ImGui::End();

        ImGui::Begin("Disassembly", NULL, 0);
        pc_cache = (mcu.core->regs.csr << 16) | (mcu.core->regs.pc & 0xfffe);
        drawdisas();
        ImGui::End();

        ImGui::Begin("Call Stack Display", NULL, 0);
        ImGui::Text("NOTE: Actual stack data may be different.");
        if (ImGui::BeginTable("callstack", 6)) {
            ImGui::TableSetupColumn(NULL, ImGuiTableColumnFlags_WidthFixed, 20);
            ImGui::TableSetupColumn("Function address");
            ImGui::TableSetupColumn("ER0", ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableSetupColumn("ER2", ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableSetupColumn("Return address");
            ImGui::TableSetupColumn("LR pushed at");
            ImGui::TableHeadersRow();
            std::vector<call_stack_data> callstack;
            {
                std::lock_guard<std::mutex> lock(mcu.call_stack_mutex);
                callstack = mcu.call_stack;
            }
            int callstack_size = callstack.size();
            //printf("[Callstack] Printing %d entries in callstack.\n", callstack_size);
            for (int i = callstack_size - 1; i >= 0; i--) {
                int j = abs(callstack_size - i - 1);
                //printf("[Callstack] Printing Entry #%d (item %d)\n", j, i);
                auto v = callstack[i];
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
                    ImGui::TableNextColumn();
                    if (v.return_addr_ptr) ImGui::TextColored(color, "%04XH", v.return_addr_ptr);
                    else if (!v.interrupt.interrupt_name.empty()) ImGui::TextColored(color, "[%s: %s]", v.interrupt.nmi ? "NMI" : "MI", v.interrupt.interrupt_name.c_str());
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();

        ImGui::Begin("Addresses", NULL, 0);
        std::map<uint32_t, wanted_sfrs_data> wanted;
        {
            std::lock_guard<std::mutex> lock(mcu.wanted_sfrs_mutex);
            if (ImGui::Button("Clear")) mcu.wanted_sfrs.clear();
            wanted = mcu.wanted_sfrs;
        }
        if (ImGui::BeginTable("wanted", 3)) {
            ImGui::TableSetupColumn("Address");
            ImGui::TableSetupColumn("Read count");
            ImGui::TableSetupColumn("Write count");
            ImGui::TableHeadersRow();
            for (const auto &[k, v] : wanted) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%02X:%04XH", k >> 16, k & 0xffff);
                ImGui::TableNextColumn();
                ImGui::Text("%d", v.read);
                ImGui::TableNextColumn();
                ImGui::Text("%d", v.write);
            }
            ImGui::EndTable();
        }
        ImGui::End();

        ImGui::Begin("Hex Editor", NULL, ImGuiWindowFlags_NoScrollbar);
        const char* preview = memselect[memselect_idx].c_str();
        if (ImGui::BeginCombo("##", preview)) {
            for (int n = 0; n < memselect.size(); n++) {
                const bool is_selected = (memselect_idx == n);
                if (ImGui::Selectable(memselect[n].c_str(), is_selected)) memselect_idx = n;
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        uint16_t data_addr = 0xffff;
        uint16_t data_length = 0;
        if (memselect_idx == 1) {
            sfrdata data{};
            mcu.labels->get_sfr_name(addr, &data);
            if (!data.name.empty()) {
                if (data.bbits && data.wbits) {
                    if (addr > data.addr) sprintf(labeltext, "%s\n%s (B)  %04X (%d bits)\n\n%s", data.name.c_str(), data.bsymbol1.c_str(), addr + addrbase, data.wbits - data.bbits, data.desc.c_str());
                    else sprintf(labeltext, "%s\n%s (B) / %s (W)  %04X (%d/%d bits)\n\n%s", data.name.c_str(), data.bsymbol0.c_str(), data.wsymbol.c_str(), data.addr + addrbase, data.bbits, data.wbits, data.desc.c_str());
                }
                else if (data.wbits) sprintf(labeltext, "%s\n%s (W)  %04X (%d bits)\n\n%s", data.name.c_str(), data.wsymbol.c_str(), data.addr + addrbase, data.wbits, data.desc.c_str());
                else if (data.bbits) sprintf(labeltext, "%s\n%s (B)  %04X (%d bits)\n\n%s", data.name.c_str(), data.bsymbol0.c_str(), data.addr + addrbase, data.bbits, data.desc.c_str());
                else labeltext[0] = '\0';
                data_addr = data.addr;
                data_length = data.wbits ? 2 : (data.bbits ? 1 : 0);
            } else labeltext[0] = '\0';
        } else {
            dldata data{};
            mcu.labels->get_name(memselect_idx, addr, &data);
            if (!data.name.empty()) {
                if (data.len > 1) sprintf(labeltext, "%s  %04X - %04X (size %d)\n\n%s", data.name.c_str(), data.addr + addrbase, data.addr + addrbase + data.len - 1, data.len, data.desc.c_str());
                else sprintf(labeltext, "%s  %04X\n\n%s", data.name.c_str(), data.addr + addrbase, data.desc.c_str());
                data_addr = data.addr;
                data_length = data.len;
            } else labeltext[0] = '\0';
        }

        ImGui::BeginChild("hexed");
        ImVec2 footer_size = ImGui::CalcTextSize(labeltext, NULL, false, ImGui::GetWindowWidth());
        if (labeltext[0]) {
            footer_size.x -= 10;
            footer_size.y += 13;
        }
        switch (memselect_idx) {
            case 1:
                addrbase = 0xf000;
                sfredit.DrawContents((void *)mcu.sfr, 0x1000, addrbase);
                sfredit.OptFooterExtraHeight = footer_size.y;
                addr = sfredit.DataEditingAddr;
                break;
            default:
                uint8_t *data;
                uint16_t contsize;
                if (memselect_idx == 2) {
                    data = mcu.ram2;
                    addrbase = config.hardware_id == HW_CLASSWIZ_CW ? 0x80000 : 0x40000;
                    contsize = config.hardware_id == HW_ES && config.is_5800p ? 0x8000 : 0x10000;
                } else {
                    data = mcu.ram;
                    addrbase = ramstart;
                    contsize = ramsize;
                }
                ramedit.DrawContents((void *)data, contsize, addrbase);
                ramedit.OptFooterExtraHeight = footer_size.y;
                addr = ramedit.DataEditingAddr;
                if (data_length) {
                    ramedit.HighlightMin = data_addr;
                    ramedit.HighlightMax = data_addr + data_length;
                } else ramedit.HighlightMin = ramedit.HighlightMax = -1;
                break;
        }
        ImGui::TextWrapped(labeltext);
        ImGui::EndChild();
        ImGui::End();

        char label[200];
        sprintf(label, "%s###Options", get_strloc(s_options));
        ImGui::Begin(label, NULL, 0);
        sprintf(label, "%s###MCU Control", get_strloc(s_options_mcu));
        if (ImGui::TreeNode(label)) {
            if (ImGui::Button(get_strloc(s_options_mcu_reset))) mcu.reset();
            ImGui::Text(get_strloc(s_options_mcu_cps));
            int cps_multiplier = mcu.cps_multiplier.load();
            const int cps_multiplier_max = 32768;
            ImGui::SliderInt("##cpsslider", &cps_multiplier, -1, cps_multiplier_max, "");
            ImGui::SameLine();
            char buf[32];
            snprintf(buf, sizeof(buf), " %d", cps_multiplier_max);
            ImGui::SetNextItemWidth(ImGui::CalcTextSize(buf).x);
            ImGui::InputInt("##cpsinput", &cps_multiplier, -1, -1, ImGuiInputTextFlags_CharsDecimal);
            cps_multiplier = std::clamp(cps_multiplier, -1, cps_multiplier_max);
            mcu.cps_multiplier.store(cps_multiplier);
            if (config.hardware_id == HW_ES) {
                ImGui::Text(get_strloc(s_options_mcu_pmode));
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
                ImGui::Text(get_strloc(s_options_mcu_pmode));
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
            ImGui::Checkbox(get_strloc(s_options_mcu_pause), &single_step);
            if (single_step && ImGui::Button(get_strloc(s_options_mcu_step))) mcu.core_step();
            if (ImGui::Button(get_strloc(s_options_mcu_resetfull))) {
                flush_ram(mcu.ram, ramsize, config.real_hardware);
                mcu.reset();
            }
            ImGui::TreePop();
            ImGui::Spacing();
        }
        sprintf(label, "%s###Interface", get_strloc(s_options_interface));
        if (ImGui::TreeNode(label)) {
            ImGui::Text(get_strloc(s_language));
            ImGui::SameLine();
            if (ImGui::BeginCombo("##langselect", get_langname(g_settings.lang))) {
                for (int n = 0; n < (int)Language::Count; n++) {
                    const bool is_selected = (g_settings.lang == (Language)n);
                    if (ImGui::Selectable(get_langname((Language)n), is_selected)) g_settings.lang = (Language)n;
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::TreePop();
            ImGui::Spacing();
        }
        sprintf(label, "%s###Other", get_strloc(s_options_other));
        if (ImGui::TreeNode(label)) {
#if defined(_WIN32) || defined(__CYGWIN__)
            if (ImGui::Button(get_strloc(s_options_other_copyclip))) {
                ImGui::OpenPopup(mcu.screen->render_clipboard() ? "copyclip_success" : "copyclip_fail");
            }
            if (ImGui::BeginPopup("copyclip_success")) {
                ImGui::Text(get_strloc(s_options_other_copyclip_success));
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup("copyclip_fail")) {
                ImGui::Text(get_strloc(s_options_other_copyclip_fail));
                ImGui::EndPopup();
            }
#endif
            ImGui::TreePop();
            ImGui::Spacing();
        }
        sprintf(label, "%s###About", get_strloc(s_options_about));
        if (ImGui::TreeNode(label)) {
            ImGui::Text("u8-emu-frontend-cpp");
#ifdef GITHUB_RUNID
            ImGui::Text("GitHub Actions build - Run ID: %llu", GITHUB_RUNID);
#endif
            ImGui::Text("Build time: %s %s", __DATE__, __TIME__);
            ImGui::Spacing();
            int linked = SDL_GetVersion();
            int linked_image = IMG_Version();
#ifdef _MSVC_LANG
            ImGui::Text("C++ standard library version: %ld", _MSVC_LANG);
#else
            ImGui::Text("C++ standard library version: %ld", __cplusplus);
#endif
            ImGui::Spacing();
            ImGui::Text("SDL version:");
            ImGui::Text("  Compiled: %u.%u.%u (%s)", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION, SDL_REVISION);
            ImGui::Text("  Linked: %u.%u.%u (%s)", SDL_VERSIONNUM_MAJOR(linked), SDL_VERSIONNUM_MINOR(linked), SDL_VERSIONNUM_MICRO(linked), SDL_GetRevision());
            ImGui::Text("SDL_image version:");
            ImGui::Text("  Compiled: %u.%u.%u", SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_MICRO_VERSION);
            ImGui::Text("  Linked: %u.%u.%u", SDL_VERSIONNUM_MAJOR(linked_image), SDL_VERSIONNUM_MINOR(linked_image), SDL_VERSIONNUM_MICRO(linked_image));
            ImGui::Spacing();
            ImGui::Text("Dear ImGui version: %s (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);

            ImGui::Spacing();
            ImGui::TextWrapped("Special thanks to the members of the Casio Calculator Hacking community for making this project possible.\n\nCode from Xyzst/CasioEmuX and telecomadm1145/CasioEmuMsvc used under GPL-v3.");

            const char *build_date = __DATE__;
            int year = atoi(build_date + 7);
            ImGui::Spacing();
            char years[10] = "2024";
            if (year > 2024) sprintf(years+4, "-%d", year);
            ImGui::Text("(c) %s GamingWithEvets Inc.\nLicensed under the GNU GPL-v3 license\n\nGitHub repository:\nhttps://github.com/gamingwithevets/u8-emu-frontend-cpp", years);
            ImGui::TreePop();
            ImGui::Spacing();
        }
        ImGui::PopFont();
        ImGui::End();

        ImGui::Render();

        SDL_RenderClear(renderer);
        SDL_RenderFillRect(renderer, NULL);
        if (interface) SDL_RenderTexture(renderer, interface, NULL, NULL);
        mcu.screen->render(renderer);
        mcu.keyboard->render(renderer);
        SDL_RenderPresent(renderer);

        SDL_RenderClear(renderer2);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer2);
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


    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    mcu.~mcu();
    free((void *)rom);
    if (interface) SDL_DestroyTexture(interface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyRenderer(renderer2);
    SDL_DestroyWindow(window);
    SDL_DestroyWindow(window2);
    SDL_Quit();

    std::exit(0);
}

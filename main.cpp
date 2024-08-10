#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <atomic>

#include "mcu/mcu.hpp"
#include "config/config.hpp"
#include "peripheral/screen.hpp"
extern "C" {
#include "u8_emu/src/core/core.h"
#include "nxu8_disas/src/lib/lib_nxu8.h"
}
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"

const int DISPLAY_WIDTH = 96;
const int DISPLAY_HEIGHT = 31;

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL. SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }
    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
        std::cerr << "Failed to initialize SDL_image. SDL_image Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    config config = {0};

    // Test config!!!!
    config.rom_file = "roms/GY454XE .bin";
    config.hardware_id = 3;
    config.real_hardware = true;
    config.status_bar_path = "images/interface_es_bar.png";
    config.interface_path = "images/interface_esp_991esp.png";
    config.w_name = "fx-570ES PLUS Emulator";
    config.screen_tl_w = 58;
    config.screen_tl_h = 132;
    config.pix_w = 3;
    config.pix_h = 3;
    config.pix_color = 0xff000000;

    SDL_Surface* interface_sf = IMG_Load(config.interface_path.c_str());
    if (interface_sf == nullptr) {
        std::cerr << "Failed to load interface image. SDL_image Error: " << IMG_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    int w = interface_sf->w;
    int h = interface_sf->h;

    SDL_Window* window = SDL_CreateWindow(config.w_name.empty() ? config.w_name.c_str() : "u8-emu-frontend-cpp", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Failed to create window. SDL_Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(interface_sf);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create renderer. SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_FreeSurface(interface_sf);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Window* window2 = SDL_CreateWindow("Debugger", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Failed to create debugger window. SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer2 = SDL_CreateRenderer(window2, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create debugger renderer. SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window2);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Texture* interface = SDL_CreateTextureFromSurface(renderer, interface_sf);
    SDL_FreeSurface(interface_sf);

    SDL_Event e;

    u8_core core;

    uint8_t *rom = (uint8_t *)malloc(0x100000);
    memset((void *)rom, 0xff, 0x100000);
    FILE *f = fopen(config.rom_file.c_str(), "rb");
    if (!f) {
        std::cout << "Cannot open ROM!" << std::endl;
        return -1;
    }
    fread(rom, sizeof(uint8_t), 0x20000, f);
    fclose(f);

    mcu mcu(&core, &config, rom, NULL, 0x8000, 0xe00);
    screen screen(&config);

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
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window2, renderer2);
    ImGui_ImplSDLRenderer2_Init(renderer2);

    std::atomic<bool> quit = false;
    printf("Test C++ ES PLUS emulator.\nPress [ESC] to quit\n");
    std::thread cs_thread(&mcu::core_step_loop, &mcu, std::ref(quit));
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (SDL_GetWindowFlags(window2) & SDL_WINDOW_INPUT_FOCUS) ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) quit = true;
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) quit = true;
            else if (e.type == SDL_KEYDOWN) {
                //if (e.key.keysym.sym == SDLK_BACKSLASH) mcu.core_step();
                if (e.key.keysym.sym == SDLK_ESCAPE) quit = true;
            }
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

        ImGui::Render();

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, interface, NULL, NULL);
        screen.render_screen(renderer);
        SDL_RenderPresent(renderer);

        SDL_RenderClear(renderer2);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer2);
        SDL_RenderPresent(renderer2);
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    screen.~screen();
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

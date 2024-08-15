#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <atomic>
#include <ctime>
#include <unistd.h>

#include "mcu.hpp"
#include "../config/config.hpp"
#include "../peripheral/standby.hpp"
#include "../peripheral/timer.hpp"
#include "../peripheral/keyboard.hpp"
extern "C" {
#include "../u8_emu/src/core/core.h"
}
#include "../imgui/imgui.h"

mcu *mcuptr;
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

uint8_t read_sfr(struct u8_core *core, uint8_t seg, uint16_t addr) {
    return mcuptr->sfr[addr];
}

void write_sfr(struct u8_core *core, uint8_t seg, uint16_t addr, uint8_t val) {
    if (mcuptr->sfr_write[addr]) mcuptr->sfr[addr] = mcuptr->sfr_write[addr](mcuptr, addr, val);
}

uint8_t battery(struct u8_core *core, uint8_t seg, uint16_t addr) {
    return 0xff;
}

uint8_t read_flash(struct u8_core *core, uint8_t seg, uint16_t offset) {
    uint32_t fo = ((seg << 16) + offset) & 0x7ffff;
    if (mcuptr->flash_mode == 6) {
        mcuptr->flash_mode = 0;
        return 0x80;
    }
    return mcuptr->flash[fo];
}

uint8_t write_dsr(mcu *mcu, uint16_t addr, uint8_t val) {
    mcu->core->regs.dsr = val;
    return val;
}

void write_flash(struct u8_core *core, uint8_t seg, uint16_t offset, uint8_t data) {
    uint32_t fo = ((seg << 16) + offset) & 0x7ffff;
    switch (mcuptr->flash_mode) {
        case 0:
            if (fo == 0xaaa && data == 0xaa) {
                mcuptr->flash_mode = 1;
                return;
            }
            break;
        case 1:
            if (fo == 0x555 && data == 0x55) {
                mcuptr->flash_mode = 2;
                return;
            }
            break;
        case 2:
            if (fo == 0xAAA && data == 0xA0) {
                mcuptr->flash_mode = 3;
                return;
            }
            if (fo == 0xaaa && data == 0x80) {
                mcuptr->flash_mode = 4;
                return;
            }
            break;
        case 3:
            //printf("%05X = %02x\n", fo + 0x80000, data);
            mcuptr->flash[fo] = data;
            mcuptr->flash_mode = 0;
            return;
        case 4:
            if (fo == 0xAAA && data == 0xaa) {
                mcuptr->flash_mode = 5;
                return;
            }
            break;
        case 5:
            if (fo == 0x555 && data == 0x55) {
                mcuptr->flash_mode = 6;
                return;
            }
            break;
        case 6: // we dont know sector's mapping(?)
            if (fo == 0)
                memset(&mcuptr->flash[fo], 0xff, 0x7fff);
            if (fo == 0x20000 || fo == 0x30000)
                memset(&mcuptr->flash[fo], 0xff, 0xffff);
            //printf("erase %05X (%02x)\n", fo+0x80000, data);
            return;
        case 7:
            if (fo == 0xaaa && data == 0xaa) {
                mcuptr->flash_mode = 1;
                return;
            }
            break;
    }
    if (data == 0xf0) {
        //printf("reset mode\n");
        mcuptr->flash_mode = 0;
        return;
    }
    //if (data == 0xb0) {
    //	printf("Erase Suspend.\n");
    //	return;
    //}
    //if (data == 0x30) {
    //	printf("Erase Suspend.\n");
    //	return;
    //}
    printf("write_flash: unknown JEDEC %05x = %02x\n", (int)fo, data);
}

mcu::mcu(struct u8_core *core, struct config *config, uint8_t *rom, uint8_t *flash, int ramstart, int ramsize, int w, int h) {
    this->core = core;
    this->config = config;
    this->rom = rom;
    this->flash = flash;

    mcuptr = this;
    this->cycles_per_second = 1024 * 1024 * 8;

    // ROM
    this->core->codemem.num_regions = (this->config->hardware_id == 2 && this->config->is_5800p) ? 2 : 1;
    this->core->codemem.regions = (struct u8_mem_reg *)malloc(sizeof(struct u8_mem_reg) * this->core->codemem.num_regions);
    this->core->codemem.regions[0] = (struct u8_mem_reg){
        .type = U8_REGION_CODE,
        .rw = false,
        .addr_l = 0,
        .addr_h = (this->config->hardware_id == HW_ES && this->config->is_5800p) ? 0x7ffff : 0xfffff,
        .acc = U8_MACC_ARR,
        .array = this->rom
    };

    this->core->mem.num_regions = 8;
    this->core->mem.regions = (struct u8_mem_reg *)malloc(sizeof(struct u8_mem_reg) * 8);

    // ROM window
    this->core->mem.regions[0] = (struct u8_mem_reg){
        .type = U8_REGION_DATA,
        .rw = false,
        .addr_l = 0,
        .addr_h = ramstart - 1,
        .acc = U8_MACC_ARR,
        .array = this->rom
    };

    // Main RAM
    this->ram = (uint8_t *)malloc(ramsize);
    memset(this->ram, 0, ramsize);
    this->core->mem.regions[1] = (struct u8_mem_reg){
        .type = U8_REGION_DATA,
        .rw = true,
        .addr_l = ramstart,
        .addr_h = ramstart + ramsize - 1,
        .acc = U8_MACC_ARR,
        .array = this->ram
    };

    // SFRs
    this->sfr = (uint8_t *)malloc(0x1000);
    memset(this->sfr, 0, 0x1000);
    this->core->mem.regions[2] = (struct u8_mem_reg){
        .type = U8_REGION_DATA,
        .rw = true,
        .addr_l = 0xf000,
        .addr_h = 0xffff,
        .acc = U8_MACC_FUNC,
        .read = &read_sfr,
        .write = &write_sfr
    };

    switch (this->config->hardware_id) {
        // ClassWiz
        case HW_CLASSWIZ_EX:  // LAPIS ML620606
        case HW_CLASSWIZ_CW:  // LAPIS ML620609
            // Code segment 1+ mirror
            this->core->mem.regions[3] = (struct u8_mem_reg){
                .type = U8_REGION_DATA,
                .rw = false,
                .addr_l = 0x10000,
                .addr_h = this->config->hardware_id == 5 ? 0x7ffff : 0x3ffff,
                .acc = U8_MACC_ARR,
                .array = (uint8_t *)(this->rom + 0x10000)
            };


            if (this->config->real_hardware) {
                // Code segment 0 mirror
                this->core->mem.regions[4] = (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = false,
                    .addr_l = this->config->hardware_id == 5 ? 0x80000 : 0x50000,
                    .addr_h = this->config->hardware_id == 5 ? 0x8ffff : 0x5ffff,
                    .acc = U8_MACC_ARR,
                    .array = this->rom
                };

                this->core->u16_mode = true;
                this->cycles_per_second = (this->config->hardware_id == 5 ? 2048 : 1024) * 1024 * 2;
            } else {
                // Segment 4/8 [emulator]
                this->ram2 = (uint8_t *)malloc(0x10000);
                memset(this->ram2, 0, 0x10000);
                this->core->mem.regions[4] = (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = true,
                    .addr_l = this->config->hardware_id == 5 ? 0x80000 : 0x40000,
                    .addr_h = this->config->hardware_id == 5 ? 0x8ffff : 0x4ffff,
                    .acc = U8_MACC_ARR,
                    .array = this->ram2
                };
            }

            break;

        // TI MathPrint - LAPIS ML620418A
        case HW_TI_MATHPRINT:
            // Code segment 1+ mirror
            this->core->mem.regions[3] = (struct u8_mem_reg){
                .type = U8_REGION_DATA,
                .rw = false,
                .addr_l = 0x10000,
                .addr_h = 0x3ffff,
                .acc = U8_MACC_ARR,
                .array = (uint8_t *)(this->rom + 0x10000)
            };

            // Code segment 0+ mirror 2
            this->core->mem.regions[4] = (struct u8_mem_reg){
                .type = U8_REGION_DATA,
                .rw = false,
                .addr_l = 0x80000,
                .addr_h = 0xaffff,
                .acc = U8_MACC_ARR,
                .array = this->rom
            };

            break;

        // SOLAR II
        case HW_SOLAR_II:
            this->core->small_mm = true;
            break;

        // ES, ES PLUS
        default:
            if (this->config->real_hardware) this->cycles_per_second = 128 * 1024 * 2;
            // Code segment 1 mirror
            this->core->mem.regions[3] = (struct u8_mem_reg){
                .type = U8_REGION_DATA,
                .rw = false,
                .addr_l = 0x10000,
                .addr_h = 0x1ffff,
                .acc = U8_MACC_ARR,
                .array = (uint8_t *)(this->rom + 0x10000)
            };

            if (!this->config->old_esp) {
                // Code segment 8 mirror
                this->core->mem.regions[4] = (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = false,
                    .addr_l = 0x80000,
                    .addr_h = 0x8ffff,
                    .acc = U8_MACC_ARR,
                    .array = this->rom
                };
            }

            // fx-5800P
            if (this->config->hardware_id == 2 && this->config->is_5800p) {
                // Flash (code)
                this->core->codemem.regions[1] = (struct u8_mem_reg){
                    .type = U8_REGION_CODE,
                    .rw = false,
                    .addr_l = 0x80000,
                    .addr_h = 0xfffff,
                    .acc = U8_MACC_ARR,
                    .array = this->flash
                };

                // Flash (data)
                this->core->mem.regions[5] = (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = true,
                    .addr_l = 0x80000,
                    .addr_h = 0xfffff,
                    .acc = U8_MACC_FUNC,
                    .read = &read_flash,
                    .write = &write_flash
                };

                // PRAM
                this->ram2 = (uint8_t *)malloc(0x80000);
                memset(this->ram2, 0, 0x80000);
                this->core->mem.regions[6] = (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = true,
                    .addr_l = 0x40000,
                    .addr_h = 0x47fff,
                    .acc = U8_MACC_ARR,
                    .array = this->ram2
                };

                // Battery
                this->core->mem.regions[7] = (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = false,
                    .addr_l = 0x100000,
                    .addr_h = 0x100000,
                    .acc = U8_MACC_FUNC,
                    .read = &battery
                };
            }
            break;
    }

    if (this->config->hardware_id != HW_TI_MATHPRINT) this->sfr[0x50] = this->config->pd_value;

    memset((void *)this->sfr_write, 0, sizeof(this->sfr_write));
    register_sfr(0, 1, &default_write<0xff>);

    this->standby = new class standby;
    this->timer = new class timer(this, 10000);
    this->keyboard = new class keyboard(this, this->config, w, h);
    this->screen = new class screen(this, this->config);

    this->reset();
}

mcu::~mcu() {
    free((void *)this->ram);
    free((void *)this->sfr);
    if (this->ram2) free((void *)this->ram2);
    this->screen->~screen();
}

void mcu::core_step() {
    this->sfr[0] = this->core->regs.dsr;
    this->core->regs.csr &= (this->config->real_hardware && this->config->hardware_id == 3) ? 1 : 0xf;

    if (!this->standby->stop_mode) {
        uint16_t data = read_mem_code(this->core, this->core->regs.csr, this->core->regs.pc, 2);
        // BL Cadr
        if ((data & 0xf0ff) == 0xf001) {
            uint16_t addr = read_mem_code(this->core, this->core->regs.csr, this->core->regs.pc+2, 2);
            call_stack.push_back((struct call_stack_data){(((data >> 8) & 0xf) << 16) | addr, (this->core->regs.csr << 16) | (this->core->regs.pc+4)});
        // BL ERn
        } else if ((data & 0xff0f) == 0xf003) {
            uint16_t addr = read_mem_code(this->core, this->core->regs.csr, this->core->regs.pc+2, 2);
            call_stack.push_back((struct call_stack_data){(this->core->regs.csr << 16) | read_reg_er(this->core, (data >> 4) & 0xf), (this->core->regs.csr << 16) | (this->core->regs.pc+4)});
        // PUSH LR
        } else if ((data & 0xf8ff) == 0xf8ce && !call_stack.empty()) call_stack.back().return_addr_ptr = this->core->regs.sp - 4;
        // POP PC
        else if ((data & 0xf2ff) == 0xf28e && !call_stack.empty()) call_stack.pop_back();
        // RT
        else if (data == 0xfe1f && !call_stack.empty()) call_stack.pop_back();

        u8_step(this->core);

        if (this->ips_ctr++ % 1000 == 0) {
            double cur = get_time();
            if (cur - this->ips_start != 0) this->ips = (1000 / (cur - this->ips_start));
            this->ips_start = cur;
        }
    } else {
        this->timer->tick();
        if (!this->config->real_hardware) this->keyboard->tick_emu();
    }
    this->keyboard->tick();
}

void core_step_loop(std::atomic<bool>& stop) {
    stop = false;

    struct timespec last_ins_time;
    struct timespec current_time;
    double delta_time;

    double interval = 1.0 / mcuptr->cycles_per_second;
    clock_gettime(CLOCK_MONOTONIC, &last_ins_time);

    while (!stop.load()) {
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        delta_time = (current_time.tv_sec - last_ins_time.tv_sec) + (current_time.tv_nsec - last_ins_time.tv_nsec) / 1e9;
        if (delta_time >= interval) {
            mcuptr->core_step();
            last_ins_time = current_time;
        }
    }
}

void mcu::reset() {
    u8_reset(this->core);
    for (int i = 0; i < this->screen->height; i++) write_mem_data(this->core, 0, 0xf800 + i*this->screen->bytes_per_row_real, this->screen->bytes_per_row, 0);
    this->call_stack.clear();
    this->ips_start = get_time();
    this->ips = 0;
    this->ips_ctr = 0;
}

void register_sfr(uint16_t addr, uint16_t len, uint8_t (*callback)(mcu*, uint16_t, uint8_t)) {
    for (int i = 0; i < len; i++)
        mcuptr->sfr_write[addr+i] = callback;
}

// for ImGui SFR editor
ImU8 read_sfr_im(const ImU8*, size_t addr) {
    return read_sfr(mcuptr->core, 0, addr);
}
void write_sfr_im(ImU8*, size_t addr, ImU8 val) {
    write_sfr(mcuptr->core, 0, addr, val);
}

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <vector>
#include <atomic>
#include <ctime>
#include <optional>
#include <unistd.h>

#include "mcu.hpp"
#include "../config/config.hpp"
#include "datalabels.hpp"
#include "../peripheral/standby.hpp"
#include "../peripheral/wdt.hpp"
#include "../peripheral/interrupts.hpp"
#include "../peripheral/timer.hpp"
#include "../peripheral/ltb.hpp"
#include "../peripheral/keyboard.hpp"
#include "../peripheral/bcd.hpp"
extern "C" {
#include "../u8_emu/src/core/core.h"
}
#include "../imgui/imgui.h"

//#define FLASHDEBUG
#define BCD

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
    if (addr > 0xfff) printf("WARNING: Overflown write @ %04XH\nCSR:PC = %X:%04XH (after write)\n", (addr + 0xf000) & 0xffff, core->regs.csr, core->regs.pc);
    else if (mcuptr->sfr_write[addr]) mcuptr->sfr[addr] = mcuptr->sfr_write[addr](mcuptr, addr, val);
    else {
        addr += 0xf000;
        if (mcuptr->wanted_sfrs.find(addr) == mcuptr->wanted_sfrs.end()) mcuptr->wanted_sfrs.insert({addr, {0, 0}});
        ++mcuptr->wanted_sfrs[addr].write;
    }
}


uint8_t write_dsr(mcu *mcu, uint16_t, uint8_t val) {
    mcu->core->regs.dsr = val;
    return val;
}

uint8_t fx5800p_battery(struct u8_core *core, uint8_t seg, uint16_t addr) {
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

// TODO: Actually implement VLS
uint8_t ti_vlsconh(mcu *, uint16_t, uint8_t val) {
    return !val;
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
#ifdef FLASHDEBUG
			printf("%05X = %02x\n", fo + 0x80000, data);
#endif
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
#ifdef FLASHDEBUG
			printf("erase %05X (%02x)\n", fo+0x80000, data);
#endif
			return;
		case 7:
			if (fo == 0xaaa && data == 0xaa) {
				mcuptr->flash_mode = 1;
				return;
			}
			break;
	}
	if (data == 0xf0) {
		printf("reset mode\n");
		mcuptr->flash_mode = 0;
		return;
	}
#ifdef FLASHDEBUG
	if (data == 0xb0) {
		printf("write_flash: Erase Suspend.\n");
		return;
	}
	if (data == 0x30) {
		printf("write_flash: Erase Suspend.\n");
		return;
	}
#endif
	printf("write_flash: unknown JEDEC %05x = %02x\nflash_mode = %d - CSR:PC = %X:%04XH (after write)\n", (int)fo+0x80000, data, mcuptr->flash_mode, core->regs.csr, core->regs.pc);
}

uint8_t read_ram2(struct u8_core *core, uint8_t seg, uint16_t addr) {
    return mcuptr->ram2[addr];
}

void write_ram2(struct u8_core *core, uint8_t seg, uint16_t addr, uint8_t val) {
    mcuptr->ram2[addr] = val;
    if (mcuptr->config->hardware_id == HW_CLASSWIZ_CW && !mcuptr->config->real_hardware && addr >= 0x9000 && addr < 0x97f8) {
        mcuptr->screen->cw_2bpp_toggle = true;
        draw_screen_cw(mcuptr, addr - 0x9000 + 0x800, val);
        mcuptr->screen->cw_2bpp_toggle = false;
    }
}

mcu::mcu(struct u8_core *core, struct config *config, uint8_t *rom, uint8_t *flash, uint8_t *ram, int ramstart, int ramsize, int w, int h) {
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

    this->core->mem.num_regions = 7;
    this->core->mem.regions = (struct u8_mem_reg *)malloc(sizeof(struct u8_mem_reg) * 7);

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
    if (!ram) {
        this->ram = (uint8_t *)malloc(ramsize);
        srand(time(NULL));
        for (int i = 0; i < ramsize; i++) this->ram[i] = rand();
    }
    else this->ram = ram;
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
                    .acc = U8_MACC_FUNC,
                    .read = &read_ram2,
                    .write = &write_ram2
                };
            }

            break;

        // TI MathPrint - LAPIS ML620418A
        case HW_TI_MATHPRINT:
            this->core->u16_mode = true;
            this->cycles_per_second = 1024 * 1024 * 2;

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
            this->sfr[0x60] = 1;

            break;

        // SOLAR II
        case HW_SOLAR_II:
            this->cycles_per_second = 1024 * 1024 * 2;
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
                this->core->mem.regions[4] = (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = true,
                    .addr_l = 0x80000,
                    .addr_h = 0xfffff,
                    .acc = U8_MACC_FUNC,
                    .read = &read_flash,
                    .write = &write_flash
                };

                // PRAM
                this->ram2 = (uint8_t *)malloc(0x8000);
                memset(this->ram2, 0, 0x8000);
                this->core->mem.regions[5] = (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = true,
                    .addr_l = 0x40000,
                    .addr_h = 0x47fff,
                    .acc = U8_MACC_ARR,
                    .array = this->ram2
                };

                // Battery
                this->core->mem.regions[6] = (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = false,
                    .addr_l = 0x100000,
                    .addr_h = 0x100000,
                    .acc = U8_MACC_FUNC,
                    .read = &fx5800p_battery
                };
            }
            break;
    }

    this->labels = new class dlabels(this);

    memset((void *)this->sfr_write, 0, sizeof(this->sfr_write));
    register_sfr(0, 1, &default_write<0xff>);
    this->labels->set_sfr_name({0, 1, "Data segment register (DSR)",
        "DSR is a special function register (SFR) used to retain a data segment.\n"
        "For details of DSR, see \"nX-U8(U16)/100 Core Instruction Manual\"."
        });

    this->standby = new class standby;
    this->wdt = new class wdt(this);
    this->interrupts = new class interrupts(this);
    this->timer = new class sfrtimer(this);
    this->ltb = new class ltb(this);
    this->keyboard = new class keyboard(this, w, h);
    this->battery = new class battery(this->config);
#ifdef BCD
    this->bcd = new class bcd(this);
#endif
    this->screen = new class screen(this);

    this->reset();
    if (this->config->hardware_id == HW_TI_MATHPRINT) {
        this->sfr[0x900] = 0x34;
        register_sfr(0x901, 1, &ti_vlsconh);
    }
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
            call_stack.push_back({(((data >> 8) & 0xf) << 16) | addr, read_reg_er(this->core, 0), read_reg_er(this->core, 2), (this->core->regs.csr << 16) | (this->core->regs.pc+4)});
        // BL ERn
        } else if ((data & 0xff0f) == 0xf003) {
            uint16_t addr = read_mem_code(this->core, this->core->regs.csr, this->core->regs.pc+2, 2);
            call_stack.push_back({(this->core->regs.csr << 16) | read_reg_er(this->core, (data >> 4) & 0xf), read_reg_er(this->core, 0), read_reg_er(this->core, 2), (this->core->regs.csr << 16) | (this->core->regs.pc+4)});
        // PUSH LR
        } else if ((data & 0xf8ff) == 0xf8ce && !call_stack.empty()) call_stack.back().return_addr_ptr = this->core->regs.sp - 4;
        // POP PC
        else if ((data & 0xf2ff) == 0xf28e && !call_stack.empty()) call_stack.pop_back();
        // RT / RTI
        else if ((data == 0xfe1f || data == 0xfe0f) && !call_stack.empty()) call_stack.pop_back();
        // BRK
        else if (data == 0xffff) {
            if ((this->core->regs.psw & 3) >= 2) call_stack.clear();
            else call_stack.push_back({read_mem_code(this->core, 0, 4, 2), read_reg_er(this->core, 0), read_reg_er(this->core, 2), (this->core->regs.csr << 16) | (this->core->regs.pc+2), 0, {"BRK", true}});
        }
        // SWI #snum
        else if ((data & 0xffc0) == 0xe500) {
            uint8_t snum = data & 0x3f;
            char s[8]; sprintf(s, "SWI #%d", snum);
            call_stack.push_back({read_mem_code(this->core, 0, 0x80+snum<<1, 2), read_reg_er(this->core, 0), read_reg_er(this->core, 2), (this->core->regs.csr << 16) | (this->core->regs.pc+2), 0, {std::string(s)}});
        }

        u8_step(this->core);
        if (this->core->last_read_size && !this->core->last_read_success) {
            if (wanted_sfrs.find(this->core->last_read) == wanted_sfrs.end()) wanted_sfrs.insert({this->core->last_read, {0, 0}});
            ++wanted_sfrs[this->core->last_read].read;
        }
        if (this->core->last_write_size && !this->core->last_write_success) {
            printf("Write: %05X, %X:%04XH\n", this->core->last_write, this->core->regs.csr, this->core->regs.pc);
            if (wanted_sfrs.find(this->core->last_write) == wanted_sfrs.end()) wanted_sfrs.insert({this->core->last_write, {0, 0}});
            ++wanted_sfrs[this->core->last_write].write;
        }

        if (this->ips_ctr++ % 1000 == 0) {
            double cur = get_time();
            if (cur - this->ips_start != 0) this->ips = (1000 / (cur - this->ips_start));
            this->ips_start = cur;
        }
    } else {
        this->timer->tick();
        if (!this->config->real_hardware && this->config->hardware_id != HW_TI_MATHPRINT) this->keyboard->tick_emu();
    }

    this->wdt->tick();
    if (this->config->hardware_id != HW_TI_MATHPRINT) this->keyboard->tick();
    else this->ltb->tick();
    int_callstack interrupt = this->interrupts->tick();
    if (!interrupt.interrupt_name.empty()) {
        uint8_t elevel = this->core->regs.psw & 3;
        call_stack.push_back({this->core->regs.pc, 0, 0, (this->core->regs.ecsr[elevel-1] << 16) | (this->core->regs.elr[elevel-1]), 0, interrupt});
    }

    if (this->config->hardware_id == HW_TI_MATHPRINT) {
        if (this->config->real_hardware) {
            // TODO
        } else if (this->core->last_swi < 0x40) {
            switch (this->core->last_swi) {
            case 1:
                this->ti_screen_addr = read_reg_er(this->core, 0) - 0xb000;
                write_reg_er(this->core, 0, 0);
                this->ti_screen_changed = true;
                break;
            case 2: {
                std::optional<uint8_t> k;
                if (this->keyboard->enable_keypress) {
                    k = this->keyboard->get_button();
                    if (k.has_value()) this->keyboard->enable_keypress = false;
                }
                write_reg_er(this->core, 0, k.has_value() ? k.value() : 0);
                break;
            }case 4:
                this->ti_status_bar_addr = read_reg_er(this->core, 0) - 0xb000;
                write_reg_er(this->core, 0, 0);
                this->ti_screen_changed = true;
                break;
            }
        }
    }
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
            //if (mcuptr->core->regs.pc == 0x4e9a) stop = true;
            last_ins_time = current_time;
        }
    }
}

void mcu::reset() {
    u8_reset(this->core);
    this->standby->stop_mode = false;
#ifdef BCD
    this->bcd->perApi_Reset();
#endif
    if (this->config->hardware_id == HW_TI_MATHPRINT) {
        this->ti_screen_changed = false;
        this->ti_screen_addr = 0;
        this->ti_status_bar_addr = 0;

        // TODO: Implement the peripherals and move these to their respective peripheral's code
        this->sfr[2] = 0x13;
        this->sfr[3] = 3;
        this->sfr[4] = 2;
        this->sfr[5] = 0x40;
        this->sfr[0xa] = 3;
        this->sfr[0x65] = 6;
        this->sfr[0x64] = 0x30;
        memset(&this->sfr[0x10], 0, 0x3f);
    } else this->screen->reset();
    this->call_stack.clear();
    this->wanted_sfrs.clear();
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

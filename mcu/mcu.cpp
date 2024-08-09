#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>

#include "mcu.hpp"
#include "../config/config.hpp"
extern "C" {
#include "../u8_emu/src/core/core.h"
}

mcu *mcuptr;

void add_mem_region(struct u8_core *core, struct u8_mem_reg reg) {
    ++core->mem.num_regions;

    printf("Add data mem region: Type %d, %s, %05X - %05X\n", reg.type, reg.rw ? "R/W" : "R", reg.addr_l, reg.addr_h);

    if (!mcuptr->core->mem.regions) {
        mcuptr->core->mem.regions = (u8_mem_reg *)malloc(sizeof(struct u8_mem_reg) * core->mem.num_regions);
        if (mcuptr->core->mem.regions) core->mem.regions[0] = reg;
        else {
            printf("Failed.\n");
        }

    } else {
        void *p = realloc((void *)mcuptr->core->mem.regions, sizeof(struct u8_mem_reg) * core->mem.num_regions);
        if (p) {
            core->mem.regions = (u8_mem_reg *)p;
            core->mem.regions[core->mem.num_regions-1] = reg;
        } else {
            printf("Failed.\n");
        }
    }
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

mcu::mcu(struct u8_core *core, struct config *config, uint8_t *rom, uint8_t *flash, int ramstart, int ramsize) {
    this->core = core;
    this->config = config;
    this->rom = rom;
    this->flash = flash;

    mcuptr = this;

    // ROM
    this->core->codemem.num_regions = (this->config->hardware_id == 2 && this->config->is_5800p) ? 2 : 1;
    this->core->codemem.regions = (struct u8_mem_reg *)malloc(sizeof(struct u8_mem_reg) * this->core->codemem.num_regions);
    this->core->codemem.regions[0] = (struct u8_mem_reg){
        .type = U8_REGION_CODE,
        .rw = false,
        .addr_l = 0,
        .addr_h = (this->config->hardware_id == 2 && this->config->is_5800p) ? 0x7ffff : 0xfffff,
        .acc = U8_MACC_ARR,
        .array = this->rom
    };

    // ROM window
    add_mem_region(this->core, (struct u8_mem_reg){
        .type = U8_REGION_DATA,
        .rw = false,
        .addr_l = 0,
        .addr_h = ramstart - 1,
        .acc = U8_MACC_ARR,
        .array = this->rom
    });

    // Main RAM
    this->ram = (uint8_t *)malloc(ramsize);
    memset(this->ram, 0, ramsize);
    add_mem_region(this->core, (struct u8_mem_reg){
        .type = U8_REGION_DATA,
        .rw = true,
        .addr_l = ramstart,
        .addr_h = ramstart + ramsize - 1,
        .acc = U8_MACC_ARR,
        .array = this->ram
    });

    // SFRs
    this->sfr = (uint8_t *)malloc(0x1000);
    memset(this->sfr, 0, 0x1000);
    add_mem_region(this->core, (struct u8_mem_reg){
        .type = U8_REGION_DATA,
        .rw = true,
        .addr_l = 0xf000,
        .addr_h = 0xffff,
        .acc = U8_MACC_FUNC,
        .read = &read_sfr,
        .write = &write_sfr
    });

    switch (this->config->hardware_id) {
        // ClassWiz
        case 4:  // LAPIS ML620606
        case 5:  // LAPIS ML620609
            // Code segment 1+ mirror
            add_mem_region(this->core, (struct u8_mem_reg){
                .type = U8_REGION_DATA,
                .rw = false,
                .addr_l = 0x10000,
                .addr_h = this->config->hardware_id == 5 ? 0x7ffff : 0x3ffff,
                .acc = U8_MACC_ARR,
                .array = (uint8_t *)(this->rom + 0x10000)
            });


            if (this->config->real_hardware) {
                // Code segment 0 mirror
                add_mem_region(this->core, (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = false,
                    .addr_l = this->config->hardware_id == 5 ? 0x80000 : 0x50000,
                    .addr_h = this->config->hardware_id == 5 ? 0x8ffff : 0x5ffff,
                    .acc = U8_MACC_ARR,
                    .array = this->rom
                });

                this->core->u16_mode = true;

            } else {
                // Segment 4/8 [emulator]
                this->ram2 = (uint8_t *)malloc(0x10000);
                memset(this->ram2, 0, 0x10000);
                add_mem_region(this->core, (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = true,
                    .addr_l = this->config->hardware_id == 5 ? 0x80000 : 0x40000,
                    .addr_h = this->config->hardware_id == 5 ? 0x8ffff : 0x4ffff,
                    .acc = U8_MACC_ARR,
                    .array = this->ram2
                });
            }

            break;

        // TI MathPrint - LAPIS ML620418A
        case 6:
            // Code segment 1+ mirror
            add_mem_region(this->core, (struct u8_mem_reg){
                .type = U8_REGION_DATA,
                .rw = false,
                .addr_l = 0x10000,
                .addr_h = 0x3ffff,
                .acc = U8_MACC_ARR,
                .array = (uint8_t *)(this->rom + 0x10000)
            });

            // Code segment 0+ mirror 2
            add_mem_region(this->core, (struct u8_mem_reg){
                .type = U8_REGION_DATA,
                .rw = false,
                .addr_l = 0x80000,
                .addr_h = 0xaffff,
                .acc = U8_MACC_ARR,
                .array = this->rom
            });

            break;

        // SOLAR II
        case 0:
            this->core->small_mm = true;
            break;

        // ES, ES PLUS
        default:
            // Code segment 1 mirror
            add_mem_region(this->core, (struct u8_mem_reg){
                .type = U8_REGION_DATA,
                .rw = false,
                .addr_l = 0x10000,
                .addr_h = 0x1ffff,
                .acc = U8_MACC_ARR,
                .array = (uint8_t *)(this->rom + 0x10000)
            });

            if (!this->config->old_esp) {
                // Code segment 8 mirror
                add_mem_region(this->core, (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = false,
                    .addr_l = 0x80000,
                    .addr_h = 0x8ffff,
                    .acc = U8_MACC_ARR,
                    .array = this->rom
                });
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
                add_mem_region(this->core, (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = true,
                    .addr_l = 0x80000,
                    .addr_h = 0xfffff,
                    .acc = U8_MACC_FUNC,
                    .read = &read_flash,
                    .write = &write_flash
                });

                // PRAM
                this->ram2 = (uint8_t *)malloc(0x80000);
                memset(this->ram2, 0, 0x80000);
                add_mem_region(this->core, (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = true,
                    .addr_l = 0x40000,
                    .addr_h = 0x47fff,
                    .acc = U8_MACC_ARR,
                    .array = this->ram2
                });

                // Battery
                add_mem_region(this->core, (struct u8_mem_reg){
                    .type = U8_REGION_DATA,
                    .rw = false,
                    .addr_l = 0x100000,
                    .addr_h = 0x100000,
                    .acc = U8_MACC_FUNC,
                    .read = &battery
                });
            }
            break;
    }
}

void mcu::core_step(void) {
    write_mem_data(this->core, 0, 0xf000, 1, this->core->regs.dsr);

    uint8_t wdp = read_mem_data(this->core, 0, 0xf00e, 1) & 1;

    this->core->regs.csr &= (this->config->real_hardware && this->config->hardware_id == 3) ? 1 : 0xf;
    u8_step(this->core);
}

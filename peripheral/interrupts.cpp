#include "../mcu/mcu.hpp"
#include "../config/config.hpp"
#include "interrupts.hpp"

interrupts::interrupts(class mcu *mcu, struct config *config) {
    this->mcu = mcu;
    this->config = config;

    switch (this->config->hardware_id) {
    case HW_SOLAR_II:
        this->intr_tbl.insert({"XI0INT",     {0x08, 0x10, 0, 0x14, 0}});
        this->intr_tbl.insert({"TM0INT",     {0x0a, 0x10, 1, 0x14, 1}});
        this->intr_tbl.insert({"L256SINT",   {0x0c, 0x10, 2, 0x14, 2}});
        this->intr_tbl.insert({"L1024SINT",  {0x0e, 0x10, 3, 0x14, 3}});
        this->intr_tbl.insert({"L4096SINT",  {0x10, 0x10, 4, 0x14, 4}});
        this->intr_tbl.insert({"L16384SINT", {0x12, 0x10, 5, 0x14, 5}});
        register_sfr(0x10, 1, &default_write<0x3f>);
        register_sfr(0x14, 1, &default_write<0x3f>);
        break;
    case HW_CLASSWIZ_CW:
        this->intr_tbl.insert({"WDTINT",     {0x08, 0x00, 0, 0x14, 0}});
        this->intr_tbl.insert({"XI0INT",     {0x0a, 0x10, 1, 0x14, 1}});
        this->intr_tbl.insert({"XI1INT",     {0x0c, 0x10, 2, 0x14, 2}});
        this->intr_tbl.insert({"XI2INT",     {0x0e, 0x10, 3, 0x14, 3}});
        this->intr_tbl.insert({"XI3INT",     {0x10, 0x10, 4, 0x14, 4}});
        this->intr_tbl.insert({"TM0INT",     {0x12, 0x10, 5, 0x14, 5}});
        this->intr_tbl.insert({"L256SINT",   {0x14, 0x10, 6, 0x14, 6}});
        this->intr_tbl.insert({"L1024SINT",	 {0x16, 0x10, 7, 0x14, 7}});
        this->intr_tbl.insert({"L4096SINT",	 {0x18, 0x11, 0, 0x15, 0}});
        this->intr_tbl.insert({"L16384SINT", {0x1a, 0x11, 1, 0x15, 1}});
        this->intr_tbl.insert({"SIO0INT",    {0x1c, 0x11, 2, 0x15, 2}});
        this->intr_tbl.insert({"I2C0INT",    {0x1e, 0x11, 3, 0x15, 3}});
        this->intr_tbl.insert({"I2C1INT",    {0x20, 0x11, 4, 0x15, 4}});
        this->intr_tbl.insert({"BENDINT",    {0x22, 0x11, 5, 0x15, 5}});
        this->intr_tbl.insert({"BLOWINT",    {0x24, 0x11, 6, 0x15, 6}});
        this->intr_tbl.insert({"RTCINT",     {0x26, 0x11, 7, 0x15, 7}});
        this->intr_tbl.insert({"AL0INT",     {0x28, 0x12, 0, 0x16, 0}});
        this->intr_tbl.insert({"AL1INT",     {0x2a, 0x12, 1, 0x16, 1}});
        this->intr_tbl.insert({"UA0INT",     {0x2c, 0x12, 2, 0x16, 2}});
        // Unknown interrupts
        /*
        this->intr_tbl.insert({"___INT",     {0x2e, 0x12, 3, 0x16, 3}});
        this->intr_tbl.insert({"___INT",     {0x30, 0x12, 4, 0x16, 4}});
        this->intr_tbl.insert({"___INT",     {0x32, 0x12, 5, 0x16, 5}});
        */
        register_sfr(0x10, 2, &default_write<0xff>);
        register_sfr(0x12, 1, &default_write<0x3f>);
        register_sfr(0x14, 2, &default_write<0xff>);
        register_sfr(0x16, 1, &default_write<0x3f>);
        break;
    default:
        this->intr_tbl.insert({"WDTINT",     {0x08, 0x00, 0, 0x14, 0}});
        this->intr_tbl.insert({"XI0INT",     {0x0a, 0x10, 1, 0x14, 1}});
        this->intr_tbl.insert({"XI1INT",     {0x0c, 0x10, 2, 0x14, 2}});
        this->intr_tbl.insert({"XI2INT",     {0x0e, 0x10, 3, 0x14, 3}});
        this->intr_tbl.insert({"XI3INT",     {0x10, 0x10, 4, 0x14, 4}});
        this->intr_tbl.insert({"TM0INT",     {0x12, 0x10, 5, 0x14, 5}});
        this->intr_tbl.insert({"L256SINT",   {0x14, 0x10, 6, 0x14, 6}});
        this->intr_tbl.insert({"L1024SINT",	 {0x16, 0x10, 7, 0x14, 7}});
        this->intr_tbl.insert({"L4096SINT",	 {0x18, 0x11, 0, 0x15, 0}});
        this->intr_tbl.insert({"L16384SINT", {0x1a, 0x11, 1, 0x15, 1}});
        this->intr_tbl.insert({"SIO0INT",    {0x1c, 0x11, 2, 0x15, 2}});
        this->intr_tbl.insert({"I2C0INT",    {0x1e, 0x11, 3, 0x15, 3}});
        this->intr_tbl.insert({"I2C1INT",    {0x20, 0x11, 4, 0x15, 4}});
        this->intr_tbl.insert({"BENDINT",    {0x22, 0x11, 5, 0x15, 5}});
        this->intr_tbl.insert({"BLOWINT",    {0x24, 0x11, 6, 0x15, 6}});
        this->intr_tbl.insert({"RTCINT",     {0x26, 0x11, 7, 0x15, 7}});
        this->intr_tbl.insert({"AL0INT",     {0x28, 0x12, 0, 0x16, 0}});
        this->intr_tbl.insert({"AL1INT",     {0x2a, 0x12, 1, 0x16, 1}});
        register_sfr(0x10, 2, &default_write<0xff>);
        register_sfr(0x12, 1, &default_write<4>);
        register_sfr(0x14, 2, &default_write<0xff>);
        register_sfr(0x16, 1, &default_write<4>);
        break;
    }
}

int_callstack interrupts::tick() {
    int_callstack interrupt{};
    uint8_t elevel;

    if (this->int_timer-- == 0)
        for (const auto &[k, v] : this->intr_tbl)
            if (this->mcu->sfr[v.irq_adrs] & (1 << v.irq_bit)) {
                if (v.vector_adrs == 8) {
                    elevel = 2;
                    interrupt.nmi = true;
                } else elevel = 1;
                if ((this->mcu->sfr[v.ie_adrs] & (1 << v.ie_bit)) && (this->mcu->core->regs.psw & 3) < elevel) {
                    this->mcu->standby->stop_mode = false;
                    interrupt.interrupt_name = k;

                    this->mcu->core->regs.elr[elevel-1] = this->mcu->core->regs.pc;
                    this->mcu->core->regs.ecsr[elevel-1] = this->mcu->core->regs.csr;
                    this->mcu->core->regs.epsw[elevel-1] = this->mcu->core->regs.psw;
                    this->mcu->core->regs.psw &= 0b11111100;
                    this->mcu->core->regs.psw |= elevel;
                    if (elevel == 1) this->mcu->core->regs.psw &= 0b11110111;
                    this->mcu->sfr[v.irq_adrs] &= ~(1 << v.irq_bit);
                    this->mcu->core->regs.csr = 0;
                    this->mcu->core->regs.pc = read_mem_code(this->mcu->core, 0, v.vector_adrs, 2);
                    this->int_timer = 1;
                }
            }

    return interrupt;
}

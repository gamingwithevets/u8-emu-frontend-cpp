#include <optional>
#include <cstdint>
#include "../mcu/mcu.hpp"
#include "../config/config.hpp"
#include "interrupts.hpp"

interrupts::interrupts(class mcu *mcu) {
    this->mcu = mcu;
    this->config = mcu->config;

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
        this->intr_tbl.insert({"UNK2e",      {0x2e, 0x12, 3, 0x16, 3}});
        this->intr_tbl.insert({"UNK30",      {0x30, 0x12, 4, 0x16, 4}});
        this->intr_tbl.insert({"UNK32",      {0x32, 0x12, 5, 0x16, 5}});
        register_sfr(0x10, 2, &default_write<0xff>);
        register_sfr(0x12, 1, &default_write<0x3f>);
        register_sfr(0x14, 2, &default_write<0xff>);
        register_sfr(0x16, 1, &default_write<0x3f>);
        break;
    case HW_TI_MATHPRINT:
        this->intr_tbl.insert({"WDTINT",     {0x08, 0x00, 0, 0x18, 0}});
        this->intr_tbl.insert({"EXI0INT",    {0x10, 0x11, 0, 0x19, 0}});
        this->intr_tbl.insert({"EXI1INT",    {0x12, 0x11, 1, 0x19, 1}});
        this->intr_tbl.insert({"EXI2INT",    {0x14, 0x11, 2, 0x19, 2}});
        this->intr_tbl.insert({"EXI3INT",    {0x16, 0x11, 3, 0x19, 3}});
        this->intr_tbl.insert({"EXI4INT",    {0x18, 0x11, 4, 0x19, 4}});
        this->intr_tbl.insert({"EXI5INT",    {0x1a, 0x11, 5, 0x19, 5}});
        this->intr_tbl.insert({"EXI6INT",    {0x1c, 0x11, 6, 0x19, 6}});
        this->intr_tbl.insert({"EXI7INT",    {0x1e, 0x11, 7, 0x19, 7}});
        this->intr_tbl.insert({"SIO0INT",    {0x20, 0x12, 0, 0x1a, 0}});
        this->intr_tbl.insert({"SIOF0INT",   {0x22, 0x12, 1, 0x1a, 1}});
        this->intr_tbl.insert({"I2C1INT",    {0x26, 0x12, 3, 0x1a, 3}});
        this->intr_tbl.insert({"UA0INT",     {0x28, 0x12, 4, 0x1a, 4}});
        this->intr_tbl.insert({"UA1INT",     {0x2a, 0x12, 5, 0x1a, 5}});
        this->intr_tbl.insert({"UAF0INT",    {0x2c, 0x12, 6, 0x1a, 6}});
        this->intr_tbl.insert({"UAF1INT",    {0x2e, 0x12, 7, 0x1a, 7}});
        this->intr_tbl.insert({"I2CF0INT",   {0x30, 0x13, 0, 0x1b, 0}});
        this->intr_tbl.insert({"I2CF1INT",   {0x32, 0x13, 1, 0x1b, 1}});
        this->intr_tbl.insert({"LOSCINT",    {0x3a, 0x13, 5, 0x1b, 5}});
        this->intr_tbl.insert({"VLSINT",     {0x3c, 0x13, 6, 0x1b, 6}});
        this->intr_tbl.insert({"MD0INT",     {0x3e, 0x13, 7, 0x1b, 7}});
        this->intr_tbl.insert({"SADINT",     {0x40, 0x14, 0, 0x1c, 0}});
        this->intr_tbl.insert({"RADINT",     {0x42, 0x14, 1, 0x1c, 1}});
        this->intr_tbl.insert({"CMP0INT",    {0x48, 0x14, 4, 0x1c, 4}});
        this->intr_tbl.insert({"CMP1INT",    {0x4a, 0x14, 5, 0x1c, 5}});
        this->intr_tbl.insert({"TM0INT",     {0x50, 0x15, 0, 0x1d, 0}});
        this->intr_tbl.insert({"TM1INT",     {0x52, 0x15, 1, 0x1d, 1}});
        this->intr_tbl.insert({"TM2INT",     {0x54, 0x15, 2, 0x1d, 2}});
        this->intr_tbl.insert({"TM3INT",     {0x56, 0x15, 3, 0x1d, 3}});
        this->intr_tbl.insert({"TM4INT",     {0x58, 0x15, 4, 0x1d, 4}});
        this->intr_tbl.insert({"TM5INT",     {0x5a, 0x15, 5, 0x1d, 5}});
        this->intr_tbl.insert({"TM6INT",     {0x5c, 0x15, 6, 0x1d, 6}});
        this->intr_tbl.insert({"TM7INT",     {0x5e, 0x15, 7, 0x1d, 7}});
        this->intr_tbl.insert({"FTM0INT",    {0x60, 0x16, 0, 0x1e, 0}});
        this->intr_tbl.insert({"FTM1INT",    {0x62, 0x16, 1, 0x1e, 1}});
        this->intr_tbl.insert({"FTM2INT",    {0x64, 0x16, 2, 0x1e, 2}});
        this->intr_tbl.insert({"FTM3INT",    {0x66, 0x16, 3, 0x1e, 3}});
        this->intr_tbl.insert({"RTC1INT",    {0x68, 0x16, 4, 0x1e, 4}});
        this->intr_tbl.insert({"AL11INT",    {0x6a, 0x16, 5, 0x1e, 5}});
        this->intr_tbl.insert({"TM1KINT",    {0x6e, 0x16, 7, 0x1e, 7}});
        this->intr_tbl.insert({"LTB0INT",    {0x70, 0x17, 0, 0x1f, 0}});
        this->intr_tbl.insert({"LTB1INT",    {0x72, 0x17, 1, 0x1f, 1}});
        this->intr_tbl.insert({"LTB2INT",    {0x74, 0x17, 2, 0x1f, 2}});
        this->intr_tbl.insert({"LCDINT",     {0x76, 0x17, 3, 0x1f, 3}});
        this->intr_tbl.insert({"RTC2INT",    {0x78, 0x17, 4, 0x1f, 4}});
        this->intr_tbl.insert({"AL12INT",    {0x7a, 0x17, 5, 0x1f, 5}});
        this->intr_tbl.insert({"RTC0INT",    {0x7c, 0x17, 6, 0x1f, 6}});
        this->intr_tbl.insert({"AL10INT",    {0x7e, 0x17, 7, 0x1f, 7}});
        register_sfr(0x11, 1, &default_write<0xff>);
        register_sfr(0x12, 1, &default_write<0xfb>);
        register_sfr(0x13, 1, &default_write<0xe3>);
        register_sfr(0x14, 1, &default_write<0x33>);
        register_sfr(0x15, 1, &default_write<0xff>);
        register_sfr(0x16, 1, &default_write<0xbf>);
        register_sfr(0x17, 1, &default_write<0xff>);
        register_sfr(0x18, 1, &default_write<1>);
        register_sfr(0x19, 1, &default_write<0xff>);
        register_sfr(0x1a, 1, &default_write<0xfb>);
        register_sfr(0x1b, 1, &default_write<0xe3>);
        register_sfr(0x1c, 1, &default_write<0x33>);
        register_sfr(0x1d, 1, &default_write<0xff>);
        register_sfr(0x1e, 1, &default_write<0xbf>);
        register_sfr(0x1f, 1, &default_write<0xff>);
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
        if ((this->config->hardware_id == HW_ES_PLUS && !this->config->old_esp) || this->config->hardware_id != HW_ES) {
            this->intr_tbl.insert({"SIO0INT",    {0x1c, 0x11, 2, 0x15, 2}});
            this->intr_tbl.insert({"I2C0INT",    {0x1e, 0x11, 3, 0x15, 3}});
            this->intr_tbl.insert({"I2C1INT",    {0x20, 0x11, 4, 0x15, 4}});
            this->intr_tbl.insert({"BENDINT",    {0x22, 0x11, 5, 0x15, 5}});
            this->intr_tbl.insert({"BLOWINT",    {0x24, 0x11, 6, 0x15, 6}});
            this->intr_tbl.insert({"RTCINT",     {0x26, 0x11, 7, 0x15, 7}});
            this->intr_tbl.insert({"AL0INT",     {0x28, 0x12, 0, 0x16, 0}});
            this->intr_tbl.insert({"AL1INT",     {0x2a, 0x12, 1, 0x16, 1}});
            register_sfr(0x10, 2, &default_write<0xff>);
            register_sfr(0x12, 1, &default_write<3>);
            register_sfr(0x14, 2, &default_write<0xff>);
            register_sfr(0x16, 1, &default_write<3>);
        } else {
            register_sfr(0x10, 1, &default_write<0xff>);
            register_sfr(0x11, 1, &default_write<3>);
            register_sfr(0x14, 1, &default_write<0xff>);
            register_sfr(0x15, 1, &default_write<3>);
        }
        break;
    }
}

int_callstack interrupts::tick() {
    int_callstack interrupt{};
    uint8_t elevel;

    if (this->int_timer-- == 0)
        for (const auto &[k, v] : this->intr_tbl)
            if (this->mcu->sfr[v.irq_adrs] & (1 << v.irq_bit)) {
                if (this->config->hardware_id == HW_ES || (this->config->hardware_id == HW_ES_PLUS && this->config->old_esp)) {
                   this->mcu->standby->stop_mode = false;
                   break;
                }
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
                break;
            }

    return interrupt;
}

std::optional<std::string> interrupts::find_int(uint16_t vector_adrs) {
    for (const auto& entry : intr_tbl) {
        if (entry.second.vector_adrs == vector_adrs) return entry.first;
    }
    return std::nullopt;
}

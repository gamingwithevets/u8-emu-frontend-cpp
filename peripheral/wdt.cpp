#include <cstdint>
#include <cmath>

#include "../mcu/mcu.hpp"
#include "wdt.hpp"

uint8_t wdtc(mcu *mcu, uint16_t addr, uint8_t val) {
    if (mcu->wdt->wdp && mcu->wdt->wdtcon == 0x5a && val == 0xa5) {
        mcu->wdt->wdt_count = 0;
        mcu->wdt->overflow_count = false;
    }
    mcu->wdt->wdp = !mcu->wdt->wdp;
    mcu->wdt->wdtcon = val;
    return mcu->wdt->wdp;
}

wdt::wdt(class mcu *mcu) {
    this->mcu = mcu;

    this->reset();

    register_sfr(0xe, 1, &wdtc);
    if (this->mcu->config->hardware_id == HW_CLASSWIZ_CW) register_sfr(0xf, 1, &default_write<7>);
    else if (this->mcu->config->hardware_id == HW_TI_MATHPRINT) register_sfr(0xf, 1, &default_write<0x82>);
}

void wdt::reset() {
    if (this->mcu->config->hardware_id != HW_CLASSWIZ_CW && this->mcu->config->hardware_id != HW_TI_MATHPRINT) return;
    this->mcu->sfr[0xe] = 0;
    this->mcu->sfr[0xf] = this->mcu->config->hardware_id == HW_TI_MATHPRINT ? 0x82 : 2;
    this->wdp = false;
    this->overflow_count = false;
    this->wdt_count = 0;
}

void wdt::tick() {
    // TODO: add clock generator
/*
    // Accept 256Hz output
    if (emulator.chipset.HSCLK_output & 0x20) {
        if (++WDT_counter >= 32 * pow(4, data_WDTMOD)) {
            if (!overflow_count) {
                emulator.chipset.RequestNonmaskable();
                data_WDTCON = 0;
                data_WDP = false;
                WDT_counter = 0;
                overflow_count = true;
            }
            else {
                emulator.chipset.Reset();
            }
        }
    }
*/
}

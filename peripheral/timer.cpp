#include <cstdint>
#include "../mcu/mcu.hpp"
#include "timer.hpp"

timer::timer(double tps) {
    this->time_scale = 1;
    this->passed_time = 0;
    this->tps = tps;
    this->last_time = get_time() * 1e9;
}

void timer::tick() {
    uint64_t now = get_time() * 1e9;
    uint64_t passed_ns = now - this->last_time;
    this->last_time = now;
    if (passed_ns < 0) passed_ns = 0;
    if (passed_ns > 1e9) passed_ns = 1e9;

    this->passed_time += passed_ns * this->time_scale * this->tps / 1e9;
    this->ticks = (int)this->passed_time;
    if (this->ticks > 100) this->ticks = 100;
    this->passed_time -= this->ticks;
}

sfrtimer::sfrtimer(class mcu *mcu) {
    this->mcu = mcu;
    this->timer = new class timer(10000);

    if (this->mcu->config->hardware_id == HW_TI_MATHPRINT) {
        register_sfr(0x300, 2, &default_write<0xff>);
        register_sfr(0x310, 2, &default_write<0xff>);

    } else {
        register_sfr(0x20, 4, &default_write<0xff>);
        register_sfr(0x24, 1, &default_write<0xf>);
        register_sfr(0x25, 1, &default_write<1>);
    }
}

void sfrtimer::tick() {
    this->timer->tick();
    uint16_t counter, target;

    if (this->mcu->config->hardware_id == HW_TI_MATHPRINT) {
        counter = (this->mcu->sfr[0x301] << 8) | this->mcu->sfr[0x300];
        target = (this->mcu->sfr[0x311] << 8) | this->mcu->sfr[0x310];
        counter += this->timer->ticks;
        this->mcu->sfr[0x310] = (uint8_t)counter;
        this->mcu->sfr[0x311] = (uint8_t)(counter >> 8);
        if (counter >= target && this->mcu->standby->stop_mode) this->mcu->sfr[0x1d] |= 1;

    } else {
        if (this->mcu->sfr[0x25]) {
            uint16_t counter = (this->mcu->sfr[0x23] << 8) | this->mcu->sfr[0x22];
            uint16_t target = (this->mcu->sfr[0x21] << 8) | this->mcu->sfr[0x20];

            counter += this->timer->ticks;
            this->mcu->sfr[0x22] = (uint8_t)counter;
            this->mcu->sfr[0x23] = (uint8_t)(counter >> 8);

            if (counter >= target && this->mcu->standby->stop_mode) this->mcu->sfr[0x14] |= this->mcu->config->hardware_id == HW_SOLAR_II ? 2 : 0x20;
        }
    }
}

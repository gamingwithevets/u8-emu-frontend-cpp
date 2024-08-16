#include <chrono>
#include <cstdint>
#include "../mcu/mcu.hpp"
#include "timer.hpp"

timer::timer(class mcu *mcu, double tps) {
    this->mcu = mcu;

    this->time_scale = 1;
    this->passed_time = 0;
    this->tps = tps;
    this->last_time = get_time() * 1e9;

    register_sfr(0x20, 4, &default_write<0xff>);
    register_sfr(0x24, 1, &default_write<0xf>);
    register_sfr(0x25, 1, &default_write<1>);
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
    this->passed_time -= ticks;

    if (this->mcu->sfr[0x25]) {
        uint16_t counter = (this->mcu->sfr[0x23] << 8) | this->mcu->sfr[0x22];
        uint16_t target = (this->mcu->sfr[0x21] << 8) | this->mcu->sfr[0x20];

        counter += ticks;
        this->mcu->sfr[0x22] = (uint8_t)counter;
        this->mcu->sfr[0x23] = (uint8_t)(counter >> 8);

        if (counter >= target && this->mcu->standby->stop_mode) this->mcu->sfr[0x14] |= 0x20;
    }
}

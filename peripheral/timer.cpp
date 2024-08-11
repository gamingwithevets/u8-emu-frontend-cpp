#include <chrono>
#include <cstdint>
#include "../mcu/mcu.hpp"
#include "timer.hpp"

timer::timer(class mcu *mcu) {
    this->mcu = mcu;
    this->tps = 10000;

    register_sfr(0x20, 4, &default_write<0xff>);
    register_sfr(0x24, 1, &default_write<0xf>);
    register_sfr(0x25, 1, &default_write<1>);
}

void timer::tick() {
    double now = get_time() * 1e9;
    auto passed_ns = now - this->last_time;
    this->last_time = now;
    if (passed_ns < 0) passed_ns = 0;
    else if (passed_ns > 1e9) passed_ns = 0;

    this->passed_time += passed_ns * this->tps / 1e9;
    auto ticks = (this->passed_time < 100) ? (int)this->passed_time : 100;
    this->passed_time -= ticks;

    if (this->mcu->sfr[0x25]) {
        uint16_t counter = (this->mcu->sfr[0x23] << 8) | this->mcu->sfr[0x22];
        uint16_t target = (this->mcu->sfr[0x21] << 8) | this->mcu->sfr[0x20];

        ++counter;
        this->mcu->sfr[0x22] = (uint8_t)counter;
        this->mcu->sfr[0x23] = (uint8_t)(counter >> 8);

        if (counter >= target && this->mcu->standby->stop_mode) {
            this->mcu->standby->stop_mode = false;
            this->mcu->sfr[0x14] |= 0x20;
        }
    }
}

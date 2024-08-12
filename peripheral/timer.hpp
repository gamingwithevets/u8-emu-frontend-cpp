#pragma once

#include "../mcu/mcu.hpp"
#include "standby.hpp"

class timer {
private:
    class mcu *mcu;
    standby *standby;
    double tps;
    uint64_t last_time;
public:
    int ticks;
    double time_scale;
    double passed_time;
    timer(class mcu *mcu, double tps);
    void tick();
};

#pragma once

#include "../mcu/mcu.hpp"
#include "standby.hpp"

class timer {
private:
    double tps;
    uint64_t last_time;
public:
    int ticks;
    double time_scale;
    double passed_time;
    timer(double tps);
    void tick();
};

class sfrtimer {
private:
    class mcu *mcu;
    standby *standby;
    class timer *timer;
public:
    sfrtimer(class mcu *mcu);
    void tick();
};

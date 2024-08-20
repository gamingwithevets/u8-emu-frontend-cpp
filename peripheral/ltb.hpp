#pragma once

#include "../mcu/mcu.hpp"
#include "standby.hpp"
#include "timer.hpp"

class ltb {
private:
    class mcu *mcu;
    standby *standby;
    class timer *timer;
public:
    ltb(class mcu *mcu);
    void tick();
};

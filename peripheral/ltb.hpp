#pragma once

#include "../mcu/mcu.hpp"
#include "timer.hpp"

class ltb {
private:
    class mcu *mcu;
    class timer *timer;
public:
    ltb(class mcu *mcu);
    void tick();
};

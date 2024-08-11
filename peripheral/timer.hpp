#ifndef TIMER
#define TIMER

#include "../mcu/mcu.hpp"
#include "standby.hpp"

class timer {
private:
    class mcu *mcu;
    standby *standby;
    double tps;
    double last_time;
    double passed_time;
public:
    timer(class mcu *mcu);
    void tick();
};

#endif

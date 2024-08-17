#pragma once

#include <cstdint>

#include "../mcu/mcu.hpp"
#include "../config/config.hpp"

class wdt {
    class mcu *mcu;
    struct config *config;

public:
    bool wdp;

    uint8_t wdtcon;
    size_t wdt_count;
    bool overflow_count;

    wdt(class mcu *mcu);
    void reset();
    void tick();
};

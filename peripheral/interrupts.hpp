#pragma once

#include <iostream>
#include <map>

#include "../mcu/mcu.hpp"
#include "../config/config.hpp"

struct intr_data {
    uint16_t vector_adrs;
    uint16_t ie_adrs;
    uint8_t  ie_bit;
    uint16_t irq_adrs;
    uint8_t  irq_bit;
};

struct int_callstack {
    std::string interrupt_name;
    bool nmi;
};

class interrupts {
public:
    class mcu *mcu;
    struct config *config;
    std::map<std::string, intr_data> intr_tbl;
    char int_timer;
    interrupts(class mcu *mcu);
    int_callstack tick();
};

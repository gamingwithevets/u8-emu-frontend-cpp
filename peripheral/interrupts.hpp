#pragma once

#include <iostream>
#include <map>
#include <cstdint>
#include <optional>

#include "../mcu/mcu.hpp"
#include "../config/config.hpp"

typedef struct {
    uint16_t vector_adrs;
    uint16_t ie_adrs;
    uint8_t  ie_bit;
    uint16_t irq_adrs;
    uint8_t  irq_bit;
} intr_data;

typedef struct {
    std::string interrupt_name;
    bool nmi;
} int_callstack;

class interrupts {
public:
    class mcu *mcu;
    struct config *config;
    std::map<std::string, intr_data> intr_tbl;
    char int_timer;
    interrupts(class mcu *mcu);
    int_callstack tick();
    std::optional<std::string> find_int(uint16_t vector_adrs);
};

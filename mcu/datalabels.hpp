#pragma once
#include <iostream>
#include <map>
#include "mcu.hpp"

typedef struct {
    uint16_t addr;
    uint16_t len;
    std::string name;
    std::string desc;
} dldata;

typedef struct {
    uint16_t addr;
    uint16_t bbits;
    uint16_t wbits;
    std::string bsymbol0;
    std::string bsymbol1;
    std::string wsymbol;
    std::string name;
    std::string desc;
} sfrdata;

class dlabels {
    std::vector<dldata> ramlabels;
    std::vector<sfrdata> sfrlabels;
    std::vector<dldata> ram2labels;
public:
    dlabels(class mcu *mcu);
    void get_name(int type, uint16_t addr, dldata *data);
    void get_sfr_name(uint16_t addr, sfrdata *data);
};

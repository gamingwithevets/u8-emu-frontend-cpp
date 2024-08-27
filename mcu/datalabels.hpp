#pragma once
#include <iostream>
#include <map>
#include "mcu.hpp"

struct dldata {
    uint16_t addr;
    uint16_t len;
    std::string name;
    std::string desc;
};

class dlabels {
    std::vector<dldata> ramlabels;
    std::vector<dldata> sfrlabels;
    std::vector<dldata> ram2labels;
public:
    dlabels(class mcu *mcu);
    void get_name(int type, uint16_t addr, struct dldata *data);
};

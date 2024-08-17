#pragma once

#include "../mcu/mcu.hpp"
#include "../config/config.hpp"

class bcd {
public:
    class mcu *mcu;
    struct config *config;

    uint8_t data_param1[12];
    uint8_t data_param2[12];
    uint8_t data_temp1[12];
    uint8_t data_temp2[12];

    uint8_t data_operator,
    data_type_1,
    data_type_2,
    param1,
    param2,
    param3,
    param4,
    data_F404_copy,
    data_mode,
    data_a,
    data_b,
    data_c,
    data_d,
    data_F402_copy,
    data_F405_copy;
    bool data_repeat_flag;

    bcd(class mcu *mcu);
    void check_BCDCMD();
    void state_manage();
    void calc_sl(int param);
    void calc_sr(int param);
    void exec_calc();
};

#include "bcd.hpp"
#include "../mcu/mcu.hpp"
#include "../config/config.hpp"

uint8_t bcdcon(mcu *mcu, uint16_t addr, uint8_t val) {
    if (val < 1) return 1;
    else if (val > 6) return 6;
    else return val;
}

uint8_t bcdcmd_bcdmcr(mcu *mcu, uint16_t addr, uint8_t val) {
    bcd *bcd = mcu->bcd;
    mcu->sfr[addr] = val;

    bcd->data_mode = 0x3F;
    bcd->data_a = 0;
    bcd->data_b = 0;
    bcd->data_c = 0;
    bcd->data_d = 0;
    bcd->data_F404_copy = 0;
    bcd->data_operator = 0;
    bcd->data_type_1 = 0;
    bcd->data_type_2 = 0;
    bcd->param1 = 0;
    bcd->param2 = 0;
    bcd->param3 = 0;
    if (mcu->sfr[0x400] != 0xFF) {
        bcd->check_BCDCMD();
        bcd->param4 = 0;
        mcu->sfr[0x400] = 0xFF;
    }
    bcd->data_F402_copy = mcu->sfr[0x402];
    if (mcu->sfr[0x405] & 0x7F) {
        bcd->data_F405_copy = mcu->sfr[0x405];
        bcd->data_F404_copy = mcu->sfr[0x404];
        mcu->sfr[0x405] = 0;
        bcd->data_mode = 0xFF;
    }
    do {
        printf("%d\n", bcd->data_repeat_flag);
        bcd->data_repeat_flag = !(bcd->param1 == 0 && bcd->data_mode == 0x3F);
        bcd->state_manage();
        bcd->param3 = bcd->param2;
        bcd->param2 = bcd->param1;
        bcd->exec_calc();
    } while (bcd->data_repeat_flag);
    return mcu->sfr[addr];
}

bcd::bcd(class mcu *mcu, struct config *config) {
    this->mcu = mcu;
    this->config = config;

    if (this->config->hardware_id != HW_CLASSWIZ_CW) return;

    register_sfr(0x400, 1, &bcdcmd_bcdmcr);
    register_sfr(0x404, 1, &default_write<0x1f>);
    register_sfr(0x405, 1, &bcdcmd_bcdmcr);
    register_sfr(0x410, 1, &default_write<0xbf>);
	for (int i = 0; i < 4; i++) register_sfr(0x480 + i*0x20, 12, &default_write<0xff>);
}

void bcd::check_BCDCMD() {
    data_operator = (mcu->sfr[0x400] >> 4) & 0x0F;
    data_type_2 = (mcu->sfr[0x400] >> 2) & 0x03;
    data_type_1 = mcu->sfr[0x400] & 0x03;
    if (data_operator == 0) {
        param1 = 0;
        param2 = 1;
    }
    else {
        param1 = 1;
    }
    return;
}

uint16_t RegAdr(uint8_t reg_num, uint8_t reg_pos) {
    return 0x480 + reg_num * 0x20 + reg_pos;
}
void bcd::state_manage() {
    if (data_mode == 0xFF && param1 == 0) {
        if (data_c) {
            data_mode = (mcu->sfr[RegAdr(0, 0)] & 0x0F) + 0x20;
            data_type_1 = 0;
            data_type_2 = 0;
            data_operator = 0x0D;
            param1 = 1;
            param4 = 0;
            data_F405_copy = 0;
        }
        else if (data_b) {
            data_mode = 0x18;
            data_type_1 = 1;
            data_type_2 = 0;
            data_operator = 0x08;
            param1 = 1;
            param4 = 0;
            data_F405_copy = 0;
        }
        else if (data_a) {
            data_mode = 0x18;
            data_type_1 = 1;
            data_type_2 = 0;
            data_operator = 0x0C;
            param1 = 1;
            param4 = 0;
            data_F405_copy = 0;
        }
        else {
            switch (((data_F405_copy >> 1) & 0x0F) - 1) {
            case 0:
                if (data_F405_copy & 0x01) {
                    data_mode = (mcu->sfr[RegAdr(0, 0)] & 0x0F) + 0x20;
                    data_type_1 = 0;
                    data_type_2 = 0;
                    data_operator = 0x0D;
                    data_c = 1;
                    param1 = 1;
                    param4 = 0;
                    data_F405_copy = 0;
                }
                else {
                    data_mode = 0x18;
                    data_type_1 = 0x03;
                    data_type_2 = 0x01;
                    data_operator = 0x0B;
                    data_c = 1;
                    param1 = 1;
                    param4 = 0;
                    data_F405_copy = 0;
                }
                break;
            case 1:
                if (data_F405_copy & 0x01) {
                    data_mode = 0x18;
                    data_type_1 = 1;
                    data_type_2 = 0;
                    data_operator = 0x08;
                    data_b = 1;
                    param1 = 1;
                    param4 = 0;
                    data_F405_copy = 0;
                }
                else {
                    data_mode = 0x10;
                    data_type_1 = 0x03;
                    data_type_2 = 0x01;
                    data_operator = 0x0B;
                    data_b = 1;
                    param1 = 1;
                    param4 = 0;
                    data_F405_copy = 0;
                }
                break;
            case 2:
                if (data_F405_copy & 0x01) {
                    data_mode = 0x18;
                    data_type_1 = 1;
                    data_type_2 = 0;
                    data_operator = 0x0C;
                    data_a = 1;
                    param1 = 1;
                    param4 = 0;
                    data_F405_copy = 0;
                }
                else {
                    data_mode = 0x20;
                    data_type_1 = 0x03;
                    data_type_2 = 0x01;
                    data_operator = 0x0B;
                    data_a = 1;
                    param1 = 1;
                    param4 = 0;
                    data_F405_copy = 0;
                }
                break;
            case 3:
            case 4:
            case 5:
            case 6:
                data_F404_copy += 1;
                if (data_F404_copy < 2) {
                    data_type_2 = 0;
                }
                else if (data_F404_copy < 4) {
                    data_type_2 = 1;
                }
                else if (data_F404_copy < 8) {
                    data_type_2 = 2;
                }
                else {
                    data_type_2 = 3;
                }
                if ((data_F405_copy & 0x0C) == 8) {
                    data_operator = 0x08;
                }
                else {
                    data_operator = 0x09;
                }
                data_mode = 0;
                data_type_1 = data_F405_copy & 0x03;
                data_F404_copy += 0xFF << data_type_2;
                if (data_F404_copy) {
                    data_d = 1;
                }
                else {
                    data_d = 0;
                    data_mode = 0x3F;
                }
                param1 = 1;
                param4 = 0;
                data_F405_copy = 0;
                break;
            default:
                data_type_1 = 0;
                data_type_2 = 0;
                data_operator = 0;
                data_a = 1;
                param1 = 1;
                param4 = 0;
                data_F405_copy = 0;
                break;
            }
        }
    }
    else {
        if (((data_a | data_b | data_c | data_d) != 0) && param1 == 0) {
            if (data_c != 0) {
                switch (data_mode - 0x18) {
                case 0:
                    data_mode = 0x19;
                    data_type_1 = 0x02;
                    data_type_2 = 0x01;
                    data_operator = 0x0B;
                    break;
                case 1:
                    data_mode = 0x1A;
                    data_type_1 = 0x02;
                    data_type_2 = 0x02;
                    data_operator = 0x01;
                    break;
                case 2:
                    data_mode = 0x1B;
                    data_type_1 = 0x02;
                    data_type_2 = 0x02;
                    data_operator = 0x01;
                    break;
                case 3:
                    data_mode = 0x1C;
                    data_type_1 = 0x01;
                    data_type_2 = 0x00;
                    data_operator = 0x0A;
                    break;
                case 4:
                    data_mode = (mcu->sfr[RegAdr(0, 0)] & 0x0F) + 0x20;
                    data_type_1 = 0x00;
                    data_type_2 = 0x00;
                    data_operator = 0x0D;
                    break;
                case 8:
                    data_mode = 0x3F;
                    data_type_1 = 0x01;
                    data_type_2 = 0x00;
                    data_operator = 0x09;
                    break;
                case 9:
                case 10:
                case 11:
                case 12:
                case 13:
                case 14:
                case 15:
                case 16:
                case 17:
                    data_mode += 0x10;
                    data_type_1 = 0x01;
                    data_type_2 = 0x00;
                    data_operator = 0x09;
                    break;
                case 25:
                    data_mode = 0x3F;
                    data_type_1 = 0x01;
                    data_type_2 = 0x03;
                    data_operator = 0x01;
                    break;
                case 26:
                    data_mode = 0x31;
                    data_type_1 = 0x01;
                    data_type_2 = 0x03;
                    data_operator = 0x01;
                    break;
                case 27:
                    data_mode = 0x34;
                    data_type_1 = 0x01;
                    data_type_2 = 0x03;
                    data_operator = 0x02;
                    break;
                case 28:
                    data_mode = 0x3F;
                    data_type_1 = 0x01;
                    data_type_2 = 0x02;
                    data_operator = 0x01;
                    break;
                case 29:
                    data_mode = 0x31;
                    data_type_1 = 0x01;
                    data_type_2 = 0x02;
                    data_operator = 0x01;
                    break;
                case 30:
                    data_mode = 0x32;
                    data_type_1 = 0x01;
                    data_type_2 = 0x02;
                    data_operator = 0x01;
                    break;
                case 31:
                    data_mode = 0x33;
                    data_type_1 = 0x01;
                    data_type_2 = 0x02;
                    data_operator = 0x01;
                    break;
                case 32:
                    data_mode = 0x34;
                    data_type_1 = 0x01;
                    data_type_2 = 0x02;
                    data_operator = 0x01;
                    break;
                case 33:
                    data_mode = 0x35;
                    data_type_1 = 0x01;
                    data_type_2 = 0x02;
                    data_operator = 0x01;
                    break;
                default:
                    data_mode = 0x3F;
                    data_type_1 = 0x00;
                    data_type_2 = 0x00;
                    data_operator = 0x00;
                    if (data_F404_copy) {
                        data_c = 1;
                    }
                    else {
                        data_c = 0;
                    }
                    break;
                }
                if (data_mode == 0x3F && data_F404_copy) {
                    data_F404_copy -= 1;
                    data_mode = 0xFF;
                }
            }
            else if (data_b == 0 && data_a == 0) {
                if (data_F404_copy < 2) {
                    data_type_2 = 0;
                }
                else if (data_F404_copy < 4) {
                    data_type_2 = 1;
                }
                else if (data_F404_copy < 8) {
                    data_type_2 = 2;
                }
                else {
                    data_type_2 = 3;
                }
                data_mode = 0;
                param1 = 1;
                data_F404_copy += 0xFF << data_type_2;
                if (!data_F404_copy) {
                    data_d = 0;
                    data_mode = 0x3F;
                }
            }
            else {
                uint8_t flag = mcu->sfr[0x410] >> 7;
                uint8_t data_mode_backup = data_mode;
                switch (data_mode) {
                case 0:
                case 3:
                case 6:
                    data_mode = 0x3F;
                    data_type_1 = 0x00;
                    data_type_2 = 0x00;
                    data_operator = 0x00;
                    break;
                case 1:
                    if (flag) {
                        data_mode = 0x3F;
                        data_type_1 = 0x00;
                        data_type_2 = 0x00;
                        data_operator = 0x00;
                    }
                    else {
                        data_mode = 0x00;
                        data_type_1 = 0x01;
                        data_type_2 = 0x03;
                        data_operator = 0x01;
                    }
                    break;
                case 2:
                    if (flag) {
                        data_mode = 0x3F;
                        data_type_1 = 0x00;
                        data_type_2 = 0x00;
                        data_operator = 0x00;
                    }
                    else {
                        data_mode = 0x01;
                        data_type_1 = 0x01;
                        data_type_2 = 0x03;
                        data_operator = 0x01;
                    }
                    break;
                case 4:
                    if (flag) {
                        data_mode = 0x3F;
                        data_type_1 = 0x00;
                        data_type_2 = 0x00;
                        data_operator = 0x00;
                    }
                    else {
                        data_mode = 0x03;
                        data_type_1 = 0x01;
                        data_type_2 = 0x03;
                        data_operator = 0x01;
                    }
                    break;
                case 5:
                    if (flag) {
                        data_mode = 0x3F;
                        data_type_1 = 0x00;
                        data_type_2 = 0x00;
                        data_operator = 0x00;
                    }
                    else {
                        data_mode = 0x04;
                        data_type_1 = 0x01;
                        data_type_2 = 0x03;
                        data_operator = 0x01;
                    }
                    break;
                case 7:
                    if (flag) {
                        data_mode = 0x3F;
                        data_type_1 = 0x00;
                        data_type_2 = 0x00;
                        data_operator = 0x00;
                    }
                    else {
                        data_mode = 0x06;
                        data_type_1 = 0x01;
                        data_type_2 = 0x03;
                        data_operator = 0x01;
                    }
                    break;
                case 8:
                    if (flag) {
                        data_mode = 0x3F;
                        data_type_1 = 0x00;
                        data_type_2 = 0x00;
                        data_operator = 0x00;
                    }
                    else {
                        data_mode = 0x07;
                        data_type_1 = 0x01;
                        data_type_2 = 0x03;
                        data_operator = 0x01;
                    }
                    break;
                case 9:
                    if (flag) {
                        data_mode = 0x08;
                        data_type_1 = 0x01;
                        data_type_2 = 0x03;
                        data_operator = 0x01;
                    }
                    else {
                        data_mode = 0x3F;
                        data_type_1 = 0x00;
                        data_type_2 = 0x00;
                        data_operator = 0x00;
                    }
                    break;
                case 16:
                    data_mode = 0x11;
                    data_type_1 = 0x02;
                    data_type_2 = 0x01;
                    data_operator = 0x0B;
                    break;
                case 17:
                    data_mode = 0x12;
                    data_type_1 = 0x02;
                    data_type_2 = 0x01;
                    data_operator = 0x01;
                    break;
                case 18:
                    data_mode = 0x13;
                    data_type_1 = 0x02;
                    data_type_2 = 0x01;
                    data_operator = 0x01;
                    break;
                case 19:
                    data_mode = 0x14;
                    data_type_1 = 0x01;
                    data_type_2 = 0x00;
                    data_operator = 0x0B;
                    break;
                case 20:
                    data_mode = 0x18;
                    data_type_1 = 0x00;
                    data_type_2 = 0x00;
                    data_operator = 0x0A;
                    break;
                case 24:
                    data_mode = 0x19;
                    data_type_1 = 0x00;
                    data_type_2 = 0x00;
                    data_operator = 0x0B;
                    break;
                case 25:
                    data_mode = 0x1A;
                    data_type_1 = 0x01;
                    data_type_2 = 0x02;
                    data_operator = 0x02;
                    break;
                case 26:
                    if (flag) {
                        data_mode = 0x02;
                        data_type_1 = 0x01;
                        data_type_2 = 0x03;
                        data_operator = 0x01;
                    }
                    else {
                        data_mode = 0x1B;
                        data_type_1 = 0x01;
                        data_type_2 = 0x02;
                        data_operator = 0x02;
                    }
                    break;
                case 27:
                    if (flag) {
                        data_mode = 0x05;
                        data_type_1 = 0x01;
                        data_type_2 = 0x03;
                        data_operator = 0x01;
                    }
                    else {
                        data_mode = 0x09;
                        data_type_1 = 0x01;
                        data_type_2 = 0x02;
                        data_operator = 0x02;
                    }
                    break;
                case 32:
                    data_mode = 0x21;
                    data_type_1 = 0x02;
                    data_type_2 = 0x01;
                    data_operator = 0x0B;
                    break;
                case 33:
                    data_mode = 0x22;
                    data_type_1 = 0x02;
                    data_type_2 = 0x01;
                    data_operator = 0x01;
                    break;
                case 34:
                    data_mode = 0x23;
                    data_type_1 = 0x02;
                    data_type_2 = 0x01;
                    data_operator = 0x01;
                    break;
                case 35:
                    data_mode = 0x24;
                    data_type_1 = 0x01;
                    data_type_2 = 0x03;
                    data_operator = 0x0C;
                    break;
                case 36:
                    data_mode = 0x19;
                    data_type_1 = 0x00;
                    data_type_2 = 0x03;
                    data_operator = 0x08;
                    break;
                default:
                    break;
                }
                if (data_mode == 0x3F) {
                    uint8_t data_tmp = mcu->sfr[RegAdr(0, 0)];
                    mcu->sfr[RegAdr(0, 0)] = ((data_tmp ^ data_mode_backup) & 0x0F) ^ data_tmp;
                    if (data_F404_copy) {
                        data_F404_copy--;
                        if (data_b) {
                            data_mode = 0x18;
                            data_type_1 = 0x01;
                            data_type_2 = 0x00;
                            data_operator = 0x08;
                        }
                        else if (data_a) {
                            data_mode = 0x18;
                            data_type_1 = 0x01;
                            data_type_2 = 0x00;
                            data_operator = 0x0C;
                        }
                    }
                    else {
                        data_a = 0;
                        data_b = 0;
                    }
                }
            }
            if ((data_a | data_b | data_c) != 0) {
                param1 = 1;
                if ((data_operator | data_type_1 | data_type_2) == 0)
                    param1 = 0;
            }
            param4 = 0;
        }
    }
    if ((data_a | data_b | data_c | data_d) != 0) {
        mcu->sfr[0x405] = (mcu->sfr[0x405] & 0x7F) | 0x80;
    }
    else {
        mcu->sfr[0x405] = mcu->sfr[0x405] & 0x7F;
    }
}

void bcd::calc_sl(int param) {
    if (data_type_2 > 3)
        return;
    if (data_type_2 == 0) {
        for (uint8_t offset = 0; offset < 0x0B; offset++) {
            uint16_t addr = RegAdr(data_type_1, 0x0B - offset);
            uint8_t val1 = mcu->sfr[addr];
            uint8_t val2 = mcu->sfr[addr - 1];
            mcu->sfr[addr] = (val1 << 4) | (val2 >> 4);
        }
        uint16_t addr = RegAdr((data_type_1 + 3) & 0x03, 0x0B);
        uint8_t val2 = mcu->sfr[addr];
        addr = RegAdr(data_type_1, 0);
        uint8_t val1 = mcu->sfr[addr];
        if (param == 0) {
            val2 = 0;
        }
        mcu->sfr[addr] = (val1 << 4) | (val2 >> 4);
    }
    else if (data_type_2 == 1) {
        for (uint8_t offset = 0; offset < 0x0B; offset++) {
            uint16_t addr = RegAdr(data_type_1, 0x0B - offset);
            uint8_t val = mcu->sfr[addr - 1];
            mcu->sfr[addr] = val;
        }
        uint16_t addr = RegAdr((data_type_1 + 3) & 0x03, 0x0B);
        uint8_t val = mcu->sfr[addr];
        if (param == 0) {
            val = 0;
        }
        addr = RegAdr(data_type_1, 0);
        mcu->sfr[addr] = val;
    }
    else if (data_type_2 == 2) {
        for (uint8_t offset = 0; offset < 0x0A; offset++) {
            uint16_t addr = RegAdr(data_type_1, 0x09 - offset);
            uint8_t val = mcu->sfr[addr];
            mcu->sfr[(addr + 2)] = val;
        }
        for (int i = 0; i < 2; i++) {
            uint16_t addr = RegAdr((data_type_1 + 3) & 0x03, i + 0x0A);
            uint8_t val = mcu->sfr[addr];
            if (param == 0) {
                val = 0;
            }
            addr = RegAdr(data_type_1, i);
            mcu->sfr[addr] = val;
        }
    }
    else if (data_type_2 == 3) {
        for (uint8_t offset = 0; offset < 0x08; offset++) {
            uint16_t addr = RegAdr(data_type_1, 0x07 - offset);
            uint8_t val = mcu->sfr[addr];
            mcu->sfr[(addr + 4)] = val;
        }
        for (int i = 0; i < 4; i++) {
            uint16_t addr = RegAdr((data_type_1 + 3) & 0x03, i + 0x08);
            uint8_t val = mcu->sfr[addr];
            if (param == 0) {
                val = 0;
            }
            addr = RegAdr(data_type_1, i);
            mcu->sfr[addr] = val;
        }
    }
}

void bcd::calc_sr(int param) {
    if (data_type_2 > 3)
        return;
    if (data_type_2 == 0) {
        for (uint8_t offset = 0; offset < 0x0B; offset++) {
            uint16_t addr = RegAdr(data_type_1, offset);
            uint8_t val1 = mcu->sfr[addr];
            uint8_t val2 = mcu->sfr[addr + 1];
            mcu->sfr[addr] = (val2 << 4) | (val1 >> 4);
        }
        uint16_t addr = RegAdr(data_type_1, 0x0B);
        uint8_t val1 = mcu->sfr[addr];
        addr = RegAdr((data_type_1 + 1) & 0x03, 0);
        uint8_t val2 = mcu->sfr[addr];
        if (param == 0) {
            val2 = 0;
        }
        addr = RegAdr(data_type_1, 0x0B);
        mcu->sfr[addr] = (val2 << 4) | (val1 >> 4);
    }
    else if (data_type_2 == 1) {
        for (uint8_t offset = 0; offset < 0x0B; offset++) {
            uint16_t addr = RegAdr(data_type_1, offset);
            uint8_t val = mcu->sfr[(addr + 1)];
            mcu->sfr[addr] = val;
        }
        uint16_t addr = RegAdr((data_type_1 + 1) & 0x03, 0);
        uint8_t val = mcu->sfr[addr];
        if (param == 0) {
            val = 0;
        }
        addr = RegAdr(data_type_1, 0x0B);
        mcu->sfr[addr] = val;
    }
    else if (data_type_2 == 2) {
        for (uint8_t offset = 0; offset < 0x0A; offset++) {
            uint16_t addr = RegAdr(data_type_1, offset);
            uint8_t val = mcu->sfr[(addr + 2)];
            mcu->sfr[addr] = val;
        }
        for (int i = 0x0A; i < 0x0C; i++) {
            uint16_t addr = RegAdr((data_type_1 + 1) & 0x03, i - 0x0A);
            uint8_t val = mcu->sfr[addr];
            if (param == 0) {
                val = 0;
            }
            addr = RegAdr(data_type_1, i);
            mcu->sfr[addr] = val;
        }
    }
    else if (data_type_2 == 3) {
        for (uint8_t offset = 0; offset < 0x08; offset++) {
            uint16_t addr = RegAdr(data_type_1, offset);
            uint8_t val = mcu->sfr[(addr + 4)];
            mcu->sfr[addr] = val;
        }
        for (int i = 0x08; i < 0x0C; i++) {
            uint16_t addr = RegAdr((data_type_1 + 1) & 0x03, i - 0x08);
            uint8_t val = mcu->sfr[addr];
            if (param == 0) {
                val = 0;
            }
            addr = RegAdr(data_type_1, i);
            mcu->sfr[addr] = val;
        }
    }
}

uint32_t abcd44(uint32_t tmp, uint32_t val1, uint32_t val2, int flag) {
    if (flag == 1)
        tmp ^= 0x01;
    tmp &= 0x01;
    uint32_t val1_tmp = val1 & 0x0F;
    uint32_t val2_tmp = val2 & 0x0F;
    if (flag == 1) {
        val2_tmp = (0xFFFFFFF9 - val2_tmp) & 0x0F;
    }
    val2_tmp += val1_tmp;
    val2_tmp += tmp;
    int f = 0;
    if (val2_tmp >= 0x0A) {
        val2_tmp -= 0x0A;
        f = 1;
    }
    val2_tmp &= 0x0F;
    tmp = val2_tmp;
    val1_tmp = (val1 >> 4) & 0x0F;
    val2_tmp = (val2 >> 4) & 0x0F;
    if (flag == 1) {
        val2_tmp = (0xFFFFFFF9 - val2_tmp) & 0x0F;
    }
    val1_tmp += val2_tmp;
    val1_tmp += f;
    f = 0;
    if (val1_tmp >= 0x0A) {
        val1_tmp -= 0x0A;
        f = 1;
    }
    val1_tmp = val1_tmp << 4;
    tmp |= val1_tmp & 0xF0;
    val1_tmp = (val1 >> 8) & 0x0F;
    val2_tmp = (val2 >> 8) & 0x0F;
    if (flag == 1) {
        val2_tmp = (0xFFFFFFF9 - val2_tmp) & 0x0F;
    }
    val1_tmp += val2_tmp;
    val1_tmp += f;
    f = 0;
    if (val1_tmp >= 0x0A) {
        val1_tmp -= 0x0A;
        f = 1;
    }
    val1_tmp = (val1_tmp << 8) & 0xF00;
    val1 = (val1 >> 12) & 0x0F;
    val2 = (val2 >> 12) & 0x0F;
    tmp |= val1_tmp;
    if (flag == 1) {
        val2 = (0xFFFFFFF9 - val2) & 0x0F;
    }
    val1 += val2;
    val1 += f;
    f = 0;
    if (val1 >= 0x0A) {
        val1 -= 0x0A;
        f = 1;
    }
    val1 = (val1 << 12) & 0xF000;
    tmp |= val1;
    if (flag == 1)
        f ^= 0x01;
    f = f << 0x10;
    f += tmp;
    return f;
}

void bcd::exec_calc() {
    if (param1 == 1 && param4 == 0 && data_F402_copy != 0) {
        bool storeresults = false;
        if (data_operator == 1 || data_operator == 2)
            storeresults = true;
        uint8_t offset = 0;
        uint16_t val1, val2;
        uint32_t tmp = 0;
        int flag;
        int data_F410_tmp = 1;
        for (int i = 0; i * 2 < data_F402_copy; i++) {
            offset = i * 4;
            uint16_t addr1 = RegAdr(data_type_1, offset);
            val1 = mcu->sfr[addr1 + 1] * 0x100 + mcu->sfr[addr1];
            uint16_t addr2 = RegAdr(data_type_2, offset);
            val2 = mcu->sfr[addr2 + 1] * 0x100 + mcu->sfr[addr2];
            if (data_operator == 2) {
                flag = 1;
            }
            else {
                flag = 0;
            }
            uint32_t res = abcd44(tmp, (uint32_t)val1, (uint32_t)val2, flag);
            tmp = (res >> 16) & 1;
            if ((res & 0xFFFF) == 0 && data_F410_tmp != 0) {
                data_F410_tmp = 1;
            }
            else {
                data_F410_tmp = 0;
            }
            if (storeresults) {
                uint16_t storeaddr = RegAdr(data_type_1, offset);
                mcu->sfr[storeaddr] = (uint8_t)(res & 0xFF);
                mcu->sfr[(storeaddr + 1)] = (uint8_t)((res >> 8) & 0xFF);
            }
            offset += 2;
            addr1 = RegAdr(data_type_1, offset);
            addr2 = RegAdr(data_type_2, offset);
            val1 = mcu->sfr[addr1 + 1] * 0x100 + mcu->sfr[addr1];
            val2 = mcu->sfr[addr2 + 1] * 0x100 + mcu->sfr[addr2];
            res = abcd44(tmp, (uint32_t)val1, (uint32_t)val2, flag);
            if (i * 2 + 1 != data_F402_copy) {
                tmp = (res >> 16) & 1;
                if ((res & 0xFFFF) == 0 && data_F410_tmp != 0) {
                    data_F410_tmp = 1;
                }
                else {
                    data_F410_tmp = 0;
                }
            }
            mcu->sfr[0x410] = (uint8_t)(((tmp * 2) | data_F410_tmp) << 6);
            if (storeresults) {
                uint16_t storeaddr = RegAdr(data_type_1, offset);
                if (i * 2 + 1 == data_F402_copy) {
                    mcu->sfr[storeaddr] = 0;
                    mcu->sfr[(storeaddr + 1)] = 0;
                }
                else {
                    mcu->sfr[storeaddr] = (uint8_t)(res & 0xFF);
                    mcu->sfr[(storeaddr + 1)] = (uint8_t)((res >> 8) & 0xFF);
                }
            }
        }
    }
    if (data_operator == 1 || data_operator == 2) {
        if (param2 == 1 || param3 == 1) {
            param4 += 2;
            if (param4 >= data_F402_copy)
                param1 = 0;
        }
    }
    uint8_t sign = data_operator;
    if (param1 == 0) {
        sign = 0;
    }
    else {
        sign &= 0x0F;
    }
    sign -= 8;
    switch (sign) {
    case 0:
        calc_sl(0);
        break;
    case 1:
        calc_sr(0);
        break;
    case 2:
        uint16_t addr;
        for (uint8_t offset = 1; offset < 0x0C; offset++) {
            addr = RegAdr(data_type_1, offset);
            mcu->sfr[addr] = 0;
        }
        addr = RegAdr(data_type_1, 0);
        if (data_type_2 == 3) {
            mcu->sfr[addr] = 5;
        }
        else {
            mcu->sfr[addr] = data_type_2;
        }
        break;
    case 3:
        uint8_t val;
        for (uint8_t offset = 0; offset < 0x0C; offset++) {
            addr = RegAdr(data_type_2, offset);
            val = mcu->sfr[addr];
            addr = RegAdr(data_type_1, offset);
            mcu->sfr[addr] = val;
        }
        break;
    case 4:
        calc_sl(1);
        break;
    case 5:
        calc_sr(1);
        break;
    default:
        break;
    }
    uint8_t start = 0;
    uint8_t end = 0;
    if ((mcu->sfr[0x400] & 0xF0) == 0 || (param3 == 1 && param2 != 1)) {
        uint8_t offset = 0x0B;
        bool flag = true;
        do {
            if (flag == false)
                break;
            uint16_t addr = RegAdr(data_type_1, offset);
            uint8_t value = mcu->sfr[addr];
            if (offset < data_F402_copy * 2) {
                if ((value & 0xF0) != 0) {
                    flag = false;
                }
                else {
                    end++;
                    if ((value & 0x0F) != 0) {
                        flag = false;
                    }
                    else {
                        end++;
                    }
                }
            }
            else {
                end += 2;
            }
            offset--;
        } while (offset != 0xFF);
        offset = 0;
        flag = true;
        do {
            if (flag == false)
                break;
            uint16_t addr = RegAdr(data_type_1, offset);
            uint8_t value = mcu->sfr[addr];
            if (offset < data_F402_copy * 2) {
                if ((value & 0x0F) != 0) {
                    flag = false;
                }
                else {
                    start++;
                    if ((value & 0xF0) != 0) {
                        flag = false;
                    }
                    else {
                        start++;
                    }
                }
            }
            else {
                end += 2;
            }
            offset++;
        } while (offset <= 0x0B);
        mcu->sfr[0x414] = start;
        mcu->sfr[0x415] = end;
    }
    if (mcu->sfr[0x400] != 0 && (mcu->sfr[0x400] & 0x08) == 0)
        return;
    param1 = 0;
    data_F402_copy = 6;
    return;
}

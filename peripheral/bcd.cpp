#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "../mcu/mcu.hpp"
#include "../config/config.hpp"
#include "bcd.hpp"

uint8_t bcdcmd(mcu *mcu, uint16_t addr, uint8_t val) {
    bcd *bcd = mcu->bcd;
    bcd->bcdcmd_req = val;
    if (!bcd->macro_running) bcd->run_command(val);
    else {
        mcu->paused = true;
        bcd->bcdcmd_pend = true;
    }
    return val;
}

uint8_t bcdcon(mcu *mcu, uint16_t, uint8_t val) {
    if (val < 1) return 1;
    if (val > 6) return 6;
    return val;
}

uint8_t bcdmcr(mcu *mcu, uint16_t addr, uint8_t val) {
    bcd *bcd = mcu->bcd;
    bcd->bcdmcr_req = val;
    if (!bcd->macro_running) bcd->start_macro(val);
    else {
        mcu->paused = true;
        bcd->bcdmcr_pend = true;
    }
    return val;
}

uint8_t bcdflg(mcu *mcu, uint16_t addr, uint8_t val) {
    mcu->bcd->carry = val & 0x80;
    mcu->bcd->zero = val & 0x40;
    return val & 0b11000000;
}

bcd::bcd(class mcu *mcu) {
    this->mcu = mcu;
    this->config = mcu->config;

    if (config->hardware_id != HW_CLASSWIZ_CW) return;

    register_sfr(0x400, 1, &bcdcmd);
    register_sfr(0x402, 1, &bcdcon);
    register_sfr(0x404, 1, &default_write<0x1f>);
    register_sfr(0x405, 1, &bcdmcr);
    register_sfr(0x410, 1, &bcdflg);

    for (char i = 0; i < 4; i++) {
        uint16_t addr = 0x480+i*0x20;
        register_sfr(addr, 12, &default_write<0xff>);
        bcdreg[i] = &mcu->sfr[addr];
    }
}

void bcd::tick() {
    if (!macro_running) return;

    fetch:
    uint16_t inst = curr_pgm[pgm_counter];
    uint8_t offset = (inst >> 8) & 0x1F;
    uint8_t cond = 0;

    switch (inst >> 13) {
    case 0:
        pgm_counter = offset;
        break;
    case 1:
        pgm_counter = (pgm_counter + offset) & 0x1F;
        break;
    case 2:
        pgm_counter++;
        cond = 1;
        break;
    case 3:
        pgm_counter++;
        cond = 2;
        break;
    case 4:
        pgm_counter = ((bcdreg[0][0] & 0x0F) + offset) & 0x1F;
        break;
    case 5:
        bcdreg[0][0] = (bcdreg[0][0] & 0xF0) | (pgm_counter & 0x0F);
        pgm_counter = offset;
        if (--bcdmcn) goto fetch;
        macro_running = false;
        break;
    case 6:
        bcdmcn -= offset;
        pgm_counter &= 0xFC;
        if (bcdmcn & 0xF8) pgm_counter |= 3;
        else if (bcdmcn & 0x04) pgm_counter |= 2;
        else if (bcdmcn & 0x02) pgm_counter |= 1;
        else if (!bcdmcn) macro_running = false;
        break;
    case 7:
        pgm_counter = offset;
        if (--bcdmcn) goto fetch;
        macro_running = false;
        break;
    }

    run_command(inst & 0xFF);
    if (cond && ((cond & 1) ^ carry)) pgm_counter = offset;

    if (!macro_running) {
        if (bcdcmd_pend) {
            mcu->paused = false;
            bcdcmd_pend = false;
            run_command(bcdcmd_req);
        }
        if (bcdmcr_pend) {
            mcu->paused = false;
            bcdmcr_pend = false;
            start_macro(bcdmcr_req);
        }
    }
}

void bcd::reset() {
    mcu->sfr[0x400] = mcu->sfr[0x404] = mcu->sfr[0x414] = mcu->sfr[0x415] = 0;
    mcu->sfr[0x402] = 6;

    bcdcmd_req = bcdmcr_req = 0;
    carry = zero = macro_running = bcdcmd_pend = bcdmcr_pend = false;

    curr_pgm = nullptr;
    pgm_counter = 0;
}

char* fmtreg(uint8_t array[12]) {
    static char hex_str[25];
    hex_str[24] = '\0';
    int j = 0;
    bool k = false;

    for (int i = 0; i < 12; i++) {
        if (!k) {
            if (!array[11-i]) continue;
            j += sprintf(hex_str + j, "%X", array[11-i]);
            k = true;
        } else j += sprintf(hex_str + j, "%02X", array[11-i]);
    }

    return hex_str;
}

void bcd::run_command(uint8_t cmd) {
	mcu->sfr[0x400] = cmd;
	uint8_t src = (cmd >> 2) & 3, dst = cmd & 3, op = cmd >> 4;
	bool arithmetic_mode = (op & 0x08) == 0;
	int calc_ptr = (op >= 8 || op == 1 || op == 2) ? 0 : (op == 7 ? 2 : 6);
#ifdef BCDDEBUG
    printf("[BCD] Ran command: ");
    char _dst[8]; sprintf(_dst, "BCDREG%c", 0x41+dst);
    char _src[8]; sprintf(_src, "BCDREG%c", 0x41+src);
    bool nop = false;
    switch (op) {
    case 0:
        printf("NOP");
        nop = true;
        break;
    case 1:
        printf("ADD %s, %s (%s = %s, %s = %s)", _dst, _src, _dst, fmtreg(bcdreg[dst]), _src, fmtreg(bcdreg[src]));
        break;
    case 2:
        printf("SUB %s, %s (%s = %s, %s = %s)", _dst, _src, _dst, fmtreg(bcdreg[dst]), _src, fmtreg(bcdreg[src]));
        break;
    case 7:
        printf("ADDC %s, %s (%s = %s, %s = %s) [!]", _dst, _src, _dst, fmtreg(bcdreg[dst]), _src, fmtreg(bcdreg[src]));
        break;
    case 8:
        printf("SLL %s, #%d (%s = %s)", _dst, 1<<src, _dst, fmtreg(bcdreg[dst]));
        break;
    case 9:
        printf("SRL %s, #%d (%s = %s)", _dst, 1<<src, _dst, fmtreg(bcdreg[dst]));
        break;
    case 0xa:
        printf("MOV %s, #%d", _dst, src == 3 ? 5 : src);
        break;
    case 0xb:
        printf("MOV %s, %s (%s)", _dst, _src, fmtreg(bcdreg[src]));
        break;
    case 0xc:
        printf("SLX %s, #%d (%s = %s)", _dst, 1<<src, _dst, fmtreg(bcdreg[dst]));
        break;
    case 0xd:
        printf("SXX %s, #%d (%s = %s)", _dst, 1<<src, _dst, fmtreg(bcdreg[dst]));
        break;
    case 0xf:
        printf("MOV %s, #0 [!]", _dst);
        break;
    default:
        printf("NOP [!]");
        nop = true;
        break;
    }

    printf("\n");
#endif // BCDDEBUG
	while (calc_ptr < 6) {
		if (calc_ptr == 0) {
			carry = false;
			zero = true;
		}
		bool carry = op == 2 ? !carry : carry;
		uint16_t res = 0, op_src = bcdreg[src][calc_ptr << 1] | uint16_t(bcdreg[src][(calc_ptr << 1) + 1] << 8),
			op_dst = bcdreg[dst][calc_ptr << 1] | uint16_t(bcdreg[dst][(calc_ptr << 1) + 1] << 8);
		for (int i = 0; i < 4; i++) {
			uint8_t op1 = op_src & 0x0F, op2 = op_dst & 0x0F;
			op_src >>= 4;
			op_dst >>= 4;
			if (op == 2) op1 = (9 - op1) & 0x0F;
			op2 += op1 + (carry ? 1 : 0);
			if ((carry = op2 >= 10)) op2 -= 10;
			res |= op2 << (i * 4);
		}
		if (op == 2) carry = !carry;
		if (res) zero = false;
		if (arithmetic_mode) {
			bcdreg[dst][calc_ptr << 1] = res & 0xFF;
			bcdreg[dst][(calc_ptr << 1) + 1] = res >> 8;
			carry = carry;
			calc_ptr++;
			if (op < 7 && calc_ptr >= mcu->sfr[0x402]) break;
		}
		else {
			if (calc_ptr < mcu->sfr[0x402]) carry = carry;
			if (calc_ptr++) break;
		}
	}

	if (!arithmetic_mode)
		switch (op & 0x07) {
		case 0:
			shift_left(src, dst, false);
			break;
		case 1:
			shift_right(src, dst, false);
			break;
		case 2:
			memset(bcdreg[dst], 0, 12);
			bcdreg[dst][0] = src == 3 ? 5 : src;
			break;
		case 3:
			memcpy(bcdreg[dst], bcdreg[src], 12);
			break;
		case 4:
			shift_left(src, dst, true);
			break;
		case 5:
			shift_right(src, dst, true);
			break;
		case 7:
			memset(bcdreg[dst], 0, 12);
			break;
		}

	mcu->sfr[0x414] = 0;
	for (int i = 0; i < 12; i++) {
		if (i >= 2 * mcu->sfr[0x402]) {
			mcu->sfr[0x414] += 2;
			continue;
		}
		if (bcdreg[dst][i] & 0x0F) break;
		mcu->sfr[0x414]++;
		if (bcdreg[dst][i] >> 4) break;
		mcu->sfr[0x414]++;
	}

	mcu->sfr[0x415] = 24 - 4 * mcu->sfr[0x402];
	for (int i = 2 * mcu->sfr[0x402] - 1; i >= 0; i--) {
		if (bcdreg[dst][i] >> 4) break;
		mcu->sfr[0x415]++;
		if (bcdreg[dst][i] & 0x0F) break;
		mcu->sfr[0x415]++;
	}

#ifdef BCDDEBUG
    if (!nop) printf("Result: %s = %s\n", _dst, fmtreg(bcdreg[dst]));
#endif // BCDDEBUG
}

void bcd::start_macro(uint8_t index) {
	bcdmcn = mcu->sfr[0x404] + 1;
	if (index > 0x0F) {
		curr_pgm = nullptr;
		pgm_counter = 0;
	}
	else {
		curr_pgm = (uint16_t *)pgm_ptr[index];
		pgm_counter = pgm_entry[index];
		if (index & 8) {
			if (bcdmcn & 0xF8) pgm_counter |= 3;
			else if (bcdmcn & 0x04) pgm_counter |= 2;
			else if (bcdmcn & 0x02) pgm_counter |= 1;
		}
#ifdef BCDDEBUG
        printf("[BCD] Started ");
        if (curr_pgm == mul_pgm) printf("mul_pgm");
        else if (curr_pgm == div_pgm) printf("div_pgm");
        else if (curr_pgm == divsn_pgm) printf("divsn_pgm");
        else if (curr_pgm == sft_pgm) printf("sft_pgm");
        else printf("null");
        printf(" with program counter = 0x%02X\n", pgm_counter);
#endif
	}
	if (curr_pgm != nullptr) macro_running = true;
}

void bcd::shift_left(uint8_t src, uint8_t dst, bool continuous) {
	if (!src) {
		for (int i = 11; i > 0; i--) bcdreg[dst][i] = (bcdreg[dst][i] << 4) | (bcdreg[dst][i - 1] >> 4);
		bcdreg[dst][0] = (bcdreg[dst][0] << 4) | (continuous ? (bcdreg[(dst + 3) & 3][11] >> 4) : 0);
	}
	else {
		int size = 1 << (src - 1);
		memmove(bcdreg[dst] + size, bcdreg[dst], 12 - size);
		if (continuous)
			memcpy(bcdreg[dst], bcdreg[(dst + 3) & 3] + 12 - size, size);
		else
			memset(bcdreg[dst], 0, size);
	}
}

void bcd::shift_right(uint8_t src, uint8_t dst, bool continuous) {
	if (!src) {
		for (int i = 0; i < 11; i++) bcdreg[dst][i] = (bcdreg[dst][i] >> 4) | (bcdreg[dst][i + 1] << 4);
		bcdreg[dst][11] = (bcdreg[dst][11] >> 4) | (continuous ? (bcdreg[(dst + 1) & 3][0] << 4) : 0);
	}
	else {
		int size = 1 << (src - 1);
		memmove(bcdreg[dst], bcdreg[dst] + size, 12 - size);
		if (continuous)
			memcpy(bcdreg[dst] + 12 - size, bcdreg[(dst + 1) & 3], size);
		else
			memset(bcdreg[dst] + 12 - size, 0, size);
	}
}

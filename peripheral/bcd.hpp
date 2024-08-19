#pragma once

#include <cstdio>
#include "../mcu/mcu.hpp"
#include "../config/config.hpp"

#define BCDDEBUG

using byte = unsigned char;
using uint = unsigned int;
using ushort = unsigned short;

uint8_t bcdcmd_mcr(mcu *mcu, uint16_t addr, uint8_t val);
uint8_t bcdcon(mcu *mcu, uint16_t addr, uint8_t val);

class bcd {
public:
    class mcu *mcu;
    struct config *config;
private:
	int calc_mode;
	int dst;
	int src;
	bool calc_en;
	bool calc_en_d;
	bool calc_en_dd;
	bool divsn_mode;
	bool div_mode;
	bool mul_mode;
	bool sft_mode;
	uint calc_len;
	int calc_pos;
	uint BMC;
	uint macro_state;
	uint macro_cnt;

	const int REG_BCDCMD = 0x400;
	const int REG_BCDCON = 0x402;
	const int REG_BCDMCN = 0x404;
	const int REG_BCDMCR = 0x405;
	const int REG_BCDFLG = 0x410;
	const int REG_BCDLLZ = 0x414;
	const int REG_BCDMLZ = 0x415;

	const int CAL_NOP = 0;
	const int CAL_ADD = 1;
	const int CAL_SUB = 2;
	const int CAL_SL = 8;
	const int CAL_SR = 9;
	const int CAL_CON = 0xa;
	const int CAL_CP = 0xb;
	const int CAL_SLX = 0xc;
	const int CAL_SRX = 0xd;
	const int NOCMD_VAL = 0xff;

	const int MACRO_START = 0xff;
	const int MACRO_END = 0x3f;

	const int STATE_MUL_INIT_0 = 0x18;
	const int STATE_MUL_INIT_1 = 0x19;
	const int STATE_MUL_INIT_2 = 0x1a;
	const int STATE_MUL_INIT_3 = 0x1b;
	const int STATE_MUL_INIT_4 = 0x1c;

	const int STATE_MUL_1 = 0x20;
	const int STATE_MUL_2 = 0x21;
	const int STATE_MUL_3 = 0x22;
	const int STATE_MUL_4 = 0x23;
	const int STATE_MUL_5 = 0x24;
	const int STATE_MUL_6 = 0x25;
	const int STATE_MUL_7 = 0x26;
	const int STATE_MUL_8 = 0x27;
	const int STATE_MUL_9 = 0x28;
	const int STATE_MUL_10 = 0x29;
	const int STATE_MUL_11 = 0x31;
	const int STATE_MUL_12 = 0x32;
	const int STATE_MUL_13 = 0x33;
	const int STATE_MUL_14 = 0x34;
	const int STATE_MUL_15 = 0x35;
	const int STATE_MUL_16 = 0x36;
	const int STATE_MUL_17 = 0x37;
	const int STATE_MUL_18 = 0x38;
	const int STATE_MUL_19 = 0x39;

	const int STATE_DIVSN_INIT_0 = 0x20;
	const int STATE_DIVSN_INIT_1 = 0x21;
	const int STATE_DIVSN_INIT_2 = 0x22;
	const int STATE_DIVSN_INIT_3 = 0x23;
	const int STATE_DIVSN_INIT_4 = 0x24;

	const int STATE_DIV_INIT_0 = 0x10;
	const int STATE_DIV_INIT_1 = 0x11;
	const int STATE_DIV_INIT_2 = 0x12;
	const int STATE_DIV_INIT_3 = 0x13;
	const int STATE_DIV_INIT_4 = 0x14;

	const int STATE_DIV_0 = 24;
	const int STATE_DIV_1 = 25;
	const int STATE_DIV_2 = 26;
	const int STATE_DIV_3 = 2;
	const int STATE_DIV_4 = 1;
	const int STATE_DIV_5 = 0;
	const int STATE_DIV_6 = 27;
	const int STATE_DIV_7 = 5;
	const int STATE_DIV_8 = 4;
	const int STATE_DIV_9 = 3;
	const int STATE_DIV_10 = 9;
	const int STATE_DIV_11 = 8;
	const int STATE_DIV_12 = 7;
	const int STATE_DIV_13 = 6;

public:
	bcd(class mcu *mcu) {
        this->mcu = mcu;
        this->config = mcu->config;

        if (config->hardware_id != HW_CLASSWIZ_CW) return;

        register_sfr(REG_BCDCMD, 1, &bcdcmd_mcr);
        register_sfr(REG_BCDCON, 1, &bcdcon);
        register_sfr(REG_BCDMCN, 1, &default_write<0x3f>);
        register_sfr(REG_BCDMCR, 1, &bcdcmd_mcr);
        register_sfr(REG_BCDFLG, 1, &default_write<0xbf>);
        for (int i = 0; i < 4; i++) register_sfr(0x480 + i*0x20, 12, &default_write<0xff>);
        this->perApi_Reset();
	}

private:
	uint RegAdr(int reg_num, int reg_pos) {
		return (uint)(0x480 + reg_num * 0x20 + reg_pos);
	}

	int RegPrev(int reg_num) {
		return (reg_num - 1 + 4) % 4;
	}

	int RegNext(int reg_num) {
		return (reg_num + 1) % 4;
	}

	uint abcd44(bool m, uint a, uint b, uint ci)
	{
#ifdef BCDDEBUG
        printf("abcd44: m = %d, a = %d, b = %d, ci = %d\n", m, a, b, ci);
#endif // BCDDEBUG
		ci = (m ? (ci ^ 1u) : ci) & 1u;
		uint num = 0u;
		for (int i = 0; i < 4; i++)
		{
			uint num2 = (a >> i * 4) & 0xF;
			uint num3 = (b >> i * 4) & 0xFu;
			if (m)
			{
				num3 = (9 - num3) & 0xFu;
			}
			uint num4 = num2 + num3 + ci;
			ci = ((num4 >= 10) ? 1u : 0u);
			num4 = (uint)(int)(num4 - ((ci != 0) ? 10 : 0)) & 0xFu;
			num |= num4 << i * 4;
		}
		ci = (m ? (ci ^ 1u) : ci);
#ifdef BCDDEBUG
        printf("abcd44: ci = %d, retval = %d\n", ci, (ci << 16) + num);
#endif // BCDDEBUG
		return (ci << 16) + num;
	}

	void calc_sl(bool ex)
	{
#ifdef BCDDEBUG
        printf("calc_sl: ex = %d, dst = %d, src = %d\n", ex, dst, src);
#endif // BCDDEBUG
		byte b = 0;
		byte b2 = 0;
		byte b3 = 0;
		switch (src)
		{
		case 0:
		{
			for (int i = 0; i < 11; i++)
			{
				b2 = mcu->sfr[RegAdr(dst, 11 - i)];
				b3 = mcu->sfr[RegAdr(dst, 10 - i)];
				b = (byte)((b2 << 4) | (b3 >> 4));
				mcu->sfr[RegAdr(dst, 11 - i)] = b;
			}
			b2 = mcu->sfr[RegAdr(dst, 0)];
			if (ex)
			{
				b3 = mcu->sfr[RegAdr(RegPrev(dst), 11)];
			}
			else
			{
				b3 = 0;
			}
			b = (byte)((b2 << 4) | (b3 >> 4));
			mcu->sfr[RegAdr(dst, 0)] = b;
			break;
		}
		case 1:
		{
			for (int i = 0; i < 11; i++)
			{
				b = mcu->sfr[RegAdr(dst, 10 - i)];
				mcu->sfr[RegAdr(dst, 11 - i)] = b;
			}
			if (ex)
			{
				b = mcu->sfr[RegAdr(RegPrev(dst), 11)];
			}
			else
			{
				b = 0;
			}
			mcu->sfr[RegAdr(dst, 0)] = b;
			break;
		}
		case 2:
		{
			for (int i = 0; i < 10; i++)
			{
				b = mcu->sfr[RegAdr(dst, 9 - i)];
				mcu->sfr[RegAdr(dst, 11 - i)] = b;
			}
			for (int i = 0; i < 2; i++)
			{
				if (ex)
				{
					b = mcu->sfr[RegAdr(RegPrev(dst), 10 + i)];
				}
				else
				{
					b = 0;
				}
				mcu->sfr[RegAdr(dst, i)] = b;
			}
			break;
		}
		case 3:
		{
			for (int i = 0; i < 8; i++)
			{
				b = mcu->sfr[RegAdr(dst, 7 - i)];
				mcu->sfr[RegAdr(dst, 11 - i)] = b;
			}
			for (int i = 0; i < 4; i++)
			{
				if (ex)
				{
					b = mcu->sfr[RegAdr(RegPrev(dst), 8 + i)];
				}
				else
				{
					b = 0;
				}
				mcu->sfr[RegAdr(dst, i)] = b;
			}
			break;
		}
		}
	}

	void calc_sr(bool ex)
	{
#ifdef BCDDEBUG
        printf("calc_sr: ex = %d, dst = %d, src = %d\n", ex, dst, src);
#endif // BCDDEBUG
		byte b = 0;
		byte b2 = 0;
		byte b3 = 0;
		switch (src)
		{
		case 0:
		{
			for (int i = 0; i < 11; i++)
			{
				b2 = mcu->sfr[RegAdr(dst, i)];
				b3 = mcu->sfr[RegAdr(dst, i + 1)];
				b = (byte)((b2 >> 4) | (b3 << 4));
				mcu->sfr[RegAdr(dst, i)] = b;
			}
			b2 = mcu->sfr[RegAdr(dst, 11)];
			if (ex)
			{
				b3 = mcu->sfr[RegAdr(RegNext(dst), 0)];
			}
			else
			{
				b3 = 0;
			}
			b = (byte)((b2 >> 4) | (b3 << 4));
			mcu->sfr[RegAdr(dst, 11)] = b;
			break;
		}
		case 1:
		{
			for (int i = 0; i < 11; i++)
			{
				b = mcu->sfr[RegAdr(dst, i + 1)];
				mcu->sfr[RegAdr(dst, i)] = b;
			}
			if (ex)
			{
				b = mcu->sfr[RegAdr(RegNext(dst), 0)];
			}
			else
			{
				b = 0;
			}
			mcu->sfr[RegAdr(dst, 11)] = b;
			break;
		}
		case 2:
		{
			int i;
			for (i = 0; i < 10; i++)
			{
				b = mcu->sfr[RegAdr(dst, i + 2)];
				mcu->sfr[RegAdr(dst, i)] = b;
			}
			for (; i < 12; i++)
			{
				if (ex)
				{
					b = mcu->sfr[RegAdr(RegNext(dst), i - 12 + 2)];
				}
				else
				{
					b = 0;
				}
				mcu->sfr[RegAdr(dst, i)] = b;
			}
			break;
		}
		case 3:
		{
			int i;
			for (i = 0; i < 8; i++)
			{
				b = mcu->sfr[RegAdr(dst, i + 4)];
				mcu->sfr[RegAdr(dst, i)] = b;
			}
			for (; i < 12; i++)
			{
				if (ex)
				{
					b = mcu->sfr[RegAdr(RegNext(dst), i - 12 + 4)];
				}
				else
				{
					b = 0;
				}
				mcu->sfr[RegAdr(dst, i)] = b;
			}
			break;
		}
		}
	}

	void exec_calc()
	{
		exec_Add_Sub();
		exec_Sft_Con_Cp();
		update_LLZ_MLZ();
		check_calc_end();
	}

	void exec_Add_Sub()
	{
		ushort a = 0;
		ushort b = 0;
		uint num = 0u;
		if (calc_en && calc_pos == 0)
		{
			uint num2 = 1u;
			uint num3 = 0u;
			bool flag = calc_mode == 1 || calc_mode == 2;
			for (int i = 0; i < calc_len; i += 2)
			{
				a = (mcu->sfr[RegAdr(dst, i * 2) + 1] << 8) | mcu->sfr[RegAdr(dst, i * 2)];
				b = (mcu->sfr[RegAdr(src, i * 2) + 1] << 8) | mcu->sfr[RegAdr(src, i * 2)];
				num = abcd44(calc_mode == 2, a, b, num3);
				num3 = (num >> 16) & 1u;
				num2 = (((num & 0xFFFF) == 0 && num2 != 0) ? 1u : 0u);
				if (flag)
				{
					mcu->sfr[RegAdr(dst, i * 2) + 1] = num >> 8 & 0xff; mcu->sfr[RegAdr(dst, i * 2)] = num & 0xff;
				}
#ifdef BCDDEBUG
                printf("exec_Add_Sub: i = %d, calc_len = %d, a = %04x, b = %04x, flag = %d\n", i, calc_len, 0xf000+RegAdr(dst, i * 2), 0xf000+RegAdr(src, i * 2), flag);
#endif // BCDDEBUG
				a = (mcu->sfr[RegAdr(dst, i * 2 + 2) + 1] << 8) | mcu->sfr[RegAdr(dst, i * 2 + 2)];
				b = (mcu->sfr[RegAdr(src, i * 2 + 2) + 1] << 8) | mcu->sfr[RegAdr(src, i * 2 + 2)];
				num = abcd44(calc_mode == 2, a, b, num3);
				if (i + 1 != calc_len)
				{
					num3 = (num >> 16) & 1u;
				}
				if (i + 1 != calc_len)
				{
					num2 = (((num & 0xFFFF) == 0 && num2 != 0) ? 1u : 0u);
				}
				mcu->sfr[REG_BCDFLG] = (byte)((num3 << 7) | (num2 << 6));
				if (flag)
				{
					if (i + 1 == calc_len)
					{
						num = 0u;
					}
					mcu->sfr[RegAdr(dst, i * 2 + 2) + 1] = num >> 8 & 0xff; mcu->sfr[RegAdr(dst, i * 2 + 2)] = num & 0xff;
				}
			}
		}
		if ((calc_mode == 1 || calc_mode == 2) && (calc_en_d || calc_en_dd))
		{
			calc_pos += 2;
			if (calc_pos >= calc_len)
			{
				calc_en = false;
			}
		}
	}

	void exec_Sft_Con_Cp()
	{
		byte b = 0;
		switch (calc_mode & (calc_en ? 15 : 0))
		{
		case 8:
			calc_sl(false);
			break;
		case 9:
			calc_sr(false);
			break;
		case 12:
			calc_sl(true);
			break;
		case 13:
			calc_sr(true);
			break;
		case 10:
		{
			for (int i = 1; i < 12; i++)
			{
				mcu->sfr[RegAdr(dst, i)] = 0;
			}
			mcu->sfr[RegAdr(dst, 0)] = (byte)((src == 3) ? 5u : ((uint)src));
			break;
		}
		case 11:
		{
			for (int i = 0; i < 12; i++)
			{
				b = mcu->sfr[RegAdr(src, i)];
				mcu->sfr[RegAdr(dst, i)] = b;
			}
			break;
		}
		}
	}

	void update_LLZ_MLZ()
	{
		byte b = 0;
		b = mcu->sfr[REG_BCDCMD];
		if ((!calc_en_dd || calc_en_d) && (b & 0xF0u) != 0)
		{
			return;
		}
		int num = 11;
		byte b2 = 0;
		while (num >= 0)
		{
			b = mcu->sfr[RegAdr(dst, num)];
			if (num < calc_len * 2 && (b & 0xF0u) != 0)
			{
				break;
			}
			b2++;
			if (num < calc_len * 2 && (b & 0xFu) != 0)
			{
				break;
			}
			b2++;
			num--;
		}
		num = 0;
		byte b3 = 0;
		for (; num < 12; num++)
		{
			b = mcu->sfr[RegAdr(dst, num)];
			if (num < calc_len * 2 && (b & 0xFu) != 0)
			{
				break;
			}
			b3++;
			if (num < calc_len * 2 && (b & 0xF0u) != 0)
			{
				break;
			}
			b3++;
		}
		mcu->sfr[REG_BCDLLZ] = b3;
		mcu->sfr[REG_BCDMLZ] = b2;
	}

	void check_calc_end() {
		if (((uint)calc_mode & 8u) != 0 || calc_mode == 0)
		{
			calc_en = false;
			calc_pos = 6;
		}
	}

	void check_BCD_Register() {
		check_BCDCMD();
		calc_len = mcu->sfr[REG_BCDCON];
		check_BCDMCR();
	}

	void check_BCDCMD() {
		byte b = 0;
		b = mcu->sfr[REG_BCDCMD];
		if (b == NOCMD_VAL) return;
		calc_mode = (b >> 4) & 0xF;
		src = (b >> 2) & 3;
		dst = b & 3;
		calc_pos = 0;
		if (calc_mode != 0) calc_en = true;
		else {
			calc_en = false;
			calc_en_d = true;
		}
		mcu->sfr[REG_BCDCMD] = NOCMD_VAL;
	}

	void check_BCDMCR() {
		byte b = 0;
		b = mcu->sfr[REG_BCDMCR];
		if ((b & 0x7Fu) != 0)
		{
			BMC = b;
			b = mcu->sfr[REG_BCDMCN];
			macro_cnt = b;
			mcu->sfr[REG_BCDMCR] = (byte)0;
			macro_state = MACRO_START;
		}
	}

	void state_manage() {
		byte b = 0;
		if (macro_state == MACRO_START && !calc_en) state_manage_init();
		else if ((mul_mode | div_mode | divsn_mode | sft_mode) && !calc_en) {
			if (mul_mode) state_manage_mul();
			else if (div_mode | divsn_mode) state_manage_div_divsn();
			else if (sft_mode) state_manage_sft();
			if (mul_mode || div_mode || divsn_mode) calc_en = (calc_mode | src | dst) != 0;
			calc_pos = 0;
		}
		b = mcu->sfr[REG_BCDMCR];
		b = (byte)((b & 0x7fu) | ((mul_mode | div_mode | divsn_mode | sft_mode) ? 0x80u : 0u));
		mcu->sfr[REG_BCDMCR] = b;
	}

	void state_set(int s_mode, int s_src, int s_dst, uint s_state) {
		calc_mode = s_mode;
		src = s_src;
		dst = s_dst;
		macro_state = s_state;
	}

	void state_manage_init() {
		byte b = 0;
		if (mul_mode)
		{
			b = mcu->sfr[RegAdr(0, 0)];
			state_set(13, 0, 0, (uint)(32 + (b & 0xF)));
		}
		else if (div_mode)
		{
			state_set(8, 0, 1, 24u);
		}
		else if (divsn_mode)
		{
			state_set(12, 0, 1, 24u);
		}
		else
		{
			switch ((BMC >> 1) & 0xF)
			{
			case 1u:
				if ((BMC & (true ? 1u : 0u)) != 0)
				{
					state_set(13, 0, 0, (uint)(32 + (b & 0xF)));
				}
				else
				{
					state_set(11, 1, 3, 24u);
				}
				mul_mode = true;
				break;
			case 2u:
				if ((BMC & (true ? 1u : 0u)) != 0)
				{
					state_set(8, 0, 1, 24u);
				}
				else
				{
					state_set(11, 1, 3, 16u);
				}
				div_mode = true;
				break;
			case 3u:
				if ((BMC & (true ? 1u : 0u)) != 0)
				{
					state_set(12, 0, 1, 24u);
				}
				else
				{
					state_set(11, 1, 3, 32u);
				}
				divsn_mode = true;
				break;
			case 4u:
			case 5u:
			case 6u:
			case 7u:
				macro_cnt++;
				state_set(((BMC & 0xC) == 8) ? 8 : 9, (macro_cnt >= 8) ? 3 : ((macro_cnt >= 4) ? 2 : ((macro_cnt >= 2) ? 1 : 0)), (int)(BMC & 3), 0u);
				macro_cnt -= (uint)(1 << src);
				sft_mode = macro_cnt != 0;
				break;
			default:
				state_set(0, 0, 0, macro_state);
				break;
			}
		}
		calc_pos = 0;
		calc_en = true;
		BMC = 0u;
	}

	void state_manage_mul() {
		byte b = 0;
		switch (macro_state)
		{
		case 24u:
			state_set(11, 1, 2, 25u);
			break;
		case 25u:
			state_set(1, 2, 2, 26u);
			break;
		case 26u:
			state_set(1, 2, 2, 27u);
			break;
		case 27u:
			state_set(10, 0, 1, 28u);
			break;
		case 28u:
			b = mcu->sfr[RegAdr(0, 0)];
			state_set(13, 0, 0, (uint)(32 + (b & 0xF)));
			break;
		case 32u:
			state_set(9, 0, 1, 63u);
			break;
		case 33u:
		case 34u:
		case 35u:
		case 36u:
		case 37u:
		case 38u:
		case 39u:
		case 40u:
		case 41u:
			state_set(9, 0, 1, macro_state + 16);
			break;
		case 49u:
			state_set(1, 3, 1, 63u);
			break;
		case 50u:
			state_set(1, 3, 1, 49u);
			break;
		case 51u:
			state_set(2, 3, 1, 52u);
			break;
		case 52u:
			state_set(1, 2, 1, 63u);
			break;
		case 53u:
			state_set(1, 2, 1, 49u);
			break;
		case 54u:
			state_set(1, 2, 1, 50u);
			break;
		case 55u:
			state_set(1, 2, 1, 51u);
			break;
		case 56u:
			state_set(1, 2, 1, 52u);
			break;
		case 57u:
			state_set(1, 2, 1, 53u);
			break;
		default:
			state_set(0, 0, 0, 63u);
			mul_mode = macro_cnt != 0;
			break;
		}
		if (macro_state == 63 && macro_cnt != 0)
		{
			macro_cnt--;
			macro_state = 255u;
		}
	}

	void state_manage_div_divsn() {
		byte b = 0;
		b = mcu->sfr[REG_BCDFLG];
		uint num = (((b & 0x80u) != 0) ? 1u : 0u);
		uint num2 = macro_state;
		switch (macro_state)
		{
		case 32u:
			state_set(11, 1, 2, 33u);
			break;
		case 33u:
			state_set(1, 1, 2, 34u);
			break;
		case 34u:
			state_set(1, 1, 2, 35u);
			break;
		case 35u:
			state_set(12, 3, 1, 36u);
			break;
		case 36u:
			state_set(8, 3, 0, 25u);
			break;
		case 16u:
			state_set(11, 1, 2, 17u);
			break;
		case 17u:
			state_set(1, 1, 2, 18u);
			break;
		case 18u:
			state_set(1, 1, 2, 19u);
			break;
		case 19u:
			state_set(11, 0, 1, 20u);
			break;
		case 20u:
			state_set(10, 0, 0, 24u);
			break;
		case 24u:
			state_set(8, 0, 0, 25u);
			break;
		case 25u:
			state_set(2, 2, 1, 26u);
			break;
		case 26u:
			if (num != 0)
			{
				state_set(1, 3, 1, 2u);
			}
			else
			{
				state_set(2, 2, 1, 27u);
			}
			break;
		case 2u:
			if (num != 0)
			{
				state_set(0, 0, 0, 63u);
			}
			else
			{
				state_set(1, 3, 1, 1u);
			}
			break;
		case 1u:
			if (num != 0)
			{
				state_set(0, 0, 0, 63u);
			}
			else
			{
				state_set(1, 3, 1, 0u);
			}
			break;
		case 0u:
			state_set(0, 0, 0, 63u);
			break;
		case 27u:
			if (num != 0)
			{
				state_set(1, 3, 1, 5u);
			}
			else
			{
				state_set(2, 2, 1, 9u);
			}
			break;
		case 5u:
			if (num != 0)
			{
				state_set(0, 0, 0, 63u);
			}
			else
			{
				state_set(1, 3, 1, 4u);
			}
			break;
		case 4u:
			if (num != 0)
			{
				state_set(0, 0, 0, 63u);
			}
			else
			{
				state_set(1, 3, 1, 3u);
			}
			break;
		case 3u:
			state_set(0, 0, 0, 63u);
			break;
		case 9u:
			if (num != 0)
			{
				state_set(1, 3, 1, 8u);
			}
			else
			{
				state_set(0, 0, 0, 63u);
			}
			break;
		case 8u:
			if (num != 0)
			{
				state_set(0, 0, 0, 63u);
			}
			else
			{
				state_set(1, 3, 1, 7u);
			}
			break;
		case 7u:
			if (num != 0)
			{
				state_set(0, 0, 0, 63u);
			}
			else
			{
				state_set(1, 3, 1, 6u);
			}
			break;
		case 6u:
			state_set(0, 0, 0, 63u);
			break;
		}
		if (macro_state != 63)
		{
			return;
		}
		b = mcu->sfr[RegAdr(0, 0)];
		b = (byte)((b & 0xF0u) | (num2 & 0xFu));
		mcu->sfr[RegAdr(0, 0)] = b;
		if (macro_cnt != 0)
		{
			macro_cnt--;
			if (div_mode)
			{
				state_set(8, 0, 1, 24u);
			}
			else if (divsn_mode)
			{
				state_set(12, 0, 1, 24u);
			}
		}
		else
		{
			div_mode = false;
			divsn_mode = false;
		}
	}

	void state_manage_sft() {
		src = ((macro_cnt >= 8) ? 3 : ((macro_cnt >= 4) ? 2 : ((macro_cnt >= 2) ? 1 : 0)));
		macro_cnt -= (uint)(1 << src);
		if (macro_cnt == 0)
		{
			sft_mode = false;
		}
		calc_en = true;
		macro_state = 0u;
	}

public:
	void perApi_Run() {
#ifdef BCDDEBUG
        printf("===== START =====\nf400 = %02x, f405 = %02x\n", mcu->sfr[REG_BCDCMD], mcu->sfr[REG_BCDMCR]);
#endif // BCDDEBUG
	    macro_state = MACRO_END;
	    calc_en = false;
	    calc_en_d = false;
	    calc_en_dd = false;
	    check_BCD_Register();
	    bool repeat = true;
	    do {
            repeat = !(!calc_en && macro_state == MACRO_END);
#ifdef BCDDEBUG
            printf("=== loop ===\ncalc_en = %d, macro_state = %02x, repeat = %d\n", calc_en, macro_state, repeat);
#endif // BCDDEBUG
            state_manage();
            calc_en_dd = calc_en_d;
            calc_en = calc_en_d;
            exec_calc();
#ifdef BCDDEBUG
            printf("=== end loop ===\n");
#endif
	    } while (repeat);
#ifdef BCDDEBUG
	    printf("===== END =====\n");
#endif
	}

	void perApi_Reset() {
	    if (config->hardware_id != HW_CLASSWIZ_CW) return;
		mcu->sfr[REG_BCDCMD] = 0;
		mcu->sfr[REG_BCDCON] = 0;
		mcu->sfr[REG_BCDMCN] = 0;
		mcu->sfr[REG_BCDMCR] = 0;
		mcu->sfr[REG_BCDFLG] = 0;
		mcu->sfr[REG_BCDLLZ] = 0;
		mcu->sfr[REG_BCDMLZ] = 0;
		for (int i = 0; i < 4; i++) for (int j = 0; j < 12; j++) mcu->sfr[0x480 + i*0x20 + j] = 0;
		calc_len = 0;
		calc_pos = 0;
	}
};

uint8_t bcdcmd_mcr(mcu *mcu, uint16_t addr, uint8_t val) {
    mcu->sfr[addr] = val;
    mcu->bcd->perApi_Run();
    return mcu->sfr[addr];
}

uint8_t bcdcon(mcu *mcu, uint16_t addr, uint8_t val) {
    if (val < 1) return 1;
    if (val > 6) return 6;
    return val;
}

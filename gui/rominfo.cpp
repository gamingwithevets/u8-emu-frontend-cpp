#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>

#include "rominfo.hpp"
#include "cwmem.hpp"

inline uint16_t le_read(auto& p) {
	return *(uint16_t *)&p;
}
inline void calc_cs_negword(uint16_t& sum, byte* bt, int len) {
	for (size_t i = 0; i < len; i += 2) {
		sum -= le_read(bt[i]);
	}
}

inline void calc_cs_negword_cs_negbyte(uint16_t& sum, byte* bt, int len) {
	for (size_t i = 0; i < len; i++) {
		sum -= bt[i];
	}
}

inline void calc_cs_negword_cs_posbyte(uint16_t& sum, byte* bt, int len) {
    for (size_t i = 0; i < len; i++) {
        sum += bt[i];
    }
}

ROMInfo rom_info(std::vector<byte> rom, std::vector<byte> flash, bool checksum) {
	auto dat = rom.data();
	auto dat2 = flash.data();
	ROMInfo ri{};
	auto spinit = *(uint16_t *)dat;
	enum {
		ES_PLUS_GY_OLD,
		ES_PLUS,
		ES_PLUS_2,
		CLASSWIZ_EX,
		CLASSWIZ_CW,
	} sum_type{};
	if (spinit == 0xf000) {
		if (rom.size() < 0x40000) {
			return ri;
		}
		if (rom.size() == 0x40000 || ((dat[0x3ffee] == 'C' && dat[0x3ffef] == 'Y'))) {
		    if (dat[0x3ffee] != 'C' || dat[0x3ffef] != 'Y') return ri;
			memcpy(ri.ver, &dat[0x3ffee], 8);
            memcpy(ri.cid, &dat[0x3fff8], 8);
            ri.desired_sum = le_read(dat[0x3fff6]);
            sum_type = CLASSWIZ_EX;
		}
		else {
			if (rom.size() < 0x60000) {
				return ri;
			}
			if (dat[0x5ffee] != 'E') {
				if (rom.size() < 0x80000) return ri;
				memcpy(&dat[0x5e000], &dat[0x70000], 0x2000);
			}
			memcpy(ri.ver, &dat[0x5ffee], 8);
			memcpy(ri.cid, &dat[0x5fff8], 8);
			if (ri.ver[0] != 'E') {
				auto ver = FindSignature(dat, 0x5e000, {
                             -1, 0x00, 0xe9, 0x90, 0xca, 0xff,
                             -1, 0x00, 0xe9, 0x90, 0xcb, 0xff,
                             -1, 0x00, 0xe9, 0x90, 0xcc, 0xff,
                             -1, 0x00, 0xe9, 0x90, 0xcd, 0xff,
                             -1, 0x00, 0xe9, 0x90, 0xce, 0xff,
                             -1, 0x00, 0xe9, 0x90, 0xcf, 0xff
                });
				auto ver2 = FindSignature(dat, 0x5e000, {
                              0x56, 0x00, 0xe9, 0x90, 0xd1, 0xff,
                              0x2e, 0x00, 0xe9, 0x90, 0xd2, 0xff
                });
				auto ofst = ver2[14] | (ver2[15] << 8);
				for (size_t i = 0; i < 6; i++) {
					ri.ver[i] = ver[i * 6];
				}
				ri.ver[6] = dat[ofst];
				ri.ver[7] = dat[ofst + 1];
			}
			ri.desired_sum = le_read(dat[0x5fff6]);
			sum_type = CLASSWIZ_CW;
		}
	}
	else if (spinit == 0x8dfe || spinit == 0x8e00) {
        auto str = FindSignature(dat, 0x8000, {'I', 'N', 'R', 'O', 'M'});
        if (str) {
            if (flash.size() < 0x80000) {
                return ri;
            }
            ri.type = ROMInfo::ES_5800P;
            ri.desired_sum = le_read(dat2[0x7fffe]);
            strcpy(ri.ver, (char *)str);
            if (checksum) calc_cs_negword_cs_posbyte(ri.real_sum, &dat2[0x40000], 0x3fffe);
            ri.ok = true;
            return ri;
        }
        str = FindSignature(dat, 0x8000, {'R', 'O', 'M'});
        if (str) {
            strcpy(ri.ver, (char *)str);
            ri.type = ROMInfo::ES;
        }
        return ri;
	}
	else if (spinit < 0x8dfe) {
		if (rom.size() < 0x20000) {
			return ri;
		}
		memcpy(ri.ver, &dat[0x1fff4], 8);
		ri.desired_sum = le_read(dat[0x1fffc]);
		sum_type = ES_PLUS;
	}
	else {
        auto str = FindSignature(dat, 0xb000, {'s', 'i', 'n', 'h', 0});
        if (str) {
            strcpy(ri.ver, (char *)(str + 6));
            ri.type = ROMInfo::TI_MATHPRINT;
        }
        return ri;
	}
	if (ri.ver[1] != 'Y') {
		return ri;
	}
	if (sum_type == ES_PLUS) {
		if (ri.ver[0] == 'L') sum_type = ES_PLUS;
		else if (ri.ver[5] == 'X') sum_type = ES_PLUS;
		else if (ri.ver[0] == 'G') sum_type = ES_PLUS_GY_OLD;
		else if (ri.ver[0] == 'C') sum_type = ES_PLUS_2;
		else {
			return ri;
		}
	}

	switch (sum_type) {
	case ES_PLUS_GY_OLD:
		if (checksum) {
			calc_cs_negword_cs_negbyte(ri.real_sum, dat, 0x8000);
			calc_cs_negword_cs_negbyte(ri.real_sum, &dat[0x10000], 0xfffc);
		}
		ri.type = ROMInfo::ES_PLUS;
		break;
	case ES_PLUS:
		if (checksum) {
			calc_cs_negword_cs_negbyte(ri.real_sum, dat, 0x10000);
			calc_cs_negword_cs_negbyte(ri.real_sum, &dat[0x10000], 0xfffc);
		}
		ri.type = ROMInfo::ES_PLUS;
		break;
	case ES_PLUS_2:
		if (checksum) {
			calc_cs_negword_cs_negbyte(ri.real_sum, dat, 0x10000);
			calc_cs_negword_cs_negbyte(ri.real_sum, &dat[0x10000], 0xff40);
			calc_cs_negword_cs_negbyte(ri.real_sum, &dat[0x1ffd0], 0x2c);
		}
		ri.type = ROMInfo::ES_PLUS_2;
		break;
	case CLASSWIZ_EX:
		if (checksum) {
			calc_cs_negword(ri.real_sum, dat, 0xfc00);
			calc_cs_negword(ri.real_sum, &dat[0x10000], 0x2fff6);
		}
		ri.type = ROMInfo::CLASSWIZ_EX;
		break;
	case CLASSWIZ_CW:
		if (checksum) {
			calc_cs_negword(ri.real_sum, dat, 0xfc00);
			calc_cs_negword(ri.real_sum, &dat[0x10000], 0x4fff6);
		}
		ri.type = ROMInfo::CLASSWIZ_CW;
		break;
	default:
		return ri;
	}
	ri.ok = true;
	return ri;
}

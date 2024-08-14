﻿#include "rominfo.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

inline word le_read(auto& p) {
	// this works for le machine
	return *(word*)&p;
}
inline void calc(word& sum, byte* bt, int len) {
	for (size_t i = 0; i < len; i += 2) {
		sum -= le_read(bt[i]);
	}
}

inline void calc2(word& sum, byte* bt, int len) {
	for (size_t i = 0; i < len; i++) {
		sum -= bt[i];
	}
}

RomInfo rom_info(std::vector<byte> rom,bool checksum) {
	auto dat = rom.data();
	RomInfo ri{};
	auto spinit = *(word*)dat;
	enum {
		Unk,
		ESP1,
		ESP2,
		CWX,
		CWII,
	} sum_type{};
	if (spinit == 0xf000) { // cwx or cwii
		if (rom.size() < 0x40000) {
			return ri;
		}
		if (rom.size() == 0x40000) { // must be cwx
		cwx_p:
			memcpy(ri.ver, &dat[0x3ffee], 8);
			memcpy(ri.cid, &dat[0x3fff8], 8);
			ri.desired_sum = le_read(dat[0x3fff6]);
			sum_type = CWX;
		}
		else {
			memcpy(ri.ver, &dat[0x3ffee], 8);
			if (ri.ver[0] == 'C' && ri.ver[1] == 'Y') {
				goto cwx_p;
			}
			if (rom.size() < 0x60000) {
				return ri;
			}
			if (dat[0x5ffee] != 'E') { // this means... it is stored at 0x71xxx
				if (rom.size() < 0x80000) {
					return ri;
				}
				memcpy(&dat[0x5e000], &dat[0x70000], 0x2000);
			}
			memcpy(ri.ver, &dat[0x5ffee], 8);
			memcpy(ri.cid, &dat[0x5fff8], 8);
			ri.desired_sum = le_read(dat[0x5fff6]);
			sum_type = CWII;
		}
	}
	else if (spinit == 0x8dfe) {
		ri.type = RomInfo::ES;
		return ri;
	}
	else if (spinit == 0x8e00) {
		ri.type = RomInfo::Fx5800p;
		return ri;
	}
	else if (spinit == 0x8dec || spinit == 0x8df2 || spinit == 0x8dea) {
		if (rom.size() < 0x20000) {
			return ri;
		}
		memcpy(ri.ver, &dat[0x1fff4], 8);
		ri.desired_sum = le_read(dat[0x1fffc]);
		sum_type = ESP1;
	}
	if (ri.ver[1] != 'Y') {
		return ri;
	}
	if (sum_type == ESP1) {
		if (ri.ver[0] == 'L' || ri.ver[0] == 'G') {
			sum_type = ESP1;
		}
		else if (ri.ver[0] == 'C') {
			sum_type = ESP2;
		}
		else {
			return ri;
		}
	}

	// std::cout << sum_type << "\n";
	switch (sum_type) {
	case ESP1:
		// calc(real_sum, dat, 0xfc00);
		if (checksum) {
			calc2(ri.real_sum, dat, 0x10000);
			calc2(ri.real_sum, &dat[0x10000], 0xfffc);
		}
		ri.type = RomInfo::ESP;
		break;
	case ESP2:
		// calc(real_sum, dat, 0xfc00);
		if (checksum) {
			calc2(ri.real_sum, dat, 0x10000);
			calc2(ri.real_sum, &dat[0x10000], 0xff40);
			calc2(ri.real_sum, &dat[0x1ffd0], 0x2c);
		}
		ri.type = RomInfo::ESP2nd;
		break;
	case CWX:
		if (checksum) {
			calc(ri.real_sum, dat, 0xfc00);
			calc(ri.real_sum, &dat[0x10000], 0x2fff6);
		}
		ri.type = RomInfo::CWX;
		break;
	case CWII:
		if (checksum) {
			calc(ri.real_sum, dat, 0xfc00);
			calc(ri.real_sum, &dat[0x10000], 0x4fff6);
		}
		ri.type = RomInfo::CWII;
		break;
	default:
		return ri;
	}
	ri.ok = true;
	return ri;
}

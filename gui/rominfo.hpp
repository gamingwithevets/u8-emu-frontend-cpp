#pragma once
#include <vector>
using word = unsigned short;
using byte = unsigned char;
struct ROMInfo {
	char ver[10];
	byte cid[8];
	word desired_sum;
	word real_sum;
	enum {
		ES,
		ES_5800P,
		ES_PLUS,
		ES_PLUS_2,
		CLASSWIZ_EX,
		CLASSWIZ_CW,
		TI_MATHPRINT,
	} type;
	bool ok;
};
ROMInfo rom_info(std::vector<byte> rom, std::vector<byte> flash, bool checksum = true);

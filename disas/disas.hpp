#pragma once

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "../peripheral/interrupts.hpp"

typedef struct {
	uint32_t offset;
	char srcbuf[80];
	bool is_label;
	int xref_operand;
} CodeElem;

inline std::map<int,bool> p_labels;

inline std::string signedtohex(int n, int binlen) {
	binlen--;
	bool ispositive = (n >> binlen) == 0;
	if (!ispositive)
		n = (2 << binlen) - n;
	std::string retval = "";
	binlen = 1 + binlen / 4;
	for (int x = 0; x < binlen; x++) {
		retval = "0123456789ABCDEF"[n & 0xF] + retval;
		n >>= 4;
	}
	if (retval[0] > '@') retval = "0" + retval;
	return (ispositive ? retval : ("-" + retval)) + "H";
}
inline std::string tohex(int n, int len) {
	std::string retval = "";
	for (int x = 0; x < len; x++) {
		retval = "0123456789ABCDEF"[n & 0xF] + retval;
		n >>= 4;
	}
	if (retval[0] > '@') retval = "0" + retval;
	return retval + "H";
}
inline std::string tobin(int n) {
	std::string retval = "";
	for (int x = 0; x < 8; x++) {
		retval = "01"[n & 1] + retval;
		n >>= 1;
	}
	return retval + "B";
}

inline void LABEL_FUNCTION(int x) {
	p_labels[x] = true;
}
inline void LABEL_LABEL(int x) {
	p_labels[x];
}

void decode(std::ostream& out, uint8_t*& buf, uint32_t pc, class Interrupts *ints);

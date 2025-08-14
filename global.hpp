/*
    u8-emu-frontend-cpp
    Copyright (C) 2024-2025  GamingWithEvets Inc.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <iostream>

// https://stackoverflow.com/a/2072890
inline bool ends_with(std::string const &fullString, std::string const &ending) {
    if (ending.size() > fullString.size()) return false;
    return fullString.compare(fullString.size() - ending.size(), ending.size(), ending) == 0;
}

inline char* stristr(const char* str1, const char* str2) {
	const char* p1 = str1;
	const char* p2 = str2;
	const char* r = *p2 == 0 ? str1 : 0;

	while (*p1 != 0 && *p2 != 0) {
		if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2)) {
			if (r == 0) {
				r = p1;
			}

			p2++;
		}
		else {
			p2 = str2;
			if (r != 0) {
				p1 = r + 1;
			}

			if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2)) {
				r = p1;
				p2++;
			}
			else {
				r = 0;
			}
		}

		p1++;
	}

	return *p2 == 0 ? (char*)r : 0;
}

inline std::string _tohex(int n, int len) {
	std::string retval = "";
	for (int x = 0; x < len; x++) {
		retval = "0123456789ABCDEF"[n & 0xF] + retval;
		n >>= 4;
	}
	return retval;
}

// ChatGPT
inline char *strprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    // figure out required length
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    char* buf = (char *)malloc(len + 1);
    if (!buf) return NULL;

    vsnprintf(buf, len + 1, fmt, args);
    va_end(args);
    return buf;  // caller must free
}

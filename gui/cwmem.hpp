#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

// ChatGPT
inline const uint8_t *FindSignature(const uint8_t* start, size_t size, const std::vector<int16_t>& pattern) {
    if (pattern.empty() || size < pattern.size()) return nullptr;

    const uint8_t* end = start + size - pattern.size() + 1;

    for (const uint8_t* p = start; p < end; ++p) {
        bool match = true;
        for (size_t i = 0; i < pattern.size(); ++i) {
            uint16_t pat = pattern[i];
            if (pat <= 0xFF && p[i] != static_cast<uint8_t>(pat)) {
                match = false;
                break;
            }
            // pat > 0xFF is a wildcard, matches anything
        }
        if (match) return p;
    }

    return nullptr;
}

#include <iostream>
#include <map>
#include "mcu.hpp"
#include "datalabels.hpp"

dlabels::dlabels(class mcu *mcu) {
    switch (mcu->config->hardware_id) {
    case HW_ES:
    case HW_ES_PLUS:
        ramlabels.push_back({0xdd, 1, "Disable cursor flashing", "If non-zero, the cursor will stop flashing on the next call to getscancode."});
        ramlabels.push_back({0xf2, 2, "Scancode of last key pressed", "Contains the KI and KO bits of the last key pressed."});
        ramlabels.push_back({0xf5, 1, "Keycode of last function pressed", "Contains the keycode of the last function pressed, e.g. sin^-1 = B0"});
        ramlabels.push_back({0xf8, 1, "Key modifier", "Bit 3: SHIFT - Bit 2: ALPHA - Bit 1: RCL - Bit 0: STO"});
        ramlabels.push_back({0xf9, 1, "Current mode ID", "Used IDs:\nC1 COMP - 02 BASE-N - 03 STAT - C4 CMPLX\n45 EQN - 06 MATRIX - 07 VECTOR - 88 TABLE"});
        ramlabels.push_back({0xfa, 1, "Current submode ID", "Used by some modes to specify the mode's submode. Each mode has a different submode map."});
        ramlabels.push_back({0xfb, 1, "Screen state", "0: Normal - 1: MODE - 2: SETUP - 3: All other menus"});

        ramlabels.push_back({0x102, 1, "Setup: Number Format", "8: Fix - 9: Sci - 0: Norm1 - 1: Norm2"});
        ramlabels.push_back({0x103, 1, "Setup: FixN/SciN"});
        ramlabels.push_back({0x104, 1, "Setup: Decimal Mark", "0: Comma - 1: Dot"});
        ramlabels.push_back({0x105, 1, "Setup: Angle Unit", "4: Degree - 5: Radian - 6: Gradian"});
        ramlabels.push_back({0x106, 1, "Setup: Math input toggle"});
        ramlabels.push_back({0x107, 1, "Setup: Fraction Result", "0: d/c - 1: ab/c"});
        ramlabels.push_back({0x108, 1, "Setup: Complex Result", "0: Polar Coord - 1: Rectangular Coord"});
        ramlabels.push_back({0x109, 1, "Setup: STAT - Frequency toggle"});
        ramlabels.push_back({0x10a, 1, "Setup: Recurring Decimal toggle"});
        ramlabels.push_back({0x10b, 1, "Setup: Manual simplification"});
        ramlabels.push_back({0x10c, 1, "Setup: Decimal output toggle"});
        ramlabels.push_back({0x10e, 1, "Setup: Contrast"});
        ramlabels.push_back({0x117, 1, "Font size"});

        if (mcu->config->hardware_id == HW_ES) {
            ramlabels.push_back({0x140, 100, "Input area", "Contains the tokens inputted onto the screen."});
            ramlabels.push_back({0x5e6, 0x10, "Memory integrity check", "Also known as the \"magic string\". Should always contain the bytes 0F 0E ... 01 00.\nIf on startup this area is found to be corrupted, the calculator will automatically\nreset all settings."});
        } else {
            ramlabels.push_back({0x154, 100, "Input area", "Contains the tokens inputted onto the screen."});
            ramlabels.push_back({0x60e, 0x10, "Memory integrity check", "Also known as the \"magic string\". Should always contain the bytes 0F 0E ... 01 00.\nIf on startup this area is found to be corrupted, the calculator will automatically\nreset all settings."});
        }
        ramlabels.push_back({0xa18, 1000, "Stack data", "Allocated for the stack.\nFor some reason, on ES and ES PLUS this region is filled with byte 90 (0x5A)."});
        break;
    }
}

void dlabels::get_name(int type, uint16_t addr, struct dldata *data) {
    std::vector<dldata> labels;
    switch (type) {
    case 0: labels = ramlabels; break;
    case 1: labels = sfrlabels; break;
    case 2: labels = ram2labels; break;
    default: return;
    }
    for (const dldata &d : labels) {
        if (addr >= d.addr && addr < d.addr + d.len) {
            *data = d;
            return;
        }
    }
}

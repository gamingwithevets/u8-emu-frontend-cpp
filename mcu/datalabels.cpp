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
        ramlabels.push_back({0x110, 1, "Cursor position"});
        ramlabels.push_back({0x112, 1, "Formula X coordinate"});
        ramlabels.push_back({0x113, 1, "Formula Y coordinate"});
        ramlabels.push_back({0x114, 1, "Cursor X coordinate"});
        ramlabels.push_back({0x115, 1, "Cursor Y coordinate"});
        ramlabels.push_back({0x116, 1, "Cursor character", "The font character to use for the cursor."});
        ramlabels.push_back({0x117, 1, mcu->config->hardware_id == HW_ES ? "Font size" : "Table font size"});
        int offset = 0x118;
        if (mcu->config->hardware_id == HW_ES_PLUS) {
            ramlabels.push_back({0x118, 1, "Table viewport", "The index (into the table) of the first row printed to the screen.\nChanges when scrolling."});
            ramlabels.push_back({0x119, 1, "Table highlighted row", "Relative to the current viewport."});
            ramlabels.push_back({0x11a, 1, "Table highlighted column", "Relative to the current viewport."});
            ramlabels.push_back({0x11b, 1, "Font size"});
            ramlabels.push_back({0x123, 1, "Use output character set", "If non-zero, some characters in the input will be displayed as a font character."});
            ramlabels.push_back({0x128, 2, "Formula pointer", "Contains a pointer to the current formula. (?)"});
            offset += 4;
        }
        ramlabels.push_back({offset, 1, "Draw mode", "0: White background\n4: White background (sanitize, only draw inside background)\n1: Transparent background\n2: AND with screen\n3 (otherwise): XOR with screen"});
        ramlabels.push_back({offset+1, 1, "Buffer toggle", "Switches between the real screen and the RAM screen buffer."});
        offset += mcu->config->hardware_id == HW_ES ? 12 : 36;
        ramlabels.push_back({offset, 10, "Displayed result (part 1)"});
        ramlabels.push_back({offset+10, 10, "Displayed result (part 2)"});
        ramlabels.push_back({offset+20, 100, "Input area", "Contains the tokens inputted onto the screen."});
        ramlabels.push_back({offset+120, 100, "Cache area", "The input area is copied to this area when a calculation happens.\nAlso used by the input recall feature by pressing [<-] or [->] when the input is\nempty."});
        ramlabels.push_back({offset+220, 8, "Random seed", "Used by the calculator's random number generator."});
        ramlabels.push_back({offset+228, 2, "Timer", "Counts up by 1 on every tick (i.e. every time the cursor turns on or off).\nAlso known as the \"unstable characters\"."});
        ramlabels.push_back({offset+230, 10, "Variable: M"});
        ramlabels.push_back({offset+230 + 10, 10, "Variable: Ans"});
        ramlabels.push_back({offset+230 + 10 * 2, 10, "Variable: A"});
        ramlabels.push_back({offset+230 + 10 * 3, 10, "Variable: B"});
        ramlabels.push_back({offset+230 + 10 * 4, 10, "Variable: C"});
        ramlabels.push_back({offset+230 + 10 * 5, 10, "Variable: D"});
        ramlabels.push_back({offset+230 + 10 * 6, 10, "Variable: E"});
        ramlabels.push_back({offset+230 + 10 * 7, 10, "Variable: F"});
        ramlabels.push_back({offset+230 + 10 * 8, 10, "Variable: X"});
        ramlabels.push_back({offset+230 + 10 * 9, 10, "Variable: Y"});
        ramlabels.push_back({offset+230 + 10 * (mcu->config->hardware_id == HW_ES ? 10 : 12), 250, "Calculation history",
                            "Can store up to 16 entries. Each entry's input data ends with a colon token.\nEach entry is not a fixed address, therefore if there's more input data then there\nis less space for new entries."});
        ramlabels.push_back({offset+588, 100, mcu->config->hardware_id == HW_ES ? "Result area" : "Undo buffer / Result area"});
        if (mcu->config->hardware_id == HW_ES_PLUS) offset += 12;
        ramlabels.push_back({offset+700, 10, "Variable: M im"});
        ramlabels.push_back({offset+700 + 10, 10, "Variable: Ans im"});
        ramlabels.push_back({offset+700 + 10 * 2, 10, "Variable: A im"});
        ramlabels.push_back({offset+700 + 10 * 3, 10, "Variable: B im"});
        ramlabels.push_back({offset+700 + 10 * 4, 10, "Variable: C im"});
        ramlabels.push_back({offset+700 + 10 * 5, 10, "Variable: D im"});
        ramlabels.push_back({offset+700 + 10 * 6, 10, "Variable: E im"});
        ramlabels.push_back({offset+700 + 10 * 7, 10, "Variable: F im"});
        ramlabels.push_back({offset+700 + 10 * 8, 10, "Variable: X im"});
        ramlabels.push_back({offset+700 + 10 * 9, 10, "Variable: Y im"});
        ramlabels.push_back({offset+1018, 100, "f(x)"});
        ramlabels.push_back({offset+1218, 0x10, "Memory integrity check", "Also known as the \"magic string\". Should always contain the bytes 0F 0E ... 01 00.\nIf on startup this area is found to be corrupted, the calculator will automatically\nreset all settings."});
        ramlabels.push_back({mcu->config->hardware_id == HW_ES ? 0x600 : 0x7d0, 12*32, "Screen buffer"});
        ramlabels.push_back({0xa18, 1000, "Stack data", "Allocated for the stack."});
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

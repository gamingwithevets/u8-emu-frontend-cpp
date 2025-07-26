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
#include <iostream>
#include <map>
#include "mcu.hpp"
#include "datalabels.hpp"

dlabels::dlabels(class mcu *mcu) {
    sfrlabels.push_back({0, 8, 0,
        "DSR",
        "",
        "",
        "Data segment register",
        "DSR is a special function register (SFR) used to retain a data segment."
    });

    unsigned int offset;
    bool is_ly = false;
    switch (mcu->config->hardware_id) {
    case HW_ES:
        if (mcu->config->is_5800p) {
            // TODO
            break;
        }
    case HW_ES_PLUS:
        is_ly = (mcu->rom[0x1fff4] == 'L' || mcu->rom[0x1fff4] == 'C') && mcu->rom[0x1fff5] == 'Y';

        ramlabels.push_back({0xdd, 1, "Disable cursor flashing", "If non-zero, the cursor will stop flashing on the next call to getscancode."});
        ramlabels.push_back({0xf2, 2, "Scancode of last key pressed", "Contains the KI and KO bits of the last key pressed."});
        ramlabels.push_back({0xf5, 1, "Keycode of last function pressed", "Contains the keycode of the last function pressed."});
        ramlabels.push_back({0xf8, 1, "Key modifier", "Bit 3: SHIFT - Bit 2: ALPHA - Bit 1: RCL - Bit 0: STO"});
        if (mcu->config->hardware_id == HW_ES) ramlabels.push_back({0xf9, 1, "Current mode ID", "Used IDs: (name will vary by model)\n"
            "C1 COMP  02 BASE-N  03 STAT   C4 CMPLX\n"
            "45 EQN   06 MATRIX  88 TABLE  07 VECTOR"});
        else ramlabels.push_back({0xf9, 1, "Current mode ID", "Used IDs: (name will vary by model)\n"
            "C1 COMP  02 BASE-N  03 STAT   C4 CMPLX\n"
            "45 EQN   06 MATRIX  07 VECTOR 88 TABLE\n"
            "89 VERIF 4A RATIO   4B INEQ   0C DIST"});
        ramlabels.push_back({0xfa, 1, "Current submode ID", "Used by some modes to specify the mode's submode. Each mode has a different submode map."});
        ramlabels.push_back({0xfb, 1, "Screen state", "0: Normal - 1: MODE - 2: SETUP - 3: All other menus"});
        ramlabels.push_back({0xfc, 1, "Table mode", "Used for drawing tables.\n"
            "01 None  06 Table range prompt  12 STAT/function table  13 Matrix\n"
            "14 Vector  15 Equations  17 Ratio  18 Inequality\n"
            "A0 CALC prompt  C0 SOLVE prompt"});

        ramlabels.push_back({0xff, 1, "Result area template", "Template style for displaying the result.\n"
            "11 Rec(r,θ)  12 Pol(x,y)  13 SOLVE  14 ÷R  15 →Simp"});

        ramlabels.push_back({0x100, 1, "Result format", "High nibble: Result display format.\nLow nibble: Variable store format.\n"
            "0 None  1 Degs Mins Secs\n"
            "2 [ENG]×4  3 [ENG]×3  4 [ENG]×2  5 [ENG]×1\n"
            "6 [SHIFT][ENG]×1  7 [SHIFT][ENG]×2  8 [SHIFT][ENG]×3  9 [SHIFT][ENG]×4\n"
            "A Decimal  B Improper Fraction  C Mixed Fraction  D Standard\n"
            "E Recurring Decimal  F Prime Factor"});
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
        ramlabels.push_back({0x111, 1, "Formula width", "Will never exceed 96 (0x60)."});
        ramlabels.push_back({0x112, 1, "Formula X coordinate"});
        ramlabels.push_back({0x113, 1, "Formula Y coordinate"});
        ramlabels.push_back({0x114, 1, "Cursor X coordinate"});
        ramlabels.push_back({0x115, 1, "Cursor Y coordinate"});
        ramlabels.push_back({0x116, 1, "Cursor character", "The font character to use for the cursor."});
        ramlabels.push_back({0x117, 1, mcu->config->hardware_id == HW_ES ? "Font size" : "Table font size"});
        offset = 0x118;
        if (mcu->config->hardware_id == HW_ES_PLUS) {
            ramlabels.push_back({0x118, 1, "Table viewport", "The index (into the table) of the first row printed to the screen.\nChanges when scrolling."});
            ramlabels.push_back({0x119, 1, "Table highlighted row", "Relative to the current viewport."});
            ramlabels.push_back({0x11a, 1, "Table highlighted column", "Relative to the current viewport."});
            ramlabels.push_back({0x11b, 1, "Font size"});
            offset += 4;
        }
        ramlabels.push_back({offset, 1, "Draw mode", "0: White background\n4: White background (sanitize, only draw inside background)\n1: Transparent background\n2: AND with screen\n3 (otherwise): XOR with screen"});
        ramlabels.push_back({offset+1, 1, "Buffer toggle", "Switches between the real screen and the RAM screen buffer."});
        if (is_ly) offset += 4;
        ramlabels.push_back({offset+7, 1, "Use output character set", "MathI only. If non-zero, some characters in the input will be displayed as a font character."});
        ramlabels.push_back({offset+9, 1, "Arrow state", "Holds the state of the arrow indicators. Not used in menus.\nBit 0: Up   Bit 1: Down"});
        if (mcu->config->hardware_id == HW_ES_PLUS) offset += 1;
        ramlabels.push_back({offset+11, 2, "Formula pointer", "Contains a pointer to the current formula displayed on the screen."});
        offset += mcu->config->hardware_id == HW_ES ? 20 : 35;
        ramlabels.push_back({offset, 10, "Displayed result (part 1)"});
        ramlabels.push_back({offset+10, 10, "Displayed result (part 2)"});
        ramlabels.push_back({offset+20, 100, "Input area", "Contains the tokens inputted onto the screen."});
        ramlabels.push_back({offset+120, 100, "Cache area", "The input area is copied to this area when a calculation happens.\nAlso used by the input recall feature by pressing [<-] or [->] when the input is\nempty."});
        ramlabels.push_back({offset+220, 8, "Random seed", "Used by the calculator's random number generator."});
        ramlabels.push_back({offset+228, 2, "Timer", "Part of the random seed.\nCounts up by 1 on every tick (i.e. every time the cursor turns on or off).\nAlso known as the \"unstable characters\"."});
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
        offset += 230 + 10 * (mcu->config->hardware_id == HW_ES ? 10 : 12);
        ramlabels.push_back({offset, 880, "Mode RAM", "RAM reserved for modes. Cleared when switching modes.\nContains things such as calculation history, imaginary variables, table values, functions, etc."});
        ramlabels.push_back({offset+880, 0x10, "Memory integrity check", "Also known as the \"magic string\". Should always contain the bytes 0F 0E ... 01 00.\nIf on startup this area is found to be corrupted, the calculator will automatically perform a Reset All."});
        if (mcu->config->hardware_id == HW_ES_PLUS) {
            ramlabels.push_back({offset+930, 400, "MathI bounding boxes?", "Stores bounding boxes(?) used to draw MathI graphics. Not used by square roots."});
            offset += 424;
        }
        ramlabels.push_back({offset+906, 12*32, "Screen buffer"});
        ramlabels.push_back({0xa18, 1000, "Stack data", "Allocated for the stack. The first 900 bytes are always filled with 0x5A on a reset."});
        if (!mcu->config->real_hardware) {
            ramlabels.push_back({0xe00, 1, "Emulator: STOP type", "Emulator flags. Used for keyboard and AC key handling."});
            ramlabels.push_back({0xe01, 2, "Emulator: Key scancode", "Scancode of last key pressed. Written to by the emulator."});
            ramlabels.push_back({0x1838, 23, "Emulator: Error buffer", "Contains the last triggered error as a string.\nIn LineI, also contains the last displayed result."});
        }
        break;
    case HW_CLASSWIZ_EX:
        ramlabels.push_back({0xf5, 1, "Disable cursor flashing", "If non-zero, the cursor will stop flashing on the next call to getscancode."});
        ramlabels.push_back({0x10e, 2, "Keycode of last function pressed", "Contains the keycode of the last function pressed."});
        ramlabels.push_back({0x110, 1, "Key modifier", "Bit 3: SHIFT - Bit 2: ALPHA - Bit 0: STO"});
        ramlabels.push_back({0x111, 1, "Current mode ID", "Used IDs: (name will vary by model)\n"
            "C1 Calculate  C4 Complex        02 Base-N        06 Matrix\n"
            "07 Vector     03 Statistics     0C Distribution  08 Spreadsheet\n"
            "88 Table      45 Equation/Func  4B Inequality    89 Verify\n"
            "4A Ratio      0E Algorithm"});
        ramlabels.push_back({0x112, 1, "Current submode ID", "Used by some modes to specify the mode's submode. Each mode has a different submode map."});
        ramlabels.push_back({0x113, 1, "Screen state", "0: Normal - 1: MODE - 2: SETUP - 3: All other menus"});

        ramlabels.push_back({0x11a, 1, "Setup: Number Format", "8: Fix - 9: Sci - 0: Norm1 - 1: Norm2"});
        ramlabels.push_back({0x11b, 1, "Setup: FixN/SciN"});
        ramlabels.push_back({0x11c, 1, "Setup: Decimal Mark", "0: Comma - 1: Dot"});
        ramlabels.push_back({0x11d, 1, "Setup: Angle Unit", "4: Degree - 5: Radian - 6: Gradian"});
        ramlabels.push_back({0x11e, 1, "Setup: Math input toggle"});
        ramlabels.push_back({0x11f, 1, "Setup: Fraction Result", "0: d/c - 1: ab/c"});
        ramlabels.push_back({0x120, 1, "Setup: Complex Result", "0: Polar Coord - 1: Rectangular Coord"});
        ramlabels.push_back({0x121, 1, "Setup: Statistics - Frequency toggle"});
        ramlabels.push_back({0x122, 1, "Setup: Recurring Decimal toggle"});
        ramlabels.push_back({0x123, 1, "Setup: Manual simplification"});
        ramlabels.push_back({0x124, 1, "Setup: Decimal output toggle"});
        ramlabels.push_back({0x125, 1, "Setup: Auto Power Off", "0: 10 min. - 1: 60 min."});
        ramlabels.push_back({0x126, 1, "Setup: Table - Table Type", "0: f(x) - 1: f(x) / g(x)"});
        ramlabels.push_back({0x127, 1, "Setup: Engineering Symbol toggle"});
        ramlabels.push_back({0x128, 1, "Setup: Digit Seperator toggle"});
        ramlabels.push_back({0x129, 1, "Setup: MultiLine (LineI) font size", "Valid values are 0E for Normal Font and 0A for Small Font.\nInvalid font sizes will appear broken."});
        ramlabels.push_back({0x12a, 1, "Setup: Equation - Complex Roots toggle"});
        ramlabels.push_back({0x12b, 1, "Setup: Current language ID", "Will vary by model."});
        ramlabels.push_back({0x12c, 1, "Setup: Spreadsheet - Auto Calc toggle"});
        ramlabels.push_back({0x12d, 1, "Setup: Spreadsheet - Show Result", "0: Formula - 1: Value"});
        ramlabels.push_back({0x12e, 1, "Setup: QR Code version", "Valid values are 0B for Version 11 and 03 for Version 3.\nInvalid QR code versions will show corrupted QR codes."});
        ramlabels.push_back({0x12f, 1, "Setup: Algorithm - Background", "0: Axes - 1: Axes/Grid - 2: Axes/Label - 3: None"});
        ramlabels.push_back({0x130, 1, "Setup: Algorithm - Unit Setting", "0: pixels - 1: units"});

        ramlabels.push_back({0x132, 1, "Cursor character", "The font character to use for the cursor."});
        ramlabels.push_back({0x133, 1, "Cursor font size", "The font size to use for the cursor."});
        ramlabels.push_back({0x134, 1, "Table viewport", "The index (into the table) of the first row printed to the screen.\nChanges when scrolling."});
        ramlabels.push_back({0x135, 1, "Table highlighted row", "Relative to the current viewport."});
        ramlabels.push_back({0x136, 1, "Table highlighted column", "Relative to the current viewport."});
        ramlabels.push_back({0x137, 1, "Font size"});
        ramlabels.push_back({0x138, 1, "Draw mode", "0: White background\n4: White background (sanitize, only draw inside background)\n1: Transparent background\n2: AND with screen\n3 (otherwise): XOR with screen"});
        ramlabels.push_back({0x139, 1, "Buffer select", "0: Buffer 2 (E3D4H) - >0 = Buffer 1 (DDD4H)"});
        ramlabels.push_back({0x140, 1, "Scroll size", "Always 0E (Normal Font size) in MathI.\nIn LineI it is equal to font_size * num_lines + 1"});
        ramlabels.push_back({0x144, 2, "Formula pointer", "Contains a pointer to the current formula displayed on the screen."});
        ramlabels.push_back({0x19d4, 1580, "Stack data", "Allocated for the stack."});
        if (!mcu->config->real_hardware) {
            ram2labels.push_back({0x8e00, 1, "Emulator: STOP type", "Emulator flags. Used for keyboard, AC key handling, and QR code display."});
            ram2labels.push_back({0x8e01, 2, "Emulator: Key scancode", "Scancode of last key pressed. Written to by the emulator."});
        }
        break;
    case HW_CLASSWIZ_CW:
        if (!mcu->config->real_hardware) {
            ram2labels.push_back({0x8e00, 1, "Emulator: STOP type", "Emulator flags. Used for keyboard, AC key handling, and QR code display."});
            ram2labels.push_back({0x8e01, 2, "Emulator: Key scancode", "Scancode of last key pressed. Written to by the emulator."});
        }
        break;
    case HW_TI_MATHPRINT:
        sfrlabels.push_back({2, 8, 16,
            "FCON0",
            "FCON1",
            "FCON01",
            "Frequency control register 01",
            "FCON01 is a special function register (SFR) used to control the high-speed clock generation circuit and to select system clock."
        });
        sfrlabels.push_back({4, 8, 16,
            "FCON2",
            "FCON3",
            "FCON23",
            "Frequency control register 23",
            "FCON23 is a special function register (SFR) used to select the clock for the low-speed clock generation circuit."
        });
        sfrlabels.push_back({8, 8, 0,
            "STPACP",
            "",
            "",
            "Stop code acceptor",
            "STPACP is a write-only special function register (SFR) that is used for setting a STOP mode."
        });
        sfrlabels.push_back({9, 8, 0,
            "SBYCON",
            "",
            "",
            "Standby control register",
            "SBYCON is a special function register (SFR) to control the operation mode of MCU."
        });
        sfrlabels.push_back({0xa, 8, 0,
            "FSTAT",
            "",
            "",
            "Frequency status register",
        });
        sfrlabels.push_back({0xc, 8, 0,
            "RSTAT",
            "",
            "",
            "Reset status register",
        });
        sfrlabels.push_back({0xe, 8, 0,
            "WDTCON",
            "",
            "",
            "Watchdog timer control register",
        });
        sfrlabels.push_back({0xf, 8, 0,
            "WDTMOD",
            "",
            "",
            "Watchdog timer mode register",
        });
        sfrlabels.push_back({0x10, 8, 16,
            "IE0",
            "IE1",
            "IE01",
            "Interrupt enable register 01",
        });
        sfrlabels.push_back({0x12, 8, 16,
            "IE2",
            "IE3",
            "IE23",
            "Interrupt enable register 23",
        });
        sfrlabels.push_back({0x14, 8, 16,
            "IE4",
            "IE5",
            "IE45",
            "Interrupt enable register 45",
        });
        sfrlabels.push_back({0x16, 8, 16,
            "IE6",
            "IE7",
            "IE67",
            "Interrupt enable register 67",
        });
        sfrlabels.push_back({0x18, 8, 16,
            "IRQ0",
            "IRQ1",
            "IRQ01",
            "Interrupt request register 01",
        });
        sfrlabels.push_back({0x1a, 8, 16,
            "IRQ2",
            "IRQ3",
            "IRQ23",
            "Interrupt request register 23",
        });
        sfrlabels.push_back({0x1c, 8, 16,
            "IRQ4",
            "IRQ5",
            "IRQ45",
            "Interrupt request register 45",
        });
        sfrlabels.push_back({0x1e, 8, 16,
            "IRQ6",
            "IRQ7",
            "IRQ67",
            "Interrupt request register 67",
        });
        break;
    }
}

void dlabels::get_name(int type, uint16_t addr, dldata *data) {
    std::vector<dldata> labels;
    switch (type) {
    case 0: labels = ramlabels; break;
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

void dlabels::get_sfr_name(uint16_t addr, sfrdata *data) {
    for (const sfrdata &d : sfrlabels) {
        uint16_t len = (d.wbits > 0 ? d.wbits : d.bbits) / 8;
        if (addr >= d.addr && addr < d.addr + len) {
            *data = d;
            return;
        }
    }
}

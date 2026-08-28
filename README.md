## We've moved!
The project will continue on [Codeberg](https://codeberg.org/gwetm/u8-emu-frontend-cpp). See you there!

This repository has also been archived, as a result.

---

u8-emu-frontend-cpp is a frontend for Fraserbc's [U8 core library](https://github.com/Fraserbc/u8_emu).

Initially, it was developed as a replacement to [u8-emu-frontend](https://github.com/gamingwithevets/u8-emu-frontend), written in Python.

This emulator uses the [`docking` branch of Dear ImGui](https://github.com/ocornut/imgui/tree/docking).

## Building
The Makefile has 5 targets:
- `clean`: Clean all object files.
- `all`: Everything below.
- `release` and `release_linux`: Optimized build; recommended for building.  
  - Note that, as of now, these two targets are seperate. `release` will only work on Windows, and `release_linux` will only work on Linux. This is subject to change in the future.
- `debug`: Unoptimized debug build. Windows only.

Note that this project uses SDL3 and not SDL2.

## Configuration
The emulator uses a specialized binary configuration file format. The format uses the `.bin` extension. The file layout can be found in [`config/config.hpp`](config/config.hpp).

A Python script to convert Python configuration files to binary format is provided in the u8-emu-frontend repository.

### Startup UI
u8-emu-frontend-cpp uses a modified version of the startup UI ported from [CasioEmuMsvc](https://github.com/telecomadm1145/CasioEmuMsvc).

To use the startup UI, all configuration files must be placed in the **root** of the `configs` directory of your current working directory.
If the directory is not found, you need to specify the path to a configuration binary as a command-line argument.

## Notes
- This emulator uses BCD emulation code for CW models from [Xyzst's CasioEmuX](https://github.com/Xyzstk/CasioEmuX), licensed under GPL-v3. However, it is not entirely accurate.
- ROM packages, LUA configs, and CasioEmuMsvc configuration files **are not supported**.

## Special thanks
- [Xyzst](https://github.com/Xyzstk) - BCD emulation, low speed time base counter, watchdog timer
- [telecomadm1145](https://github.com/telecomadm1145) - Startup UI, binary parser, disassembly listing
- [Fraserbc](https://github.com/Fraserbc) - U8 core library
- Other members of the [Casio Calculator Hacking community](http://discord.gg/QjGpH6rSQQ) - Documentation, research, and help

## External links
[Casio Calculator Reverse Engineering Discord](https://discord.gg/bG9BCJ5MW3)

## License
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
[GNU General Public License](LICENSE) for more details.

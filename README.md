Replacement to [u8-emu-frontend](https://github.com/gamingwithevets/u8-emu-frontend) written in Python.
This new, fresh emulator built fully from scratch and written in C++ is faster than the previous Python-C hybrid version.

This emulator uses the [`docking` branch of Dear ImGui](https://github.com/ocornut/imgui/tree/docking).

## Building
A `Makefile` for MSYS2 MinGW32 is provided in the repository. You may need to edit it to conform with your environment.

## Configuration
The emulator uses a specialized binary configuration file format. The format uses the `.bin` extension. The file layout can be found in [`config/config.hpp`](config/config.hpp).

A Python script to convert Python configuration files to binary format is now provided in the u8-emu-frontend repository.

### Startup UI
To use the startup UI, all configuration files must be placed in the **root** of the `configs` directory of your current working directory.
If the directory is not found, you need to specify the path to a configuration file as a command-line argument.

## Special thanks
- [Xyzst](https://github.com/Xyzstk) - Wrote some of the code used
- [telecomadm1145](https://github.com/telecomadm1145) - Startup UI, wrote some of the code used
- [Fraser Price / Delta / frsr](https://github.com/Fraserbc) - U8 core library
- [Other members of the Casio Calculator Hacking community](http://discord.gg/QjGpH6rSQQ) - Documentation, research, and help

## License
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

For more information, see the [full license document](LICENSE). 

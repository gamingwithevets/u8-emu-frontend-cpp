Planned future replacement to [u8-emu-frontend](https://github.com/gamingwithevets/u8-emu-frontend) written in Python.
This new, fresh emulator built fully from scratch and written in C++ will be faster than the previous Python-C hybrid version.

## Building
A `Makefile` for MSYS2 MinGW32 is provided in the repository. You may need to edit it to conform with your environment.

## Configuration
The emulator uses the Genshit configuration format (definitely original name). The format uses the `.bin` extension. The file layout can be found in [`config/config.hpp`](config/config.hpp).

A Python script to convert Python configuration files to the Genshit configuration format is now provided in the u8-emu-frontend repository.

### Startup UI
To use the startup UI, all Genshit configuration files must be placed in the **root** of the `configs` directory of your current working directory.
If the directory is not found, you need to specify the path to a configuration file as a command-line argument.

## Special thanks
- [telecomadm1145](https://github.com/telecomadm1145), for making the startup UI, inspiring the name of the configuration format and wrote some of the code used
- [Fraser Price / Delta / frsr](https://github.com/Fraserbc), for making the original U8 core library
- [The Casio Calculator Hacking community](http://discord.gg/QjGpH6rSQQ) for all documentation, research, and help

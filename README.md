# An NES emulator written in C++

This is a NES emulator I wrote to learn about game emulation. This is a working in progress.

## Current Status

See bBelow the features implemented.

* CPU (6502)
    - [x] Official opcodes
    - [x] Unofficial opcodes
    - [x] Cycle accuracy (official & unofficial)
    - [x] Dummy reads/writes
    - [x] BCD arithmetic (Not needed by NES anyway)
* Memory
    - [ ] Battery backed (persistent) save RAM
    - [ ] Open-bus
* PPU (Picture Processing Unit)
    - [x] Open-bus
    - [ ] NTSC
    - [ ] PAL
    - [ ] Dendy
* APU (Audio Processing Unit)
    - [ ] Pulse channel
    - [ ] Triangle channel
    - [ ] Noise channel
    - [ ] Delta Modulation Channel
* Gaming input
    - [ ] Keyboard input
    - [ ] Keyboard (multiplayer)
    - [ ] Original NES controller
    - [ ] Gamepad controller (Multiplayer)
    - [ ] Turbo keys
    - [ ] Custom Touch controller (Android)
* Formats
    - [x] iNES
    - [ ] NES2.0
    - [ ] NSF
    - [ ] NSFe
    - [ ] NSF2
    - [ ] UNIF
    - [ ] FDS
    - [ ] IPS
* Mappers
    - [ ] \#0   NROM
    - [ ] \#1   MMC1
    - [ ] \#2   UxROM
    - [ ] \#3   CNROM
    - [x] \#4   MMC3
    - [ ] \#5   MMC5
    - [ ] \#7   AxROM
    - [ ] \#11  Color Dreams
    - [ ] \#66  GNROM
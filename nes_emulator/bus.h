#pragma once
#include <cstdint>
#include <array>

class CPU6502;
class PPU2C02;
class Cartridge;

class Bus {
public:
    Bus();

    uint8_t cpuRead(uint16_t addr, bool readOnly = false);

    void    cpuWrite(uint16_t addr, uint8_t data);

public: //Inner Variables

    // Devices
    CPU6502* cpu = nullptr;     //Central Processing Unit
    PPU2C02* PPU = nullptr;     //Picture Processing Unit
    Cartridge* cart = nullptr;  //ROM Memory

    // RAM
    std::array<uint8_t, 2048> ram;

    // Test Mode (used for testing)
    bool nestestMode = false;
};

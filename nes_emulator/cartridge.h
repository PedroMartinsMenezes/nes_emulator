#pragma once
#include <cstdint>
#include <vector>
#include <string>

class Cartridge {
public:
    Cartridge(const std::string& romPath);

    void reset();

    bool cpuRead(uint16_t addr, uint8_t& data);

    bool cpuWrite(uint16_t addr, uint8_t data);

    bool irq = false;

private:
    // PRG ROM
    std::vector<uint8_t> prgROM;

    // For now: NROM only (mapper 0)
    uint8_t prgBanks = 0;
};

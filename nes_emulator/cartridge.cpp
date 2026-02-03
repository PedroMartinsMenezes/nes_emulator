#include "cartridge.h"
#include <fstream>
#include <iostream>
#include <cassert>
#include <fstream>
#include <cstring>
#include "Cartridge.h"
#include "mapper1_mmc1.h"


Cartridge::Cartridge(const std::string& romPath)
{
    std::ifstream file(romPath, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open the NES file " + romPath);

    INesHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    // Validate iNES signature
    if (std::strncmp(header.name, "NES\x1A", 4) != 0)
        return;

    bool hasTrainer = header.flags6 & 0x04;

    // Skip trainer if present
    if (hasTrainer)
        file.seekg(512, std::ios::cur);

    // --- Load PRG-ROM ---
    const size_t prgSize = header.prgChunks * 16 * 1024;
    prgROM.resize(prgSize);
    file.read(reinterpret_cast<char*>(prgROM.data()), prgSize);

    // --- Load CHR ---
    bool chrIsRam = (header.chrChunks == 0);

    if (chrIsRam)
    {
        chrRAM.resize(8 * 1024);
    }
    else
    {
        const size_t chrSize = header.chrChunks * 8 * 1024;
        chrROM.resize(chrSize);
        file.read(reinterpret_cast<char*>(chrROM.data()), chrSize);
    }

    // --- Mapper number ---
    uint8_t mapperId =
        (header.flags7 & 0xF0) |
        (header.flags6 >> 4);

    switch (mapperId)
    {
    case 1: // MMC1
        mapper = std::make_unique<Mapper1>(
            header.prgChunks,
            header.chrChunks,
            prgROM,
            chrIsRam ? chrRAM : chrROM,
            chrIsRam
        );
        break;

    default:
        // Unsupported mapper
        return;
    }

    valid = true;
}

// ------------------------------------------------------------
// CPU interface
// ------------------------------------------------------------

bool Cartridge::cpuRead(uint16_t addr, uint8_t& data)
{
    return mapper && mapper->cpuRead(addr, data);
}

bool Cartridge::cpuWrite(uint16_t addr, uint8_t data)
{
    return mapper && mapper->cpuWrite(addr, data);
}

// ------------------------------------------------------------
// PPU interface
// ------------------------------------------------------------

bool Cartridge::ppuRead(uint16_t addr, uint8_t& data)
{
    return mapper && mapper->ppuRead(addr, data);
}

bool Cartridge::ppuWrite(uint16_t addr, uint8_t data)
{
    return mapper && mapper->ppuWrite(addr, data);
}

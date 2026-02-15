#include "cartridge.h"
#include <fstream>
#include <iostream>
#include <cassert>
#include <fstream>
#include <cstring>
#include "Cartridge.h"
#include "mapper1_mmc1.h"
#include "mapper0_nrom.h"


Cartridge::Cartridge(const std::string& romPath)
{
    std::ifstream file(romPath, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open NES file: " + romPath);

    INesHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (!file)
        throw std::runtime_error("Failed to read iNES header.");

    // Validate signature
    if (std::strncmp(header.name, "NES\x1A", 4) != 0)
        throw std::runtime_error("Invalid iNES file.");

    // Reject NES 2.0 for now
    if ((header.flags7 & 0x0C) == 0x08)
        throw std::runtime_error("NES 2.0 format not supported yet.");

    bool hasTrainer = header.flags6 & 0x04;

    if (hasTrainer)
        file.seekg(512, std::ios::cur);

    // -------------------------------------------------
    // PRG ROM
    // -------------------------------------------------
    const size_t prgSize = header.prgChunks * 16 * 1024;
    prgROM.resize(prgSize);
    file.read(reinterpret_cast<char*>(prgROM.data()), prgSize);

    if (!file)
        throw std::runtime_error("Failed to read PRG ROM.");

    // -------------------------------------------------
    // CHR
    // -------------------------------------------------
    bool chrIsRam = (header.chrChunks == 0);

    if (chrIsRam)
    {
        chrRAM.resize(8 * 1024); // standard CHR-RAM size
    }
    else
    {
        const size_t chrSize = header.chrChunks * 8 * 1024;
        chrROM.resize(chrSize);
        file.read(reinterpret_cast<char*>(chrROM.data()), chrSize);

        if (!file)
            throw std::runtime_error("Failed to read CHR ROM.");
    }

    // -------------------------------------------------
    // PRG RAM (default 8KB if unspecified)
    // -------------------------------------------------
    // iNES 1.0 default PRG RAM = 8KB
    prgRAM.resize(8 * 1024);


    // -------------------------------------------------
    // Mapper ID
    // -------------------------------------------------
    uint8_t mapperId =
        (header.flags6 >> 4) |
        (header.flags7 & 0xF0);

    // -------------------------------------------------
    // Mirroring from header
    // -------------------------------------------------
    bool verticalMirror = header.flags6 & 0x01;

    MIRROR mirror =
        verticalMirror ? MIRROR::VERTICAL
        : MIRROR::HORIZONTAL;

    // -------------------------------------------------
    // Instantiate Mapper
    // -------------------------------------------------
    switch (mapperId)
    {
    case 0: // NROM
        mapper = std::make_unique<Mapper0>(
            header.prgChunks,
            header.chrChunks,
            prgROM,
            prgRAM,
            chrROM,
            chrRAM,
            mirror
        );
        break;

    case 1: // MMC1
        mapper = std::make_unique<Mapper1>(
            header.prgChunks,
            header.chrChunks,
            prgROM,
            prgRAM,
            chrROM,
            chrRAM,
            chrIsRam,
            mirror
        );
        break;

    default:
        throw std::runtime_error("Unsupported mapper: " + std::to_string(mapperId));
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

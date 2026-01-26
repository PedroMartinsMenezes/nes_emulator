#include "cartridge.h"
#include <fstream>
#include <iostream>

Cartridge::Cartridge(const std::string& romPath) {
    std::ifstream ifs(romPath, std::ifstream::binary);

    if (!ifs) {
        throw std::runtime_error("Failed to open ROM");
    }

    // Read iNES header
    struct Header {
        char name[4];
        uint8_t prgChunks;
        uint8_t chrChunks;
        uint8_t flags6;
        uint8_t flags7;
        uint8_t flags8;
        uint8_t flags9;
        uint8_t flags10;
        uint8_t padding[5];
    } header{};

    ifs.read(reinterpret_cast<char*>(&header), sizeof(Header));

    if (header.name[0] != 'N' ||
        header.name[1] != 'E' ||
        header.name[2] != 'S') {
        throw std::runtime_error("Invalid iNES file");
    }

    prgBanks = header.prgChunks;

    // Skip trainer if present
    if (header.flags6 & 0x04) {
        ifs.seekg(512, std::ios_base::cur);
    }

    // Read PRG ROM
    prgROM.resize(prgBanks * 16384);
    ifs.read(reinterpret_cast<char*>(prgROM.data()), prgROM.size());
}

void Cartridge::reset()
{
    //if (mapper)
    //    mapper->reset();
}


bool Cartridge::cpuRead(uint16_t addr, uint8_t& data) {
    if (addr >= 0x8000) {
        if (prgBanks == 1) {
            // 16KB mirrored
            data = prgROM[addr & 0x3FFF];
        }
        else {
            // 32KB direct
            data = prgROM[addr & 0x7FFF];
        }
        return true;
    }
    return false;
}

bool Cartridge::cpuWrite(uint16_t addr, uint8_t data) {
    // NROM has no writable PRG
    // Later: mapper registers go here
    return false;
}


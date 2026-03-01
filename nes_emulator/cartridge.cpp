#include "cartridge.h"

#include <fstream>
#include <iostream>

#include "mapper0_nrom.h"
#include "mapper1_mmc1.h"

Cartridge::Cartridge(const std::string& filename)
{
    std::ifstream file(filename, std::ifstream::binary);

    if (!file.is_open())
    {
        std::cout << "Failed to open ROM file\n";
        return;
    }

    struct iNESHeader
    {
        char name[4];
        uint8_t prg_rom_chunks;
        uint8_t chr_rom_chunks;
        uint8_t mapper1;
        uint8_t mapper2;
        uint8_t prg_ram_size;
        uint8_t tv_system1;
        uint8_t tv_system2;
        char unused[5];
    } header;

    file.read((char*)&header, sizeof(iNESHeader));

    if (header.name[0] != 'N' ||
        header.name[1] != 'E' ||
        header.name[2] != 'S')
    {
        std::cout << "Invalid iNES file\n";
        return;
    }

    mapperID = ((header.mapper2 >> 4) << 4) | (header.mapper1 >> 4);

    prgBanks = header.prg_rom_chunks;
    chrBanks = header.chr_rom_chunks;

    // Skip trainer if present
    if (header.mapper1 & 0x04)
        file.seekg(512, std::ios_base::cur);

    // Load PRG ROM
    prgROM.resize(prgBanks * 16384);
    file.read((char*)prgROM.data(), prgROM.size());

    // Load CHR ROM
    if (chrBanks == 0)
    {
        // CHR RAM (8KB)
        chrROM.resize(8192);
    }
    else
    {
        chrROM.resize(chrBanks * 8192);
        file.read((char*)chrROM.data(), chrROM.size());
    }

    switch (mapperID)
    {
    case 0:
        mapper = std::make_shared<Mapper0_NROM>(prgROM, chrROM);
        break;

    case 1:
        mapper = std::make_shared<Mapper1_MMC1>(prgROM, chrROM);
        break;

    default:
        std::cout << "Unsupported Mapper: " << (int)mapperID << "\n";
        return;
    }

    valid = true;
}

bool Cartridge::isValid() const
{
    return valid;
}

bool Cartridge::cpuRead(uint16_t addr, uint8_t& data)
{
    if (mapper)
        return mapper->cpuRead(addr, data);

    return false;
}

bool Cartridge::cpuWrite(uint16_t addr, uint8_t data)
{
    if (mapper)
        return mapper->cpuWrite(addr, data);

    return false;
}

bool Cartridge::ppuRead(uint16_t addr, uint8_t& data)
{
    if (mapper)
        return mapper->ppuRead(addr, data);

    return false;
}

bool Cartridge::ppuWrite(uint16_t addr, uint8_t data)
{
    if (mapper)
        return mapper->ppuWrite(addr, data);

    return false;
}

void Cartridge::reset()
{
    if (mapper)
        mapper->reset();
}
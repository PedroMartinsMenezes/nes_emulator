#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "IMapper.h"

struct INesHeader;

class Cartridge {
public:
    Cartridge(const std::string& romPath);

    //void reset();
    bool cpuRead(uint16_t addr, uint8_t& data);
    bool cpuWrite(uint16_t addr, uint8_t data);

    bool ppuRead(uint16_t addr, uint8_t& data);
    bool ppuWrite(uint16_t addr, uint8_t data);

    bool irq = false;

private:
    uint32_t decodePRGBanks(const INesHeader& h);
    uint32_t decodeCHRBanks(const INesHeader& h);
    bool isNES2(const INesHeader& h);

private:
    // Program ROM (Store Code and Data)
    std::vector<uint8_t> prgROM;

    // Character ROM (Store Sprites)
    std::vector<uint8_t> chrROM;

    // Character RAM ?
    std::vector<uint8_t> chrRAM;

    // For now: NROM only (mapper 0)
    uint32_t prgBanks = 0;
    uint32_t chrBanks = 0;

    bool chrIsRAM = false;
    bool valid = false;

    std::unique_ptr<IMapper> mapper;
};

#pragma pack(push, 1)
struct INesHeader {
    char name[4];
    uint8_t prgChunks;
    uint8_t chrChunks;
    uint8_t flags6;
    uint8_t flags7;
    uint8_t flags8;
    uint8_t flags9;
    uint8_t flags10;
    uint8_t padding[5];
};
#pragma pack(pop)

#pragma once

#include "IMapper.h"
#include <vector>

class Mapper0 : public IMapper
{
public:
    Mapper0(uint8_t prgBanks,
        uint8_t chrBanks,
        std::vector<uint8_t>& prgROM,
        std::vector<uint8_t>& prgRAM,
        std::vector<uint8_t>& chrROM,
        std::vector<uint8_t>& chrRAM,
        MIRROR mirrorMode);

    bool cpuRead(uint16_t addr, uint8_t& data) override;
    bool cpuWrite(uint16_t addr, uint8_t data) override;

    bool ppuRead(uint16_t addr, uint8_t& data) override;
    bool ppuWrite(uint16_t addr, uint8_t data) override;

    MIRROR Mirror() const override;

private:
    uint8_t prgBanks;
    uint8_t chrBanks;

    std::vector<uint8_t>& prgROM;
    std::vector<uint8_t>& prgRAM;
    std::vector<uint8_t>& chrROM;
    std::vector<uint8_t>& chrRAM;

    bool chrIsRam;
    MIRROR mirror;
};

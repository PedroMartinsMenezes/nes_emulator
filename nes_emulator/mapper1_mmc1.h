#pragma once

#include <cstdint>
#include <vector>
#include "IMapper.h"

class Mapper1_MMC1 : public IMapper
{
public:
    Mapper1_MMC1(const std::vector<uint8_t>& prg,
        const std::vector<uint8_t>& chr);

    bool cpuRead(uint16_t addr, uint8_t& data) override;
    bool cpuWrite(uint16_t addr, uint8_t data) override;

    bool ppuRead(uint16_t addr, uint8_t& data) override;
    bool ppuWrite(uint16_t addr, uint8_t data) override;

    void reset() override;

private:
    void updateBanks();

private:
    std::vector<uint8_t> prgROM;
    std::vector<uint8_t> chrROM;

    uint8_t shiftRegister = 0x10;
    uint8_t control = 0x0C;
    uint8_t prgBank = 0;
    uint8_t chrBank0 = 0;
    uint8_t chrBank1 = 0;

    uint32_t prgBankOffset0 = 0;
    uint32_t prgBankOffset1 = 0;
};
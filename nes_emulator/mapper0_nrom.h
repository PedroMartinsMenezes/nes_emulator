#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include "IMapper.h"

class Mapper0_NROM : public IMapper
{
public:
    Mapper0_NROM(const std::vector<uint8_t>& prg,
        const std::vector<uint8_t>& chr);

    bool cpuRead(uint16_t addr, uint8_t& data) override;
    bool cpuWrite(uint16_t addr, uint8_t data) override;

    bool ppuRead(uint16_t addr, uint8_t& data) override;
    bool ppuWrite(uint16_t addr, uint8_t data) override;

    void reset() override;

private:
    std::vector<uint8_t> prgROM;
    std::vector<uint8_t> chrROM;

    bool is16KB = false;
};
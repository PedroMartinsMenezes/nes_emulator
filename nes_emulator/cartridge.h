#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

#include "IMapper.h"

class Cartridge
{
public:
    Cartridge(const std::string& filename);

    bool isValid() const;

    bool cpuRead(uint16_t addr, uint8_t& data);
    bool cpuWrite(uint16_t addr, uint8_t data);

    bool ppuRead(uint16_t addr, uint8_t& data);
    bool ppuWrite(uint16_t addr, uint8_t data);

    void reset();

private:
    bool valid = false;

    uint8_t mapperID = 0;
    uint8_t prgBanks = 0;
    uint8_t chrBanks = 0;

    std::vector<uint8_t> prgROM;
    std::vector<uint8_t> chrROM;

    std::shared_ptr<IMapper> mapper;
};
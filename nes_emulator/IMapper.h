#pragma once
#include <cstdint>


class IMapper {
public:
    virtual ~IMapper() = default;
    virtual bool cpuRead(uint16_t addr, uint8_t& data) = 0;
    virtual bool cpuWrite(uint16_t addr, uint8_t data) = 0;
    virtual bool ppuRead(uint16_t addr, uint8_t& data) = 0;
    virtual bool ppuWrite(uint16_t addr, uint8_t data) = 0;
};

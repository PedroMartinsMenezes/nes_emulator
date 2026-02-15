#pragma once
#include <cstdint>

enum MIRROR
{
    HORIZONTAL,
    VERTICAL,
    SINGLE0,
    SINGLE1
};

class IMapper {
public:
    virtual ~IMapper() = default;
    virtual bool cpuRead(uint16_t addr, uint8_t& data) = 0;
    virtual bool cpuWrite(uint16_t addr, uint8_t data) = 0;
    virtual bool ppuRead(uint16_t addr, uint8_t& data) = 0;
    virtual bool ppuWrite(uint16_t addr, uint8_t data) = 0;
    virtual MIRROR Mirror() const = 0;
};

#include "mapper0_nrom.h"

Mapper0_NROM::Mapper0_NROM(const std::vector<uint8_t>& prg,
    const std::vector<uint8_t>& chr)
    : prgROM(prg), chrROM(chr)
{
    is16KB = (prgROM.size() == 16384);
}

void Mapper0_NROM::reset()
{
}

bool Mapper0_NROM::cpuRead(uint16_t addr, uint8_t& data)
{
    if (addr >= 0x8000)
    {
        uint32_t mapped = addr - 0x8000;

        if (is16KB)
            mapped &= 0x3FFF;  // mirror 16KB

        data = prgROM[mapped];
        return true;
    }

    return false;
}

bool Mapper0_NROM::cpuWrite(uint16_t addr, uint8_t data)
{
    // NROM is ROM only
    return false;
}

bool Mapper0_NROM::ppuRead(uint16_t addr, uint8_t& data)
{
    if (addr <= 0x1FFF)
    {
        if (!chrROM.empty())
            data = chrROM[addr];
        else
            data = 0x00;

        return true;
    }

    return false;
}

bool Mapper0_NROM::ppuWrite(uint16_t addr, uint8_t data)
{
    // Only writable if CHR-RAM
    if (addr <= 0x1FFF && chrROM.empty())
    {
        return true;
    }

    return false;
}
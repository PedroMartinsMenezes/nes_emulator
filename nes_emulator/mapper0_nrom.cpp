#include "mapper0_nrom.h"

Mapper0::Mapper0(uint8_t prgBanks_,
    uint8_t chrBanks_,
    std::vector<uint8_t>& prgROM_,
    std::vector<uint8_t>& prgRAM_,
    std::vector<uint8_t>& chrROM_,
    std::vector<uint8_t>& chrRAM_,
    MIRROR mirrorMode)
    : prgBanks(prgBanks_),
    chrBanks(chrBanks_),
    prgROM(prgROM_),
    prgRAM(prgRAM_),
    chrROM(chrROM_),
    chrRAM(chrRAM_),
    mirror(mirrorMode)
{
    chrIsRam = (chrBanks == 0);
}

bool Mapper0::cpuRead(uint16_t addr, uint8_t& data)
{
    if (addr >= 0x8000)
    {
        if (prgBanks == 1)
        {
            // 16KB mirrored
            data = prgROM[addr & 0x3FFF];
        }
        else
        {
            // 32KB
            data = prgROM[addr & 0x7FFF];
        }
        return true;
    }

    if (addr >= 0x6000 && addr < 0x8000)
    {
        data = prgRAM[addr & 0x1FFF];
        return true;
    }

    return false;
}

bool Mapper0::cpuWrite(uint16_t addr, uint8_t data)
{
    if (addr >= 0x6000 && addr < 0x8000)
    {
        prgRAM[addr & 0x1FFF] = data;
        return true;
    }

    return false;
}

bool Mapper0::ppuRead(uint16_t addr, uint8_t& data)
{
    if (addr <= 0x1FFF)
    {
        if (chrIsRam)
            data = chrRAM[addr];
        else
            data = chrROM[addr];

        return true;
    }

    return false;
}

bool Mapper0::ppuWrite(uint16_t addr, uint8_t data)
{
    if (addr <= 0x1FFF)
    {
        if (chrIsRam)
        {
            chrRAM[addr] = data;
            return true;
        }

        // CHR-ROM -> ignore writes
        return true;
    }

    return false;
}

MIRROR Mapper0::Mirror() const
{
    return mirror;
}

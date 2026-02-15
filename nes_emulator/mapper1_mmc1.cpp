#include "mapper1_mmc1.h"

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------

Mapper1::Mapper1(uint8_t prgBanks,
    uint8_t chrBanks,
    std::vector<uint8_t>& prgRom,
    std::vector<uint8_t>& prgRam,
    std::vector<uint8_t>& chrRom,
    std::vector<uint8_t>& chrRam,
    bool chrIsRam_,
    MIRROR /*initialMirror*/)
    :
    prgROM(prgRom),
    prgRAM(prgRam),
    chrROM(chrRom),
    chrRAM(chrRam),
    chrIsRam(chrIsRam_),
    prgBankCount(prgBanks)
{
    shiftReg = 0x10;   // bit 4 set on reset
    shiftCount = 0;
    control = 0x0C;    // hardware reset value
}

bool Mapper1::cpuRead(uint16_t addr, uint8_t& data)
{
    if (addr >= 0x6000 && addr <= 0x7FFF)
    {
        data = prgRAM[addr & 0x1FFF];
        return true;
    }

    if (addr < 0x8000)
        return false;

    uint32_t bank = 0;
    uint32_t offset = addr & 0x3FFF;

    switch (prgMode())
    {
    case 0:
    case 1:
        // 32KB mode
        bank = (prgBank & 0x0E) % prgBankCount;
        data = prgROM[(bank * 0x4000) + (addr & 0x7FFF)];
        return true;

    case 2:
        // Fix $8000, switch $C000
        if (addr < 0xC000)
        {
            data = prgROM[addr & 0x3FFF];
        }
        else
        {
            bank = prgBank % prgBankCount;
            data = prgROM[(bank * 0x4000) + offset];
        }
        return true;

    case 3:
        // Switch $8000, fix $C000
        if (addr < 0xC000)
        {
            bank = prgBank % prgBankCount;
            data = prgROM[(bank * 0x4000) + offset];
        }
        else
        {
            bank = prgBankCount - 1;
            data = prgROM[(bank * 0x4000) + offset];
        }
        return true;
    }

    return false;
}

bool Mapper1::cpuWrite(uint16_t addr, uint8_t data)
{
    if (addr >= 0x6000 && addr <= 0x7FFF)
    {
        prgRAM[addr & 0x1FFF] = data;
        return true;
    }

    if (addr < 0x8000)
        return false;

    // Reset shift register
    if (data & 0x80)
    {
        shiftReg = 0x10;
        shiftCount = 0;
        control |= 0x0C;
        return true;
    }

    // Shift right, insert bit into bit 4
    shiftReg >>= 1;
    shiftReg |= (data & 1) << 4;
    shiftCount++;

    if (shiftCount == 5)
    {
        uint8_t regData = shiftReg & 0x1F;

        if (addr < 0xA000)
            control = regData;
        else if (addr < 0xC000)
            chrBank0 = regData;
        else if (addr < 0xE000)
            chrBank1 = regData;
        else
            prgBank = regData & 0x0F;

        shiftReg = 0x10;
        shiftCount = 0;
    }

    return true;
}

bool Mapper1::ppuRead(uint16_t addr, uint8_t& data)
{
    if (addr >= 0x2000)
        return false;

    auto& chr = chrIsRam ? chrRAM : chrROM;

    if (chrMode() == 0)
    {
        // 8KB mode
        uint32_t bank = (chrBank0 & 0x1E);
        data = chr[(bank * 0x1000) + addr];
    }
    else
    {
        // 4KB mode
        if (addr < 0x1000)
            data = chr[(chrBank0 * 0x1000) + addr];
        else
            data = chr[(chrBank1 * 0x1000) + (addr - 0x1000)];
    }

    return true;
}

bool Mapper1::ppuWrite(uint16_t addr, uint8_t data)
{
    if (!chrIsRam || addr >= 0x2000)
        return false;

    if (chrMode() == 0)
    {
        uint32_t bank = (chrBank0 & 0x1E);
        chrRAM[(bank * 0x1000) + addr] = data;
    }
    else
    {
        if (addr < 0x1000)
            chrRAM[(chrBank0 * 0x1000) + addr] = data;
        else
            chrRAM[(chrBank1 * 0x1000) + (addr - 0x1000)] = data;
    }

    return true;
}

MIRROR Mapper1::Mirror() const
{
    switch (control & 0x03)
    {
    case 0: return MIRROR::SINGLE0;
    case 1: return MIRROR::SINGLE1;
    case 2: return MIRROR::VERTICAL;
    case 3: return MIRROR::HORIZONTAL;
    }

    return MIRROR::HORIZONTAL;
}

#include "mapper1_mmc1.h"


Mapper1::Mapper1(
    uint8_t prgBanks,
    uint8_t chrBanks,
    std::vector<uint8_t>& prg,
    std::vector<uint8_t>& chr,
    bool chrIsRam)
: 
    prgROM(prg),
    chrMem(chr),
    chrRAM(chrIsRam),
    prgBankCount(prgBanks)
{
}


bool Mapper1::cpuRead(uint16_t addr, uint8_t& data)
{
    if (addr < 0x8000)
        return false;

    uint32_t bank = 0;
    uint32_t offset = 0;

    switch (prgMode())
    {
    case 0:
    case 1:
        // 32 KB mode
        bank = (prgBank & 0x0E);
        offset = addr & 0x7FFF;
        data = prgROM[bank * 0x4000 + offset];
        return true;

    case 2:
        // fix $8000, switch $C000
        if (addr < 0xC000) {
            bank = 0;
            offset = addr & 0x3FFF;
        }
        else {
            bank = prgBank;
            offset = addr & 0x3FFF;
        }
        data = prgROM[bank * 0x4000 + offset];
        return true;

    case 3:
        // switch $8000, fix $C000 (THIS IS YOUR CASE)
        if (addr < 0xC000) {
            bank = prgBank;
            offset = addr & 0x3FFF;
        }
        else {
            bank = prgBankCount - 1;
            offset = addr & 0x3FFF;
        }
        data = prgROM[bank * 0x4000 + offset];
        return true;
    }

    return false;
}

bool Mapper1::cpuWrite(uint16_t addr, uint8_t data)
{
    if (addr < 0x8000)
        return false;

    if (data & 0x80) {
        shiftReg = 0;
        shiftCount = 0;
        control |= 0x0C;
        return true;
    }

    shiftReg >>= 1;
    shiftReg |= (data & 1) << 4;
    shiftCount++;

    if (shiftCount == 5) {
        if (addr < 0xA000)
            control = shiftReg;
        else if (addr < 0xC000)
            chrBank0 = shiftReg;
        else if (addr < 0xE000)
            chrBank1 = shiftReg;
        else
            prgBank = shiftReg & 0x0F;

        shiftReg = 0;
        shiftCount = 0;
    }

    return true;
}

bool Mapper1::ppuRead(uint16_t addr, uint8_t& data)
{
    if (addr >= 0x2000)
        return false;

    if (chrMode() == 0) {
        // 8 KB CHR
        uint32_t bank = (chrBank0 & 0x1E);
        data = chrMem[bank * 0x1000 + addr];
    }
    else {
        // 4 KB CHR
        if (addr < 0x1000) {
            data = chrMem[chrBank0 * 0x1000 + addr];
        }
        else {
            data = chrMem[chrBank1 * 0x1000 + (addr - 0x1000)];
        }
    }
    return true;
}

bool Mapper1::ppuWrite(uint16_t addr, uint8_t data)
{
    if (addr >= 0x2000 || !chrRAM)
        return false;

    if (chrMode() == 0) {
        uint32_t bank = (chrBank0 & 0x1E);
        chrMem[bank * 0x1000 + addr] = data;
    }
    else {
        if (addr < 0x1000)
            chrMem[chrBank0 * 0x1000 + addr] = data;
        else
            chrMem[chrBank1 * 0x1000 + (addr - 0x1000)] = data;
    }
    return true;
}


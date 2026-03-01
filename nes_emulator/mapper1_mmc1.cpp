#include "mapper1_mmc1.h"

Mapper1_MMC1::Mapper1_MMC1(const std::vector<uint8_t>& prg,
    const std::vector<uint8_t>& chr)
    : prgROM(prg), chrROM(chr)
{
    reset();
}

void Mapper1_MMC1::reset()
{
    shiftRegister = 0x10;
    control = 0x0C;
    prgBank = 0;
    chrBank0 = 0;
    chrBank1 = 0;

    updateBanks();
}

void Mapper1_MMC1::updateBanks()
{
    uint8_t prgMode = (control >> 2) & 0x03;
    uint32_t bankCount = prgROM.size() / 0x4000;

    switch (prgMode)
    {
    case 0:
    case 1:
        prgBankOffset0 = (prgBank & 0x0E) * 0x4000;
        prgBankOffset1 = prgBankOffset0 + 0x4000;
        break;

    case 2:
        prgBankOffset0 = 0;
        prgBankOffset1 = prgBank * 0x4000;
        break;

    case 3:
        prgBankOffset0 = prgBank * 0x4000;
        prgBankOffset1 = (bankCount - 1) * 0x4000;
        break;
    }
}

bool Mapper1_MMC1::cpuRead(uint16_t addr, uint8_t& data)
{
    if (addr >= 0x8000)
    {
        if (addr < 0xC000)
        {
            data = prgROM[prgBankOffset0 + (addr & 0x3FFF)];
        }
        else
        {
            data = prgROM[prgBankOffset1 + (addr & 0x3FFF)];
        }
        return true;
    }

    return false;
}

bool Mapper1_MMC1::cpuWrite(uint16_t addr, uint8_t data)
{
    if (addr < 0x8000)
        return false;

    if (data & 0x80)
    {
        shiftRegister = 0x10;
        control |= 0x0C;
        updateBanks();
    }
    else
    {
        bool complete = shiftRegister & 0x01;
        shiftRegister >>= 1;
        shiftRegister |= (data & 1) << 4;

        if (complete)
        {
            uint16_t reg = (addr >> 13) & 0x03;

            switch (reg)
            {
            case 0: control = shiftRegister; break;
            case 1: chrBank0 = shiftRegister; break;
            case 2: chrBank1 = shiftRegister; break;
            case 3: prgBank = shiftRegister & 0x0F; break;
            }

            shiftRegister = 0x10;
            updateBanks();
        }
    }

    return true;
}

bool Mapper1_MMC1::ppuRead(uint16_t addr, uint8_t& data)
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

bool Mapper1_MMC1::ppuWrite(uint16_t addr, uint8_t data)
{
    return false;
}
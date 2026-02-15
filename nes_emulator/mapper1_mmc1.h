#pragma once

#include "IMapper.h"
#include <vector>

class Mapper1 : public IMapper
{
public:
    Mapper1(uint8_t prgBanks,
        uint8_t chrBanks,
        std::vector<uint8_t>& prgRom,
        std::vector<uint8_t>& prgRam,
        std::vector<uint8_t>& chrRom,
        std::vector<uint8_t>& chrRam,
        bool chrIsRam,
        MIRROR initialMirror);

    bool cpuRead(uint16_t addr, uint8_t& data) override;
    bool cpuWrite(uint16_t addr, uint8_t data) override;

    bool ppuRead(uint16_t addr, uint8_t& data) override;
    bool ppuWrite(uint16_t addr, uint8_t data) override;

    MIRROR Mirror() const override;

private:
    // --- ROM / RAM references (NO COPIES) ---
    std::vector<uint8_t>& prgROM;
    std::vector<uint8_t>& prgRAM;
    std::vector<uint8_t>& chrROM;
    std::vector<uint8_t>& chrRAM;

    bool chrIsRam;

    // --- PRG ---
    uint8_t prgBankCount;
    uint8_t prgBank = 0;

    // --- CHR ---
    uint8_t chrBank0 = 0;
    uint8_t chrBank1 = 0;

    // --- MMC1 internal ---
    uint8_t shiftReg = 0x10;   // reset state (bit 4 set)
    uint8_t shiftCount = 0;
    uint8_t control = 0x0C;    // reset value per hardware

    uint8_t prgMode() const { return (control >> 2) & 0x03; }
    uint8_t chrMode() const { return (control >> 4) & 0x01; }
};

#pragma once
#include <cstdint>
#include <vector>
#include "IMapper.h"


class Mapper1 : public IMapper {
public:
    Mapper1(uint8_t prgBanks,
        uint8_t chrBanks,
        std::vector<uint8_t>& prg,
        std::vector<uint8_t>& chr,
        bool chrIsRam);

    bool cpuRead(uint16_t addr, uint8_t& data) override;
    bool cpuWrite(uint16_t addr, uint8_t data) override;

    bool ppuRead(uint16_t addr, uint8_t& data) override;
    bool ppuWrite(uint16_t addr, uint8_t data) override;

private:
    // --- ROM / RAM ---
    std::vector<uint8_t>& prgROM;
    std::vector<uint8_t>& chrMem;
    bool chrRAM;

    // --- PRG ---
    uint8_t prgBankCount;
    uint8_t prgBank = 0;

    // --- CHR ---
    uint8_t chrBank0 = 0;
    uint8_t chrBank1 = 0;

    // --- MMC1 internal ---
    uint8_t shiftReg = 0x00;
    uint8_t shiftCount = 0;
    uint8_t control = 0x0C; // forced reset value

    uint8_t prgMode() const { return (control >> 2) & 0x03; }
    uint8_t chrMode() const { return (control >> 4) & 0x01; }
};


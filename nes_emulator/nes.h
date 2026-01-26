#pragma once
#include "cpu6502.h"
#include "ppu2c02.h"
#include "apu2a03.h"
#include "cartridge.h"
#include "bus.h"

class NES 
{
public:
    NES(const std::string& romPath);

    void reset();
    void clock();

    CPU6502 cpu;
    PPU2C02 ppu;
    APU2A03 apu;
    Cartridge cart;
    Bus bus;

private:
    uint64_t systemClockCounter = 0;
};

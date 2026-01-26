#include "nes.h"

NES::NES(const std::string& romPath) : cart(romPath)
{
    bus.cpu = &cpu;
    bus.ppu = &ppu;
    bus.cart = &cart;

    cpu.connectBus(&bus);
}

void NES::reset()
{
    // Reset system clock counter (optional but recommended)
    systemClockCounter = 0;

    // Reset all components
    cpu.reset();
    ppu.reset();
    apu.reset();

    // Reset cartridge mapper
    cart.reset();
}


void NES::clock()
{
    // PPU runs 3x faster than CPU
    ppu.clock();
    ppu.clock();
    ppu.clock();

    cpu.clock();

    // Trigger NMI if PPU requests it
    if (ppu.nmi)
    {
        ppu.nmi = false;
        cpu.nmi();
    }
}


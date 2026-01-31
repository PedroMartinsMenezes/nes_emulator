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
    cart.reset();
    ppu.reset();
    apu.reset();
    cpu.reset();
}


void NES::clock()
{
    // PPU always clocks
    ppu.clock();

    // CPU & APU clock every 3 PPU cycles
    if (systemClockCounter % 3 == 0)
    {
        if (bus.dmaActive)
        {
            bus.clockDMA(); // steals CPU cycles
        }
        else
        {
            cpu.clock();
        }

        apu.clock();
    }

    // Handle NMI
    if (ppu.nmi)
    {
        ppu.nmi = false;
        cpu.nmi();
    }

    // Handle IRQs
    if (apu.irq || cart.irq)
    {
        cpu.irq();
    }

    systemClockCounter++;
}

#include "nes.h"

NES::NES()
{
    // Connect bus
    bus.connectCPU(&cpu);
    bus.connectPPU(&ppu);
    bus.connectAPU(&apu);

    cpu.connectBus(&bus);

    // CPU needs access to NES for timing
    cpu.setNES(this);
}

NES::~NES()
{
}

bool NES::insertCartridge(const std::shared_ptr<Cartridge>& cart)
{
    cartridge = cart;
    bus.insertCartridge(cart);
    return true;
}

void NES::reset()
{
    cpu.reset();
    ppu.reset();
    apu.reset();
}

void NES::run()
{
    running = true;

    while (running)
    {
        cpu.ExecOp(); //  All timing happens inside MemRead/MemWrite
    }
}

void NES::runFrame()
{
    // Run until PPU completes one frame
    int startingFrame = ppu.frameCounter;

    while (ppu.frameCounter == startingFrame)
    {
        cpu.ExecOp();
    }
}

void NES::clockCPU()
{
    //
    // This is called once per CPU cycle
    // (from CPU::RunCycle())
    //

    // 3 PPU cycles per CPU cycle
    ppu.clock();
    ppu.clock();
    ppu.clock();

    // APU runs once per CPU cycle
    apu.clock();

    // Handle DMA stall if active
    if (bus.dmaActive)
    {
        bus.clockDMA();
    }
}
#include "bus.h"
#include "cpu6502.h"
#include "ppu2c02.h"
#include "apu2a03.h"
#include "cartridge.h"

Bus::Bus()
{
    for (auto& b : cpuRam)
        b = 0x00;
}

void Bus::connectCPU(CPU6502* c)
{
    cpu = c;
}

void Bus::connectPPU(PPU2C02* p)
{
    ppu = p;
}

void Bus::connectAPU(APU2A03* a)
{
    apu = a;
}

void Bus::insertCartridge(const std::shared_ptr<Cartridge>& cart)
{
    cartridge = cart;
}

uint8_t Bus::cpuRead(uint16_t addr)
{
    uint8_t data = 0x00;

    // Cartridge space
    if (cartridge && cartridge->cpuRead(addr, data))
        return data;

    // 2KB internal RAM (mirrored every 0x800)
    if (addr <= 0x1FFF)
    {
        return cpuRam[addr & 0x07FF];
    }

    // PPU registers (mirrored every 8 bytes)
    if (addr >= 0x2000 && addr <= 0x3FFF)
    {
        // Real implementation would call PPU register read
        return 0x00;
    }

    // APU + I/O
    if (addr >= 0x4000 && addr <= 0x4017)
    {
        return 0x00;
    }

    return data;
}

void Bus::cpuWrite(uint16_t addr, uint8_t data)
{
    // Cartridge first
    if (cartridge && cartridge->cpuWrite(addr, data))
        return;

    // 2KB internal RAM
    if (addr <= 0x1FFF)
    {
        cpuRam[addr & 0x07FF] = data;
        return;
    }

    // PPU registers (stub)
    if (addr >= 0x2000 && addr <= 0x3FFF)
    {
        return;
    }

    // DMA trigger
    if (addr == 0x4014)
    {
        dmaPage = data;
        dmaAddr = 0x00;
        dmaActive = true;
        return;
    }

    // APU + I/O (stub)
    if (addr >= 0x4000 && addr <= 0x4017)
    {
        return;
    }
}

void Bus::clockDMA()
{
    if (!dmaActive)
        return;

    // DMA alternates read/write each CPU cycle
    if ((cpu->totalCycles % 2) == 0)
    {
        // Read from CPU memory
        uint16_t addr = (dmaPage << 8) | dmaAddr;
        dmaData = cpuRead(addr);
    }
    else
    {
        // Write to PPU OAM (not implemented yet)
        dmaAddr++;

        if (dmaAddr == 0x00)
        {
            dmaActive = false;
        }
    }
}
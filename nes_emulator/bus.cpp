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

void Bus::connectCPU(CPU6502* c) { cpu = c; }
void Bus::connectPPU(PPU2C02* p) { ppu = p; }
void Bus::connectAPU(APU2A03* a) { apu = a; }

void Bus::insertCartridge(const std::shared_ptr<Cartridge>& cart)
{
    cartridge = cart;
}

uint8_t Bus::cpuRead(uint16_t addr)
{
    uint8_t data = 0x00;

    if (cartridge && cartridge->cpuRead(addr, data))
        return data;

    if (addr <= 0x1FFF)
        return cpuRam[addr & 0x07FF];

    if (addr >= 0x2000 && addr <= 0x3FFF)
        return ppu->cpuRead(addr & 0x0007);

    if (addr >= 0x4000 && addr <= 0x4017)
        return 0x00;

    return data;
}

void Bus::cpuWrite(uint16_t addr, uint8_t data)
{
    if (cartridge && cartridge->cpuWrite(addr, data))
        return;

    if (addr <= 0x1FFF)
    {
        cpuRam[addr & 0x07FF] = data;
        return;
    }

    if (addr >= 0x2000 && addr <= 0x3FFF)
    {
        ppu->cpuWrite(addr & 0x0007, data);
        return;
    }

    if (addr == 0x4014)
    {
        dmaPage = data;
        dmaAddr = 0x00;
        dmaActive = true;
        dmaDummy = true;
        return;
    }
}

void Bus::clockDMA()
{
    if (!dmaActive)
        return;

    if (dmaDummy)
    {
        if (cpu->totalCycles % 2 == 1)
            return;

        dmaDummy = false;
        return;
    }

    if (cpu->totalCycles % 2 == 0)
    {
        uint16_t addr = (dmaPage << 8) | dmaAddr;
        dmaData = cpuRead(addr);
    }
    else
    {
        ppu->writeOAM(dmaAddr, dmaData);
        dmaAddr++;

        if (dmaAddr == 0x00)
        {
            dmaActive = false;
            dmaDummy = true;
        }
    }
}
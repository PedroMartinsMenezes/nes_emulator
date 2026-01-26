#include "bus.h"
#include "cpu6502.h"
#include "cartridge.h"
#include "ppu2c02.h"

Bus::Bus() {
    ram.fill(0x00);
}

uint8_t Bus::cpuRead(uint16_t addr, bool readOnly) {
    
    uint8_t data = 0x00;

    // Internal RAM ($0000 – $1FFF)
    if (addr <= 0x1FFF)
        return ram[addr & 0x07FF];

    // PPU registers ($2000 – $3FFF)
    if (addr >= 0x2000 && addr <= 0x3FFF)
        return ppu->cpuRead(addr & 7, readOnly);

    // APU + IO registers ($4000 – $4017)
    if (addr >= 0x4000 && addr <= 0x4017)
        return 0xFF;   // OPEN BUS (correct for now)

    // Disabled ($4018 – $401F)
    if (addr >= 0x4018 && addr <= 0x401F)
        return 0x00;

    // Cartridge space ($4020 – $FFFF)
    if (cart && cart->cpuRead(addr, data))
        return data;

    return 0x00;
}


void Bus::cpuWrite(uint16_t addr, uint8_t data) {
    
    // Internal RAM ($0000 – $1FFF)
    if (addr <= 0x1FFF)
    {
        ram[addr & 0x07FF] = data;
        return;
    }
    // PPU registers ($2000 – $3FFF)
    if (addr >= 0x2000 && addr <= 0x3FFF)
    {
        ppu->cpuWrite(addr & 0x07, data);
        return;
    }
    // APU + IO ($4000 – $4017)
    if (addr >= 0x4000 && addr <= 0x4017)
    {
        // $4014 = OAM DMA (VERY IMPORTANT)
        if (addr == 0x4014)
        {
            uint16_t base = uint16_t(data) << 8;
            for (int i = 0; i < 256; i++)
                ppu->oam[i] = cpuRead(base + i);
            cpu->stall += 513;
            return;
        }
        // TODO: APU + controllers later
        return;
    }

    // Disabled ($4018–$401F)
    if (addr >= 0x4018 && addr <= 0x401F)
        return;

    // Cartridge space ($4020–$FFFF)
    if (cart)
        cart->cpuWrite(addr, data);
}


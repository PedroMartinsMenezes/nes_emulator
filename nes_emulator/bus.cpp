#include "bus.h"
#include "cpu6502.h"
#include "cartridge.h"
#include "ppu2c02.h"

Bus::Bus() {
    ram.fill(0x00);
}

uint8_t Bus::cpuRead(uint16_t addr, bool readOnly) {
    uint8_t data = 0x00;

    if (nestestMode) {
        if (addr < 0x8000) 
            return ram[addr & 0x07FF];
        //return 
        //    prgROM[addr - 0x8000];
    }

    // Cartridge (PRG-ROM / mapper)
    if (cart && cart->cpuRead(addr, data)) {
        return data;
    }

    // Internal RAM ($0000–$1FFF)
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        return ram[addr & 0x07FF];
    }

    // PPU registers ($2000–$3FFF)
    if (addr >= 0x2000 && addr <= 0x3FFF) {
        if (nestestMode)
            return 0x00;

        return PPU->cpuRead(addr & 0x0007, readOnly);
    }

    // APU + IO ($4000–$4017)
    if (addr >= 0x4000 && addr <= 0x4017) {
        if (nestestMode)
            return 0x00;

        // TODO: APU + controller
        return 0x00;
    }

    // Disabled ($4018–$401F)
    if (addr >= 0x4018 && addr <= 0x401F) {
        return 0x00;
    }

    return data;
}

void Bus::cpuWrite(uint16_t addr, uint8_t data) {

    // Cartridge (mapper writes)
    if (cart) {
        cart->cpuWrite(addr, data);
        return;
    }

    // Internal RAM ($0000–$1FFF)
    if (addr >= 0x0000 && addr <= 0x1FFF) {
        ram[addr & 0x07FF] = data;
        return;
    }

    // PPU registers ($2000–$3FFF)
    if (addr >= 0x2000 && addr <= 0x3FFF) {
        if (nestestMode)
            return;

        PPU->cpuWrite(addr & 0x0007, data);
        return;
    }

    // APU + IO ($4000–$4017)
    if (addr >= 0x4000 && addr <= 0x4017) {
        if (nestestMode)
            return;

        // $4014 — OAM DMA (PPU)
        //if (addr == 0x4014) {
        //    uint16_t base = uint16_t(data) << 8;
        //    for (int i = 0; i < 256; i++) {
        //        uint8_t val = cpuRead(base + i);
        //        PPU->oamWrite(i, val);
        //    }
        //    CPU->cycles += 513; // DMA stall
        //    return;
        //}

        // TODO: APU + controller writes
        return;
    }

    // Disabled ($4018–$401F)
    if (addr >= 0x4018 && addr <= 0x401F) {
        return;
    }
}

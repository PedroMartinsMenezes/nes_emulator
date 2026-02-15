#include "bus.h"
#include "cpu6502.h"
#include "cartridge.h"
#include "ppu2c02.h"


Bus::Bus() 
{
    ram.fill(0x00);
}

void Bus::reset()
{
    systemClockCounter = 0;
}

uint8_t Bus::cpuRead(uint16_t addr, bool readOnly) 
{
    uint8_t data = 0x00;

    // Internal RAM ($0000 – $1FFF)
    if (addr <= 0x1FFF)
        return ram[addr & 0x07FF];

    // PPU registers ($2000 – $3FFF)
    if (addr >= 0x2000 && addr <= 0x3FFF)
        return ppu->cpuRead(addr & 7);

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

void Bus::cpuWrite(uint16_t addr, uint8_t data) 
{
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

uint8_t Bus::ppuRead(uint16_t addr)
{
    addr &= 0x3FFF;

    uint8_t data = 0x00;

    // -------------------------------------------------
    // 0x0000–0x1FFF : Pattern tables (CHR)
    // -------------------------------------------------
    if (addr <= 0x1FFF)
    {
        if (cart->ppuRead(addr, data))
            return data;

        // If mapper did not handle it, return 0
        return 0x00;
    }

    // -------------------------------------------------
    // 0x2000–0x3EFF : Nametables (mirrored every 4KB)
    // -------------------------------------------------
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;

        // Mirror mode from cartridge
        switch (cart->GetMapper()->Mirror())
        {
        case MIRROR::HORIZONTAL:
        {
            // [ A A B B ]
            if (addr < 0x0400)       return ppu->tblName[0][addr & 0x03FF];
            else if (addr < 0x0800)  return ppu->tblName[0][addr & 0x03FF];
            else if (addr < 0x0C00)  return ppu->tblName[1][addr & 0x03FF];
            else                     return ppu->tblName[1][addr & 0x03FF];
        }

        case MIRROR::VERTICAL:
        {
            // [ A B A B ]
            if (addr < 0x0400)       return ppu->tblName[0][addr & 0x03FF];
            else if (addr < 0x0800)  return ppu->tblName[1][addr & 0x03FF];
            else if (addr < 0x0C00)  return ppu->tblName[0][addr & 0x03FF];
            else                     return ppu->tblName[1][addr & 0x03FF];
        }

        case MIRROR::SINGLE0:
            return ppu->tblName[0][addr & 0x03FF];

        case MIRROR::SINGLE1:
            return ppu->tblName[1][addr & 0x03FF];
        }
    }

    // -------------------------------------------------
    // 0x3F00–0x3FFF : Palette RAM
    // -------------------------------------------------
    else if (addr <= 0x3FFF)
    {
        addr &= 0x001F;

        // Palette mirroring
        if (addr == 0x10) addr = 0x00;
        if (addr == 0x14) addr = 0x04;
        if (addr == 0x18) addr = 0x08;
        if (addr == 0x1C) addr = 0x0C;

        return ppu->tblPalette[addr];
    }

    return 0x00;
}

void Bus::ppuWrite(uint16_t addr, uint8_t data)
{
    addr &= 0x3FFF;

    // -------------------------------------------------
    // 0x0000–0x1FFF : Pattern tables (CHR)
    // -------------------------------------------------
    if (addr <= 0x1FFF)
    {
        // Let mapper decide (CHR-ROM or CHR-RAM)
        if (cart->ppuWrite(addr, data))
            return;

        // If mapper does not handle it (CHR-ROM), ignore write
        return;
    }

    // -------------------------------------------------
    // 0x2000–0x3EFF : Nametables (mirrored every 4KB)
    // -------------------------------------------------
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;

        switch (cart->GetMapper()->Mirror())
        {
        case MIRROR::HORIZONTAL:
        {
            // [ A A B B ]
            if (addr < 0x0400)        ppu->tblName[0][addr & 0x03FF] = data;
            else if (addr < 0x0800)   ppu->tblName[0][addr & 0x03FF] = data;
            else if (addr < 0x0C00)   ppu->tblName[1][addr & 0x03FF] = data;
            else                      ppu->tblName[1][addr & 0x03FF] = data;
            return;
        }

        case MIRROR::VERTICAL:
        {
            // [ A B A B ]
            if (addr < 0x0400)        ppu->tblName[0][addr & 0x03FF] = data;
            else if (addr < 0x0800)   ppu->tblName[1][addr & 0x03FF] = data;
            else if (addr < 0x0C00)   ppu->tblName[0][addr & 0x03FF] = data;
            else                      ppu->tblName[1][addr & 0x03FF] = data;
            return;
        }

        case MIRROR::SINGLE0:
        {
            ppu->tblName[0][addr & 0x03FF] = data;
            return;
        }

        case MIRROR::SINGLE1:
        {
            ppu->tblName[1][addr & 0x03FF] = data;
            return;
        }
        }
    }

    // -------------------------------------------------
    // 0x3F00–0x3FFF : Palette RAM
    // -------------------------------------------------
    else if (addr <= 0x3FFF)
    {
        addr &= 0x001F;

        // Palette mirroring
        if (addr == 0x10) addr = 0x00;
        if (addr == 0x14) addr = 0x04;
        if (addr == 0x18) addr = 0x08;
        if (addr == 0x1C) addr = 0x0C;

        ppu->tblPalette[addr] = data;
        return;
    }
}

void Bus::clockDMA() 
{
    //TODO Incomplete
}


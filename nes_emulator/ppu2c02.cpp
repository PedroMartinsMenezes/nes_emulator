#include "ppu2c02.h"
#include <iostream>
#include "bus.h"
#include "cartridge.h"
#include "cpu6502.h"


PPU2C02::PPU2C02() 
{
}

void PPU2C02::connectBus(Bus* bus)
{
    this->bus = bus;
}

void PPU2C02::reset()
{
    PPUCTRL = 0;
    PPUMASK = 0;
    PPUSTATUS = 0;

    vram_addr = 0;
    tram_addr = 0;
    fine_x = 0;
    write_latch = false;

    data_buffer = 0;

    scanline = 0;
    cycle = 0;

    frame = 0;

    nmiLine = false;
}

// https://www.nesdev.org/wiki/PPU_registers
uint8_t PPU2C02::cpuRead(uint16_t addr)
{
    uint8_t data = cpuDataBus;

    switch (addr & 7)
    {
    case 2: // $2002 PPUSTATUS
    {
        // Start with actual register
        data = PPUSTATUS;

        // Replace lower 5 bits with open bus
        data = (data & 0xE0) | (cpuDataBus & 0x1F);

        // Clear VBlank
        PPUSTATUS &= ~0x80;
        vblankFlag = false;
        nmiOccurred = false;

        // Clear write toggle
        write_latch = false;

        // NMI suppression window
        //if (scanline == 241 && cycle == 0)
        //    nmiLine = false;

        break;
    }

    case 7: // $2007 PPUDATA
    {
        data = data_buffer;

        data_buffer = ppuRead(vram_addr);

        // Palette reads are NOT buffered
        if (vram_addr >= 0x3F00)
            data = data_buffer;

        vram_addr += (PPUCTRL & 0x04) ? 32 : 1;
        break;
    }

    default:
        break;
    }

    cpuDataBus = data;
    return data;
}

void PPU2C02::cpuWrite(uint16_t addr, uint8_t data)
{
    cpuDataBus = data;

    switch (addr & 7)
    {
    case 0: // $2000 PPUCTRL
    {
        bool oldOutput = nmiOutput;

        PPUCTRL = data;
        nmiOutput = data & 0x80;

        tram_addr = (tram_addr & 0xF3FF) | ((data & 0x03) << 10);

        // Immediate NMI if enabling during VBlank
        if (!oldOutput && nmiOutput && vblankFlag)
        {
            if (!(scanline == 241 && cycle == 0))
                nmiOccurred = true;
        }
        break;
    }
    case 1: // $2001 PPUMASK
        PPUMASK = data;
        break;

    case 3: // $2003 OAMADDR
        OAMADDR = data;
        break;

    case 4: // $2004 OAMDATA
        oam[OAMADDR++] = data;
        break;

    case 5: // $2005 PPUSCROLL
        if (!write_latch)
        {
            fine_x = data & 7;
            tram_addr = (tram_addr & 0xFFE0) | (data >> 3);
            write_latch = true;
        }
        else
        {
            tram_addr = (tram_addr & 0x8FFF) | ((data & 7) << 12);
            tram_addr = (tram_addr & 0xFC1F) | ((data & 0xF8) << 2);
            write_latch = false;
        }
        break;

    case 6: // $2006 PPUADDR
        if (!write_latch)
        {
            tram_addr = (tram_addr & 0x00FF) | ((data & 0x3F) << 8);
            write_latch = true;
        }
        else
        {
            tram_addr = (tram_addr & 0xFF00) | data;
            vram_addr = tram_addr;
            write_latch = false;
        }
        break;

    case 7: // $2007 PPUDATA
        ppuWrite(vram_addr, data);
        vram_addr += (PPUCTRL & 0x04) ? 32 : 1;
        break;
    }
}

uint8_t PPU2C02::ppuRead(uint16_t addr)
{
    addr &= 0x3FFF;

    if (addr <= 0x1FFF)
    {
        return bus->ppuRead(addr);
    }
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;

        if (bus->cart->GetMapper()->Mirror() == HORIZONTAL)
        {
            if (addr < 0x400) return tblName[0][addr & 0x03FF];
            if (addr < 0x800) return tblName[0][addr & 0x03FF];
            if (addr < 0xC00) return tblName[1][addr & 0x03FF];
            return tblName[1][addr & 0x03FF];
        }
        else
        {
            if (addr < 0x400) return tblName[0][addr & 0x03FF];
            if (addr < 0x800) return tblName[1][addr & 0x03FF];
            if (addr < 0xC00) return tblName[0][addr & 0x03FF];
            return tblName[1][addr & 0x03FF];
        }
    }
    else if (addr <= 0x3FFF)
    {
        addr &= 0x1F;
        if (addr == 0x10) addr = 0x00;
        if (addr == 0x14) addr = 0x04;
        if (addr == 0x18) addr = 0x08;
        if (addr == 0x1C) addr = 0x0C;
        return tblPalette[addr];
    }

    return 0;
}

void PPU2C02::ppuWrite(uint16_t addr, uint8_t data)
{
    addr &= 0x3FFF;

    if (addr <= 0x1FFF)
    {
        bus->cart->ppuWrite(addr, data);
    }
    else if (addr <= 0x3EFF)
    {
        addr &= 0x0FFF;

        if (bus->cart->GetMapper()->Mirror() == HORIZONTAL)
        {
            if (addr < 0x400) tblName[0][addr & 0x03FF] = data;
            else if (addr < 0x800) tblName[0][addr & 0x03FF] = data;
            else if (addr < 0xC00) tblName[1][addr & 0x03FF] = data;
            else tblName[1][addr & 0x03FF] = data;
        }
        else
        {
            if (addr < 0x400) tblName[0][addr & 0x03FF] = data;
            else if (addr < 0x800) tblName[1][addr & 0x03FF] = data;
            else if (addr < 0xC00) tblName[0][addr & 0x03FF] = data;
            else tblName[1][addr & 0x03FF] = data;
        }
    }
    else if (addr <= 0x3FFF)
    {
        addr &= 0x1F;
        if (addr == 0x10) addr = 0x00;
        if (addr == 0x14) addr = 0x04;
        if (addr == 0x18) addr = 0x08;
        if (addr == 0x1C) addr = 0x0C;
        tblPalette[addr] = data;
    }
}

void PPU2C02::clock()
{
    cycle++;

    if (cycle >= 341)
    {
        cycle = 0;
        scanline++;

        if (scanline >= 262)
        {
            scanline = 0;
            frame++;
        }
    }

    // VBlank start
    if (scanline == 241 && cycle == 0)
    {
        vblankFlag = true;
        PPUSTATUS |= 0x80;

        if (nmiOutput)
            nmiOccurred = true;
    }

    // Pre-render line
    if (scanline == 261 && cycle == 1)
    {
        vblankFlag = false;
        PPUSTATUS &= ~0x80;

        nmiOccurred = false;
    }

    // NMI line is AND of internal wires
    nmiLine = nmiOccurred && nmiOutput;
}

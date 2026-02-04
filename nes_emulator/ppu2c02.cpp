#include "ppu2c02.h"
#include <iostream>


PPU2C02::PPU2C02() {
    
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

    nmi = false;
}

// https://www.nesdev.org/wiki/PPU_registers
uint8_t PPU2C02::cpuRead(uint16_t addr, bool readOnly) 
{
    uint8_t data = 0xFF;

    switch (addr & 7)
    {
    case 2: // $2002
        data = (PPUSTATUS & 0xE0) | (data_buffer & 0x1F);
        PPUSTATUS &= ~0x80;
        write_latch = false;
        break;

    case 4: // $2004
        data = 0x00; // sprites later
        break;

    case 7: // $2007
        data = data_buffer;
        data_buffer = ppuRead(vram_addr);

        if (vram_addr >= 0x3F00)
            data = data_buffer;

        vram_addr += (PPUCTRL & 0x04) ? 32 : 1;
        break;
    }

    return data;
}

void PPU2C02::cpuWrite(uint16_t addr, uint8_t data) {
    switch (addr & 7)
    {
    case 0: // $2000
        PPUCTRL = data;
        tram_addr = (tram_addr & 0xF3FF) | ((data & 0x03) << 10);
        break;

    case 1: // $2001
        PPUMASK = data;
        break;

    case 2: // $2002 (read only)
        break;

    case 3: // $2003
        OAMADDR = data;
        break;

    case 4: // $2004
        // ignore for now
        break;

    case 5: // $2005
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

    case 6: // $2006
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

    case 7: // $2007
        ppuWrite(vram_addr, data);
        vram_addr += (PPUCTRL & 0x04) ? 32 : 1;
        break;
    }
}

uint8_t PPU2C02::ppuRead(uint16_t addr)
{
    addr &= 0x3FFF;

    if (addr <= 0x1FFF)
        return tblPattern[(addr >> 12) & 1][addr & 0x0FFF];
    else if (addr <= 0x3EFF)
        return tblName[(addr >> 10) & 1][addr & 0x03FF];
    else
        return tblPalette[addr & 0x1F];
}


void PPU2C02::ppuWrite(uint16_t addr, uint8_t data)
{
    addr &= 0x3FFF;

    if (addr <= 0x1FFF)
        tblPattern[(addr >> 12) & 1][addr & 0x0FFF] = data;
    else if (addr <= 0x3EFF)
        tblName[(addr >> 10) & 1][addr & 0x03FF] = data;
    else
        tblPalette[addr & 0x1F] = data;
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

    //if (scanline == 241 && cycle == 1)
    //    printf("NMI\n");

    // VBlank start
    if (scanline == 241 && cycle == 1)
    {
        PPUSTATUS |= (uint8_t)PPU_Status::VBlank;
        if (PPUCTRL & (uint8_t)PPU_Status::VBlank)
            nmi = true;
    }

    // VBlank end
    if (scanline == 261 && cycle == 1)
    {
        PPUSTATUS &= ~(uint8_t)PPU_Status::VBlank;
        PPUSTATUS &= ~(uint8_t)PPU_Status::SpriteZero;
        PPUSTATUS &= ~(uint8_t)PPU_Status::SpriteOverflow;
    }


}

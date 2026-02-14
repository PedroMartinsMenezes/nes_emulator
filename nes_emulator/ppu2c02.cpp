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
    uint8_t data = 0x00;

    switch (addr & 7)
    {
    case 2: // $2002
        //reading bits 7,6,5 from PpuStatus and bits 4,3,2,1,0 from OpenBus (CpuDataBus)
        write_latch = false;
        data = (PPUSTATUS & 0xE0) | (cpuDataBus & 0x1F);
        if (data & 0x80)
            PPUSTATUS &= 0x60;
        // race conditions
        if (scanline == 241)
        {
            if ((cycle == 0))
                data &= ~0x80; //unset VBlank
            if (cycle < 3)
                nmi = false;
        }
        cpuDataBus = data;
        break;

    case 4: // $2004
        data = 0x00; // sprites later
        cpuDataBus = data;
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

    cpuDataBus = data;

    switch (addr & 7)
    {
    case 0: // $2000
        if ((data & 0x80) && !(PPUCTRL & 0x80) && (PPUSTATUS & 0x80) && (scanline != -1))
            nmi = true;
        // race condition
        if ((scanline == 241) && !(data & 0x80) && (cycle < 3))
            nmi = false;
        PPUCTRL = data;
        tram_addr = (tram_addr & 0xF3FF) | ((data & 0x03) << 10);
        break;

    case 1: // $2001
        PPUMASK = data;
        isRendering = ((PPUMASK & 0x18) && (scanline < 240));
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

void PPU2C02::clocks(int cpuCycles) {
    for (int i = 0; i < cpuCycles * 3; i++) {
        clock();
    }
}

void PPU2C02::clock()
{
    // Advance PPU timing
    cycle++;

    if (cycle == 338)
    {
        scanlineLength = (scanline == -1 && shortScanline && isRendering) ? 340 : 341;
    }
    else if (cycle == scanlineLength)
    {
        cycle = 0;
        scanline++;

        if (scanline == 240)
        {
            isRendering = false;
        }
        else if (scanline == 241) // VBlank start
        {
            PPUSTATUS |= 0x80; //Set VBlank
            if (PPUCTRL & 0x80)
                nmi = true; // Enable NMI if PPUCTRL has bit 7 set
        }
        else if (scanline == 261)
        {
            frame++;
            scanline = -1;
            shortScanline = !shortScanline;
            isRendering = (PPUMASK & 0x18) > 0;
            PPUSTATUS &= ~0x60; // Sprite flags are cleared immediately
        }
    }
    else if ((scanline == -1) && (cycle == 1))
    {
        // VBL flag gets cleared a cycle late
        PPUSTATUS &= ~0x80;
    }

    // Pre-render line (scanline 261, cycle 1)
    // Clear VBlank and sprite flags
    if (scanline == 261 && cycle == 1)
    {
        PPUSTATUS &= ~(uint8_t)PPU_Status::VBlank;
        PPUSTATUS &= ~(uint8_t)PPU_Status::SpriteZero;
        PPUSTATUS &= ~(uint8_t)PPU_Status::SpriteOverflow;
    }
}


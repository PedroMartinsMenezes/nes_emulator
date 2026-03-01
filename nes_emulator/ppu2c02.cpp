#include "ppu2c02.h"

PPU2C02::PPU2C02()
{
}

void PPU2C02::reset()
{
    cycle = 0;
    scanline = 0;
    frameCounter = 0;
    vblankFlag = false;
}

void PPU2C02::clock()
{
    cycle++;

    if (cycle >= 341)
    {
        cycle = 0;
        scanline++;

        if (scanline == 241)
        {
            vblankFlag = true;
        }

        if (scanline >= 262)
        {
            scanline = 0;
            frameCounter++;
            vblankFlag = false;
        }
    }
}
#include "ppu2c02.h"
#include "nes.h"

PPU2C02::PPU2C02() {}

void PPU2C02::connectNES(NES* n)
{
    nes = n;
}

void PPU2C02::reset()
{
    scanline = 261;
    cycle = 0;
    frame = 0;

    oddFrame = false;

    nmiOccurred = false;
    nmiPrevious = false;
    PPUSTATUS = 0;

    w = false;
}

void PPU2C02::clock()
{
    // Pre-render: clear VBlank
    if (scanline == 261 && cycle == 1)
    {
        nmiOccurred = false;
        PPUSTATUS &= ~0x80;
        PPUSTATUS &= ~0x40;
        PPUSTATUS &= ~0x20;
    }

    // VBlank start
    if (scanline == 241 && cycle == 1)
    {
        nmiOccurred = true;
        PPUSTATUS |= 0x80;
    }

    updateNMI();

    // Sprite evaluation window
    if (scanline >= 0 && scanline < 240)
    {
        if (cycle == 1)
            spriteCount = 0;

        if (cycle >= 65 && cycle <= 256)
        {
            int spriteIndex = (cycle - 65) / 3;

            if (spriteIndex < 64)
            {
                uint8_t y = oam[spriteIndex * 4];

                if (scanline >= y && scanline < y + 8)
                {
                    if (spriteCount < 8)
                    {
                        for (int i = 0; i < 4; i++)
                            secondaryOAM[spriteCount * 4 + i] =
                            oam[spriteIndex * 4 + i];

                        spriteCount++;
                    }
                    else
                    {
                        PPUSTATUS |= 0x20;
                    }
                }
            }
        }
    }

    // Background fetch pipeline (simplified timing model)
    if (scanline >= 0 && scanline < 240)
    {
        if ((PPUMASK & 0x08) || (PPUMASK & 0x10))
        {
            switch (cycle % 8)
            {
            case 1:
                bgNextTileID = vram[v & 0x07FF];
                break;

            case 3:
                bgNextTileAttrib = vram[0x03C0 | (v & 0x0C00)
                    | ((v >> 4) & 0x38)
                    | ((v >> 2) & 0x07)];
                break;

            case 5:
                bgNextTileLsb = vram[(PPUCTRL & 0x10 ? 0x1000 : 0)
                    + bgNextTileID * 16
                    + ((v >> 12) & 7)];
                break;

            case 7:
                bgNextTileMsb = vram[(PPUCTRL & 0x10 ? 0x1000 : 0)
                    + bgNextTileID * 16
                    + ((v >> 12) & 7) + 8];
                break;

            case 0:
                if ((v & 0x001F) == 31)
                {
                    v &= ~0x001F;
                    v ^= 0x0400;
                }
                else
                {
                    v++;
                }
                break;
            }
        }
    }

    cycle++;

    if (cycle >= 341)
    {
        cycle = 0;
        scanline++;

        if (scanline >= 262)
        {
            scanline = 0;
            frame++;
            oddFrame = !oddFrame;
        }
    }

    // Odd frame skip
    if (scanline == 0 &&
        cycle == 0 &&
        oddFrame &&
        (PPUMASK & 0x18))
    {
        cycle = 1;
    }
}

void PPU2C02::updateNMI()
{
    bool nmi = nmiOutput && nmiOccurred;

    if (!nmiPrevious && nmi)
        nes->cpu.requestNMI();

    nmiPrevious = nmi;
}

uint8_t PPU2C02::cpuRead(uint16_t addr)
{
    uint8_t data = ppuDataBus;

    switch (addr)
    {
    case 2:
        data = (PPUSTATUS & 0xE0) | (ppuDataBus & 0x1F);
        nmiOccurred = false;
        PPUSTATUS &= ~0x80;
        w = false;
        break;

    case 4:
        data = oam[OAMADDR];
        break;

    case 7:
        data = bufferedData;
        bufferedData = vram[v & 0x07FF];
        v += (PPUCTRL & 0x04) ? 32 : 1;
        break;
    }

    ppuDataBus = data;
    return data;
}

void PPU2C02::cpuWrite(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
    case 0:
    {
        bool old = nmiOutput;
        PPUCTRL = data;
        nmiOutput = PPUCTRL & 0x80;
        t = (t & 0xF3FF) | ((data & 0x03) << 10);

        if (!old && nmiOutput && nmiOccurred)
            nes->cpu.requestNMI();
        break;
    }

    case 1:
        PPUMASK = data;
        break;

    case 3:
        OAMADDR = data;
        break;

    case 4:
        oam[OAMADDR++] = data;
        break;

    case 5:
        if (!w)
        {
            x = data & 0x07;
            t = (t & 0xFFE0) | (data >> 3);
            w = true;
        }
        else
        {
            t = (t & 0x8FFF) | ((data & 0x07) << 12);
            t = (t & 0xFC1F) | ((data & 0xF8) << 2);
            w = false;
        }
        break;

    case 6:
        if (!w)
        {
            t = (t & 0x00FF) | ((data & 0x3F) << 8);
            w = true;
        }
        else
        {
            t = (t & 0xFF00) | data;
            v = t;
            w = false;
        }
        break;

    case 7:
        vram[v & 0x07FF] = data;
        v += (PPUCTRL & 0x04) ? 32 : 1;
        break;
    }

    ppuDataBus = data;
}

void PPU2C02::writeOAM(uint8_t addr, uint8_t data)
{
    oam[addr] = data;
}
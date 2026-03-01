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
    // -------------------------
    // VBlank timing
    // -------------------------
    if (scanline == 261 && cycle == 1)
    {
        nmiOccurred = false;
        PPUSTATUS &= ~0x80;
        PPUSTATUS &= ~0x40;
        PPUSTATUS &= ~0x20;
    }

    if (scanline == 241 && cycle == 1)
    {
        nmiOccurred = true;
        PPUSTATUS |= 0x80;
    }

    updateNMI();

    bool renderingEnabled = PPUMASK & 0x18;

    // -------------------------
    // Visible + pre-render logic
    // -------------------------
    if (scanline >= -1 && scanline < 240)
    {
        if (renderingEnabled)
        {
            // Background shift registers shift every cycle
            if (cycle >= 2 && cycle < 258)
                updateShifters();

            // Tile fetch pipeline
            switch ((cycle - 1) % 8)
            {
            case 0: loadBackgroundShifters(); break;

            case 1:
                bgNextTileID = vram[v & 0x07FF];
                break;

            case 3:
            {
                uint16_t addr = 0x03C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07);
                bgNextTileAttrib = vram[addr];
                break;
            }

            case 5:
                bgNextTileLsb =
                    vram[(PPUCTRL & 0x10 ? 0x1000 : 0)
                    + bgNextTileID * 16
                    + ((v >> 12) & 7)];
                break;

            case 7:
                bgNextTileMsb =
                    vram[(PPUCTRL & 0x10 ? 0x1000 : 0)
                    + bgNextTileID * 16
                    + ((v >> 12) & 7) + 8];
                break;
            }

            // Cycle 256: increment Y
            if (cycle == 256)
                incrementFineY();

            // Cycle 257: horizontal copy
            if (cycle == 257)
                transferHorizontal();

            // Pre-render vertical copy
            if (scanline == 261 && cycle >= 280 && cycle < 305)
                transferVertical();
        }
    }

    // -------------------------
    // Sprite pattern fetch (257–320)
    // -------------------------
    if (scanline >= 0 && scanline < 240)
    {
        if (cycle == 257)
        {
            spriteCount = 0;
        }

        if (cycle >= 257 && cycle < 321)
        {
            int sprite = (cycle - 257) / 8;

            if (sprite < spriteCount)
            {
                uint8_t y = secondaryOAM[sprite * 4];
                uint8_t tile = secondaryOAM[sprite * 4 + 1];

                uint16_t patternAddr = (PPUCTRL & 0x08 ? 0x1000 : 0) + tile * 16 + (scanline - y);

                spritePatternLow[sprite] = vram[patternAddr];
                spritePatternHigh[sprite] = vram[patternAddr + 8];
                spriteXCounter[sprite] = secondaryOAM[sprite * 4 + 3];
            }
        }
    }

    // -------------------------
    // Advance timing
    // -------------------------
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
        renderingEnabled)
    {
        cycle = 1;
    }
}

void PPU2C02::updateShifters()
{
    bgShifterPatternLow <<= 1;
    bgShifterPatternHigh <<= 1;
    bgShifterAttribLow <<= 1;
    bgShifterAttribHigh <<= 1;

    for (int i = 0; i < 8; i++)
    {
        if (spriteXCounter[i] > 0)
            spriteXCounter[i]--;
        else
        {
            spritePatternLow[i] <<= 1;
            spritePatternHigh[i] <<= 1;
        }
    }
}

void PPU2C02::loadBackgroundShifters()
{
    bgShifterPatternLow = (bgShifterPatternLow & 0xFF00) | bgNextTileLsb;

    bgShifterPatternHigh = (bgShifterPatternHigh & 0xFF00) | bgNextTileMsb;

    uint8_t attrib = bgNextTileAttrib;

    bgShifterAttribLow = (bgShifterAttribLow & 0xFF00) | ((attrib & 1) ? 0xFF : 0x00);

    bgShifterAttribHigh = (bgShifterAttribHigh & 0xFF00) | ((attrib & 2) ? 0xFF : 0x00);
}

void PPU2C02::incrementCoarseX()
{
    if ((v & 0x001F) == 31)
    {
        v &= ~0x001F;
        v ^= 0x0400;
    }
    else
        v++;
}

void PPU2C02::incrementFineY()
{
    if ((v & 0x7000) != 0x7000)
        v += 0x1000;
    else
    {
        v &= ~0x7000;
        int y = (v & 0x03E0) >> 5;
        if (y == 29)
        {
            y = 0;
            v ^= 0x0800;
        }
        else if (y == 31)
            y = 0;
        else
            y++;
        v = (v & ~0x03E0) | (y << 5);
    }
}

void PPU2C02::transferHorizontal()
{
    v = (v & ~0x041F) | (t & 0x041F);
}

void PPU2C02::transferVertical()
{
    v = (v & ~0x7BE0) | (t & 0x7BE0);
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

    case 1: PPUMASK = data; break;
    case 3: OAMADDR = data; break;
    case 4: oam[OAMADDR++] = data; break;

    case 5:
        if (!w)
        {
            x = data & 7;
            t = (t & 0xFFE0) | (data >> 3);
            w = true;
        }
        else
        {
            t = (t & 0x8FFF) | ((data & 7) << 12);
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
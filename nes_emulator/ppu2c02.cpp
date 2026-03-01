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
    // ---------------------------
    // 1. VBlank clear (pre-render)
    // ---------------------------
    if (scanline == 261 && cycle == 1)
    {
        nmiOccurred = false;
        PPUSTATUS &= ~0x80;
    }

    // ---------------------------
    // 2. VBlank set
    // ---------------------------
    if (scanline == 241 && cycle == 1)
    {
        nmiOccurred = true;
        PPUSTATUS |= 0x80;
    }

    // ---------------------------
    // 3. NMI edge logic
    // ---------------------------
    updateNMI();

    // ---------------------------
    // 4. Advance timing
    // ---------------------------
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

    // ---------------------------
    // 5. Odd frame cycle skip
    // ---------------------------
    if (scanline == 0 &&
        cycle == 0 &&
        oddFrame &&
        (PPUMASK & 0x18)) // rendering enabled
    {
        cycle = 1; // skip cycle 0
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
    uint8_t data = 0;

    switch (addr)
    {
    case 2: // PPUSTATUS
    {
        data = (PPUSTATUS & 0xE0) | (bufferedData & 0x1F);

        nmiOccurred = false;
        PPUSTATUS &= ~0x80;

        w = false;

        break;
    }

    case 4:
        data = oam[OAMADDR];
        break;

    case 7:
    {
        data = bufferedData;
        bufferedData = vram[v & 0x07FF];

        v += (PPUCTRL & 0x04) ? 32 : 1;
        break;
    }
    }

    return data;
}

void PPU2C02::cpuWrite(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
    case 0: // PPUCTRL
    {
        bool oldNMI = nmiOutput;

        PPUCTRL = data;
        nmiOutput = PPUCTRL & 0x80;

        t = (t & 0xF3FF) | ((data & 0x03) << 10);

        // NMI race condition:
        if (!oldNMI && nmiOutput && nmiOccurred)
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
}

void PPU2C02::writeOAM(uint8_t addr, uint8_t data)
{
    oam[addr] = data;
}
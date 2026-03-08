#pragma once
#include <cstdint>

class NES;
class Bus;

class PPU2C02
{
public:
    PPU2C02();
    void connectNES(NES* n);

    void reset();
    void clock();

    uint8_t cpuRead(uint16_t addr);
    void    cpuWrite(uint16_t addr, uint8_t data);

    void writeOAM(uint8_t addr, uint8_t data);

public:
    int16_t scanline = 0;
    int16_t cycle = 0;
    uint64_t frame = 0;

private:
    NES* nes = nullptr;
    Bus* bus = nullptr;

    uint8_t PPUCTRL = 0;
    uint8_t PPUMASK = 0;
    uint8_t PPUSTATUS = 0;
    uint8_t OAMADDR = 0;

    uint8_t oam[256]{};
    uint8_t vram[2048]{};
    uint8_t palette[32]{};

    uint16_t v = 0;
    uint16_t t = 0;
    uint8_t  x = 0;
    bool     w = false;

    uint8_t bufferedData = 0;
    uint8_t ppuDataBus = 0;

    bool nmiOccurred = false;
    bool nmiOutput = false;
    bool nmiPrevious = false;

    bool oddFrame = false;

private:
    void updateNMI();
};

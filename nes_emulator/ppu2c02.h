#pragma once

#include <cstdint>

class NES;

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
    int cycle = 0;
    int scanline = 0;
    int frame = 0;

private:
    NES* nes = nullptr;

    // Registers
    uint8_t PPUCTRL = 0x00;
    uint8_t PPUMASK = 0x00;
    uint8_t PPUSTATUS = 0x00;
    uint8_t OAMADDR = 0x00;

    uint8_t oam[256];

    // VRAM
    uint8_t vram[2048];
    uint8_t palette[32];

    uint16_t v = 0;  // current VRAM address
    uint16_t t = 0;  // temp VRAM address
    uint8_t  x = 0;  // fine X scroll
    bool     w = false; // write toggle

    uint8_t bufferedData = 0x00;

    // NMI
    bool nmiOccurred = false;
    bool nmiOutput = false;
    bool nmiPrevious = false;

private:
    void updateNMI();
};
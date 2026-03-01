#pragma once

#include <cstdint>

class PPU2C02
{
public:
    PPU2C02();

    void reset();
    void clock();

public:
    int cycle = 0;
    int scanline = 0;
    int frameCounter = 0;

    bool vblankFlag = false;
};
#pragma once
#include <cstdint>


class Cartridge {
public:
    Cartridge();

    uint8_t cpuRead(uint16_t addr, bool readOnly = false);

    void    cpuWrite(uint16_t addr, uint8_t data);
};
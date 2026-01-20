#include <cstdint>


class PPU2C02 {
public:
    PPU2C02();

    uint8_t cpuRead(uint16_t addr, bool readOnly = false);

    void    cpuWrite(uint16_t addr, uint8_t data);
};
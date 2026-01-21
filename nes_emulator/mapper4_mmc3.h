#include <cstdint>


class Mapper4_MMC3 {
public:
    Mapper4_MMC3();

    uint8_t cpuRead(uint16_t addr, uint8_t data);

    void    cpuWrite(uint16_t addr, uint8_t data);
};
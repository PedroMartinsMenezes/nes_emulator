#include <cstdint>


class APU2A03 {
public:
    APU2A03();

    uint8_t cpuRead(uint16_t addr, uint8_t data);

    void    cpuWrite(uint16_t addr, uint8_t data);

    void    reset();
};
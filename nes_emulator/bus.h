#pragma once
#include <cstdint>
#include <array>

class ROM;

class Bus
{
public:
    uint8_t read (uint16_t addr);
    void    write(uint16_t addr, uint8_t val);

    void attach_rom(ROM* rom) { m_rom = rom; }

    // Direct RAM access for post-run inspection (e.g. nestest result codes)
    uint8_t ram_peek(uint16_t addr) const { return m_ram[addr & 0x07FF]; }

private:
    std::array<uint8_t, 2048> m_ram{};
    ROM* m_rom = nullptr;
};

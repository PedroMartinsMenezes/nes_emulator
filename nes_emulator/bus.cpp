#include "bus.h"
#include "rom.h"

uint8_t Bus::read(uint16_t addr)
{
    uint8_t val = m_open_bus;  // default: open bus

    if (addr < 0x2000)
        val = m_ram[addr & 0x07FF];        // 2 KB RAM mirrored

    else if (addr < 0x4000)
        val = m_open_bus;                   // PPU registers (not yet)

    else if (addr < 0x4020)
        val = m_open_bus;                   // APU / IO (not yet)

    else if (addr >= 0x8000 && m_rom)
        val = m_rom->prg_read(addr);        // PRG-ROM

    m_open_bus = val;
    return val;
}

void Bus::write(uint16_t addr, uint8_t val)
{
    m_open_bus = val;  // writes also drive the bus

    if (addr < 0x2000)
        m_ram[addr & 0x07FF] = val;        // 2 KB RAM mirrored

    // PPU / APU writes ignored for now
}

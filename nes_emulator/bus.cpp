#include "bus.h"
#include "rom.h"

uint8_t Bus::read(uint16_t addr)
{
    if (addr < 0x2000)
        return m_ram[addr & 0x07FF];           // 2 KB RAM mirrored

    if (addr < 0x4000)
        return 0;                               // PPU registers (not yet)

    if (addr < 0x4020)
        return 0;                               // APU / IO (not yet)

    if (addr >= 0x8000 && m_rom)
        return m_rom->prg_read(addr);           // PRG-ROM

    return 0;
}

void Bus::write(uint16_t addr, uint8_t val)
{
    if (addr < 0x2000)
        m_ram[addr & 0x07FF] = val;            // 2 KB RAM mirrored

    // PPU / APU writes ignored for now
}

#include "rom.h"
#include <fstream>
#include <cstring>
#include <iostream>

bool ROM::load(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) { std::cerr << "ROM: cannot open " << path << "\n"; return false; }

    // iNES header (16 bytes)
    uint8_t hdr[16];
    f.read(reinterpret_cast<char*>(hdr), 16);

    if (hdr[0] != 'N' || hdr[1] != 'E' || hdr[2] != 'S' || hdr[3] != 0x1A)
    {
        std::cerr << "ROM: invalid iNES header\n";
        return false;
    }

    m_prg_banks = hdr[4];  // 16 KB units
    int chr_banks = hdr[5]; // 8 KB units (unused for now)

    uint8_t flags6 = hdr[6];
    uint8_t flags7 = hdr[7];
    m_mapper = (flags7 & 0xF0) | (flags6 >> 4);

    bool has_trainer = (flags6 & 0x04) != 0;
    if (has_trainer) f.seekg(512, std::ios::cur);

    int prg_size = m_prg_banks * 16384;
    m_prg.resize(prg_size);
    f.read(reinterpret_cast<char*>(m_prg.data()), prg_size);

    std::cout << "ROM: " << m_prg_banks << " PRG bank(s), mapper " << m_mapper << "\n";
    return true;
}

// CPU address $8000-$FFFF mapped here.
// If 1 bank (16 KB): mirrored at $8000 and $C000.
// If 2 banks (32 KB): $8000-$BFFF = bank 0, $C000-$FFFF = bank 1.
uint8_t ROM::prg_read(uint16_t addr) const
{
    uint32_t offset = addr - 0x8000;
    if (m_prg_banks == 1) offset &= 0x3FFF;  // mirror
    return m_prg[offset % m_prg.size()];
}

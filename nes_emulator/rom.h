#pragma once
#include <cstdint>
#include <vector>
#include <string>

class ROM
{
public:
    bool     load(const std::string& path);
    uint8_t  prg_read(uint16_t addr) const;

    int prg_banks() const { return m_prg_banks; }
    int mapper()    const { return m_mapper; }

private:
    std::vector<uint8_t> m_prg;
    int m_prg_banks = 0;
    int m_mapper    = 0;
};

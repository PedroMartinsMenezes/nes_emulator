#pragma once
#include <cstdint>
#include <cstdio>

class Bus;

class CPU
{
public:
    // Registers
    uint8_t  A  = 0x00;
    uint8_t  X  = 0x00;
    uint8_t  Y  = 0x00;
    uint8_t  SP = 0xFD;
    uint8_t  P  = 0x24;  // NV-BDIZC  (bit5 always 1)
    uint16_t PC = 0x0000;

    uint64_t cycles = 0;

    FILE* log_file = nullptr;

    void attach_bus(Bus* bus) { m_bus = bus; }
    void reset();            // standard power-up / reset
    void set_pc(uint16_t pc) { PC = pc; }
    void step();             // execute one instruction

private:
    Bus* m_bus = nullptr;

    // Flag accessors
    bool get_N() const { return (P >> 7) & 1; }
    bool get_V() const { return (P >> 6) & 1; }
    bool get_D() const { return (P >> 3) & 1; }
    bool get_I() const { return (P >> 2) & 1; }
    bool get_Z() const { return (P >> 1) & 1; }
    bool get_C() const { return (P >> 0) & 1; }

    void set_N(bool v) { P = (P & ~0x80u) | (v ? 0x80u : 0u); }
    void set_V(bool v) { P = (P & ~0x40u) | (v ? 0x40u : 0u); }
    void set_D(bool v) { P = (P & ~0x08u) | (v ? 0x08u : 0u); }
    void set_I(bool v) { P = (P & ~0x04u) | (v ? 0x04u : 0u); }
    void set_Z(bool v) { P = (P & ~0x02u) | (v ? 0x02u : 0u); }
    void set_C(bool v) { P = (P & ~0x01u) | (v ? 0x01u : 0u); }
    void set_NZ(uint8_t v) { set_N(v & 0x80); set_Z(v == 0); }

    uint8_t  mem_read (uint16_t addr);
    void     mem_write(uint16_t addr, uint8_t val);
    uint16_t mem_read16(uint16_t addr);
    uint16_t mem_read16_zp(uint8_t addr);  // zero-page wrap

    void    push(uint8_t val);
    uint8_t pop();
    void    push16(uint16_t val);
    uint16_t pop16();

    void do_adc(uint8_t val);
    void do_sbc(uint8_t val);
    void do_cmp(uint8_t reg, uint8_t val);
    uint8_t do_asl(uint8_t val);
    uint8_t do_lsr(uint8_t val);
    uint8_t do_rol(uint8_t val);
    uint8_t do_ror(uint8_t val);

    void build_log_prefix(uint16_t pc, uint8_t op,
                          uint8_t op1, uint8_t op2,
                          const char* name, uint8_t mode,
                          uint16_t addr, uint8_t val,
                          uint16_t mid_addr, uint16_t eff_addr,
                          char* out) const;
};

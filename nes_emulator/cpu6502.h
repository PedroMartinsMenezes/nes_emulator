#pragma once

#include <cstdint>
#include <array>
#include <functional>

class Bus;
class NES;

class CPU6502
{
public:
    CPU6502();

    void connectBus(Bus* b);
    void setNES(NES* n);

    void reset();
    void powerOn();

    void ExecOp();                 // Executes ONE instruction (full microcycle)
    void clock();                  // Executes ONE CPU cycle

    bool instructionComplete() const;

    // Interrupt lines
    void requestNMI();
    void requestIRQ();

    // Logging support
    uint8_t  GetFlag(uint8_t f) const;
    void     SetFlag(uint8_t f, bool v);

public:
    uint8_t A = 0x00;
    uint8_t X = 0x00;
    uint8_t Y = 0x00;
    uint8_t SP = 0xFD;
    uint8_t P = 0x24;

    uint16_t PC = 0x0000;

    uint64_t totalCycles = 0;

private:
    enum FLAGS6502
    {
        C = (1 << 0),
        Z = (1 << 1),
        I = (1 << 2),
        D = (1 << 3),
        B = (1 << 4),
        U = (1 << 5),
        V = (1 << 6),
        N = (1 << 7),
    };

private:
    Bus* bus = nullptr;
    NES* nes = nullptr;

    // Internal execution state
    uint8_t  opcode = 0x00;
    uint8_t  fetched = 0x00;
    uint16_t addr_abs = 0x0000;
    uint16_t addr_rel = 0x0000;

    uint8_t  cycles = 0;

    bool     nmi_pending = false;
    bool     irq_pending = false;

private:
    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t data);

    void    push(uint8_t v);
    uint8_t pull();

    void    handleInterrupt(uint16_t vector);

    void    buildOpcodeTable();
};
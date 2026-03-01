#pragma once
#include <cstdint>

class CPU6502;

class APU2A03
{
public:
    void connectCPU(CPU6502* c);

    void reset();
    void clock();

    void writeFrameCounter(uint8_t data);

private:
    CPU6502* cpu = nullptr;

    uint64_t cycle = 0;
    bool irqInhibit = false;
    bool frameIRQ = false;

    bool dmcDMA = false;
};
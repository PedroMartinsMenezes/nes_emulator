#include "apu2a03.h"
#include "cpu6502.h"

void APU2A03::connectCPU(CPU6502* c)
{
    cpu = c;
}

void APU2A03::reset()
{
    cycle = 0;
    irqInhibit = false;
    frameIRQ = false;
    dmcDMA = false;
}

void APU2A03::clock()
{
    cycle++;

    if (!irqInhibit && cycle == 29830)
    {
        frameIRQ = true;
        cpu->requestIRQ();
    }

    if (cycle >= 29830)
        cycle = 0;

    if (dmcDMA)
    {
        cpu->totalCycles++;
        dmcDMA = false;
    }
}

void APU2A03::writeFrameCounter(uint8_t data)
{
    irqInhibit = data & 0x40;
    if (irqInhibit)
        frameIRQ = false;
}
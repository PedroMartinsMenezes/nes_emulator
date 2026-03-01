#include "cpu6502.h"
#include "bus.h"
#include "nes.h"

CPU6502::CPU6502()
{
    buildOpcodeTable();
}

void CPU6502::connectBus(Bus* b)
{
    bus = b;
}

void CPU6502::setNES(NES* n)
{
    nes = n;
}

void CPU6502::powerOn()
{
    A = X = Y = 0;
    SP = 0xFD;
    P = 0x34; // IRQ disabled, U set

    totalCycles = 7; // Power-up timing

    PC = (uint16_t)read(0xFFFC) | ((uint16_t)read(0xFFFD) << 8);
}

void CPU6502::reset()
{
    SP -= 3;
    SetFlag(I, true);

    PC = (uint16_t)read(0xFFFC) | ((uint16_t)read(0xFFFD) << 8);

    cycles = 7;
}

void CPU6502::ExecOp()
{
    do {
        clock();
    } while (!instructionComplete());
}

void CPU6502::clock()
{
    if (cycles == 0)
    {
        if (nmi_pending)
        {
            nmi_pending = false;
            handleInterrupt(0xFFFA);
            return;
        }

        if (irq_pending && !GetFlag(I))
        {
            irq_pending = false;
            handleInterrupt(0xFFFE);
            return;
        }

        opcode = read(PC++);
        cycles = 0; // will be set by opcode table later
    }

    cycles--;
    totalCycles++;

    if (nes)
        nes->clockCPU();
}

bool CPU6502::instructionComplete() const
{
    return cycles == 0;
}

void CPU6502::requestNMI()
{
    nmi_pending = true;
}

void CPU6502::requestIRQ()
{
    irq_pending = true;
}

uint8_t CPU6502::read(uint16_t addr)
{
    return bus->cpuRead(addr);
}

void CPU6502::write(uint16_t addr, uint8_t data)
{
    bus->cpuWrite(addr, data);
}

void CPU6502::push(uint8_t v)
{
    write(0x0100 + SP--, v);
}

uint8_t CPU6502::pull()
{
    SP++;
    return read(0x0100 + SP);
}

void CPU6502::handleInterrupt(uint16_t vector)
{
    write(0x0100 + SP--, (PC >> 8) & 0xFF);
    write(0x0100 + SP--, PC & 0xFF);

    SetFlag(B, false);
    SetFlag(U, true);
    SetFlag(I, true);

    write(0x0100 + SP--, P);

    uint16_t lo = read(vector);
    uint16_t hi = read(vector + 1);
    PC = (hi << 8) | lo;

    cycles = 7;
}

uint8_t CPU6502::GetFlag(uint8_t f) const
{
    return ((P & f) > 0) ? 1 : 0;
}

void CPU6502::SetFlag(uint8_t f, bool v)
{
    if (v)
        P |= f;
    else
        P &= ~f;
}

void CPU6502::buildOpcodeTable()
{
    // Phase 1 only builds structure.
    // Full 256 opcode table comes next phase.
}
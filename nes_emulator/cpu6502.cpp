#include "cpu6502.h"
#include "bus.h"
#include "nes.h"

CPU6502::CPU6502()
{
    buildOpcodeTable();
}

void CPU6502::connectBus(Bus* b) { bus = b; }
void CPU6502::setNES(NES* n) { nes = n; }

uint8_t CPU6502::read(uint16_t addr) { return bus->cpuRead(addr); }
void CPU6502::write(uint16_t addr, uint8_t data) { bus->cpuWrite(addr, data); }

uint8_t CPU6502::fetch()
{
    if (!(lookup[opcode].addrmode == &CPU6502::IMP))
        fetched = read(addr_abs);
    return fetched;
}

void CPU6502::powerOn()
{
    A = X = Y = 0;
    SP = 0xFD;
    P = 0x34;
    PC = read(0xFFFC) | (read(0xFFFD) << 8);
}

void CPU6502::reset()
{
    SP -= 3;
    SetFlag(I, true);
    PC = read(0xFFFC) | (read(0xFFFD) << 8);
    cycles = 7;
}

void CPU6502::ExecOp()
{
    do { clock(); } while (!instructionComplete());
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
        cycles = lookup[opcode].cycles;

        uint8_t additional1 = (this->*lookup[opcode].addrmode)();
        uint8_t additional2 = (this->*lookup[opcode].operate)();

        cycles += (additional1 & additional2);
    }

    cycles--;
    totalCycles++;

    if (nes)
        nes->clockCPU();
}

bool CPU6502::instructionComplete() const { return cycles == 0; }

void CPU6502::requestNMI() { nmi_pending = true; }
void CPU6502::requestIRQ() { irq_pending = true; }

uint8_t CPU6502::GetFlag(uint8_t f) const { return (P & f) ? 1 : 0; }
void CPU6502::SetFlag(uint8_t f, bool v)
{
    if (v) P |= f;
    else P &= ~f;
}

void CPU6502::push(uint8_t v) { write(0x0100 + SP--, v); }
uint8_t CPU6502::pull() { SP++; return read(0x0100 + SP); }

void CPU6502::handleInterrupt(uint16_t vector)
{
    push((PC >> 8) & 0xFF);
    push(PC & 0xFF);
    SetFlag(B, false);
    SetFlag(U, true);
    SetFlag(I, true);
    push(P);
    PC = read(vector) | (read(vector + 1) << 8);
    cycles = 7;
}

//=============================================================================
//Addressing Modes (Cycle-Accurate Core)
//=============================================================================

uint8_t CPU6502::IMP() { fetched = A; return 0; }

uint8_t CPU6502::IMM() { addr_abs = PC++; return 0; }

uint8_t CPU6502::ZP0() { addr_abs = read(PC++); return 0; }

uint8_t CPU6502::ZPX()
{
    addr_abs = (read(PC++) + X) & 0x00FF;
    return 0;
}

uint8_t CPU6502::ZPY()
{
    addr_abs = (read(PC++) + Y) & 0x00FF;
    return 0;
}

uint8_t CPU6502::ABS()
{
    uint16_t lo = read(PC++);
    uint16_t hi = read(PC++);
    addr_abs = (hi << 8) | lo;
    return 0;
}

uint8_t CPU6502::ABX()
{
    uint16_t lo = read(PC++);
    uint16_t hi = read(PC++);
    addr_abs = ((hi << 8) | lo) + X;
    return (addr_abs & 0xFF00) != (hi << 8);
}

uint8_t CPU6502::ABY()
{
    uint16_t lo = read(PC++);
    uint16_t hi = read(PC++);
    addr_abs = ((hi << 8) | lo) + Y;
    return (addr_abs & 0xFF00) != (hi << 8);
}

uint8_t CPU6502::REL()
{
    addr_rel = read(PC++);
    if (addr_rel & 0x80) addr_rel |= 0xFF00;
    return 0;
}

uint8_t CPU6502::IND()
{
    uint16_t ptr_lo = read(PC++);
    uint16_t ptr_hi = read(PC++);
    uint16_t ptr = (ptr_hi << 8) | ptr_lo;

    uint16_t lo = read(ptr);
    uint16_t hi = read((ptr & 0xFF00) | ((ptr + 1) & 0x00FF));

    addr_abs = (hi << 8) | lo;
    return 0;
}

uint8_t CPU6502::IZX()
{
    uint16_t t = read(PC++);
    uint16_t lo = read((uint8_t)(t + X));
    uint16_t hi = read((uint8_t)(t + X + 1));
    addr_abs = (hi << 8) | lo;
    return 0;
}

uint8_t CPU6502::IZY()
{
    uint16_t t = read(PC++);
    uint16_t lo = read(t & 0x00FF);
    uint16_t hi = read((t + 1) & 0x00FF);
    addr_abs = ((hi << 8) | lo) + Y;
    return (addr_abs & 0xFF00) != (hi << 8);
}

//=============================================================================
// Core Official Opcodes
//=============================================================================

uint8_t CPU6502::LDA()
{
    fetch();
    A = fetched;
    SetFlag(Z, A == 0);
    SetFlag(N, A & 0x80);
    return 1;
}

uint8_t CPU6502::STA()
{
    write(addr_abs, A);
    return 0;
}

uint8_t CPU6502::TAX()
{
    X = A;
    SetFlag(Z, X == 0);
    SetFlag(N, X & 0x80);
    return 0;
}

uint8_t CPU6502::INX()
{
    X++;
    SetFlag(Z, X == 0);
    SetFlag(N, X & 0x80);
    return 0;
}

uint8_t CPU6502::JMP()
{
    PC = addr_abs;
    return 0;
}

uint8_t CPU6502::JSR()
{
    PC--;
    push((PC >> 8) & 0xFF);
    push(PC & 0xFF);
    PC = addr_abs;
    return 0;
}

uint8_t CPU6502::RTS()
{
    uint16_t lo = pull();
    uint16_t hi = pull();
    PC = ((hi << 8) | lo) + 1;
    return 0;
}

uint8_t CPU6502::BRK()
{
    PC++;
    SetFlag(I, true);
    push((PC >> 8) & 0xFF);
    push(PC & 0xFF);
    SetFlag(B, true);
    push(P);
    PC = read(0xFFFE) | (read(0xFFFF) << 8);
    return 0;
}

uint8_t CPU6502::BNE()
{
    if (GetFlag(Z) == 0)
    {
        cycles++;
        uint16_t addr = PC + addr_rel;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            cycles++;
        PC = addr;
    }
    return 0;
}

uint8_t CPU6502::NOP() { return 0; }

// Opcode Table (Partial for Now)

void CPU6502::buildOpcodeTable()
{
    for (auto& i : lookup)
        i = { "NOP", &CPU6502::NOP, &CPU6502::IMP, 2 };

    lookup[0xA9] = { "LDA", &CPU6502::LDA, &CPU6502::IMM, 2 };
    lookup[0xA5] = { "LDA", &CPU6502::LDA, &CPU6502::ZP0, 3 };
    lookup[0xAD] = { "LDA", &CPU6502::LDA, &CPU6502::ABS, 4 };

    lookup[0x8D] = { "STA", &CPU6502::STA, &CPU6502::ABS, 4 };

    lookup[0xAA] = { "TAX", &CPU6502::TAX, &CPU6502::IMP, 2 };
    lookup[0xE8] = { "INX", &CPU6502::INX, &CPU6502::IMP, 2 };

    lookup[0x4C] = { "JMP", &CPU6502::JMP, &CPU6502::ABS, 3 };
    lookup[0x6C] = { "JMP", &CPU6502::JMP, &CPU6502::IND, 5 };

    lookup[0x20] = { "JSR", &CPU6502::JSR, &CPU6502::ABS, 6 };
    lookup[0x60] = { "RTS", &CPU6502::RTS, &CPU6502::IMP, 6 };

    lookup[0x00] = { "BRK", &CPU6502::BRK, &CPU6502::IMM, 7 };
    lookup[0xD0] = { "BNE", &CPU6502::BNE, &CPU6502::REL, 2 };
}

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

    lookup[0xA2] = { "LDX", &CPU6502::LDX, &CPU6502::IMM, 2 };
    lookup[0xA0] = { "LDY", &CPU6502::LDY, &CPU6502::IMM, 2 };
    lookup[0x86] = { "STX", &CPU6502::STX, &CPU6502::ZP0, 3 };
    lookup[0x84] = { "STY", &CPU6502::STY, &CPU6502::ZP0, 3 };

    lookup[0x69] = { "ADC", &CPU6502::ADC, &CPU6502::IMM, 2 };
    lookup[0xE9] = { "SBC", &CPU6502::SBC, &CPU6502::IMM, 2 };

    lookup[0x29] = { "AND", &CPU6502::AND, &CPU6502::IMM, 2 };
    lookup[0x09] = { "ORA", &CPU6502::ORA, &CPU6502::IMM, 2 };
    lookup[0x49] = { "EOR", &CPU6502::EOR, &CPU6502::IMM, 2 };

    lookup[0xC9] = { "CMP", &CPU6502::CMP, &CPU6502::IMM, 2 };
    lookup[0xE0] = { "CPX", &CPU6502::CPX, &CPU6502::IMM, 2 };
    lookup[0xC0] = { "CPY", &CPU6502::CPY, &CPU6502::IMM, 2 };

    lookup[0x18] = { "CLC", &CPU6502::CLC, &CPU6502::IMP, 2 };
    lookup[0x38] = { "SEC", &CPU6502::SEC, &CPU6502::IMP, 2 };
    lookup[0x58] = { "CLI", &CPU6502::CLI, &CPU6502::IMP, 2 };
    lookup[0x78] = { "SEI", &CPU6502::SEI, &CPU6502::IMP, 2 };
    lookup[0xB8] = { "CLV", &CPU6502::CLV, &CPU6502::IMP, 2 };
    lookup[0xD8] = { "CLD", &CPU6502::CLD, &CPU6502::IMP, 2 };
    lookup[0xF8] = { "SED", &CPU6502::SED, &CPU6502::IMP, 2 };

    // ASL
    lookup[0x0A] = { "ASL", &CPU6502::ASL, &CPU6502::IMP, 2 };
    lookup[0x06] = { "ASL", &CPU6502::ASL, &CPU6502::ZP0, 5 };
    lookup[0x16] = { "ASL", &CPU6502::ASL, &CPU6502::ZPX, 6 };
    lookup[0x0E] = { "ASL", &CPU6502::ASL, &CPU6502::ABS, 6 };
    lookup[0x1E] = { "ASL", &CPU6502::ASL, &CPU6502::ABX, 7 };

    // LSR
    lookup[0x4A] = { "LSR", &CPU6502::LSR, &CPU6502::IMP, 2 };
    lookup[0x46] = { "LSR", &CPU6502::LSR, &CPU6502::ZP0, 5 };
    lookup[0x56] = { "LSR", &CPU6502::LSR, &CPU6502::ZPX, 6 };
    lookup[0x4E] = { "LSR", &CPU6502::LSR, &CPU6502::ABS, 6 };
    lookup[0x5E] = { "LSR", &CPU6502::LSR, &CPU6502::ABX, 7 };

    // ROL
    lookup[0x2A] = { "ROL", &CPU6502::ROL, &CPU6502::IMP, 2 };
    lookup[0x26] = { "ROL", &CPU6502::ROL, &CPU6502::ZP0, 5 };
    lookup[0x36] = { "ROL", &CPU6502::ROL, &CPU6502::ZPX, 6 };
    lookup[0x2E] = { "ROL", &CPU6502::ROL, &CPU6502::ABS, 6 };
    lookup[0x3E] = { "ROL", &CPU6502::ROL, &CPU6502::ABX, 7 };

    // ROR
    lookup[0x6A] = { "ROR", &CPU6502::ROR, &CPU6502::IMP, 2 };
    lookup[0x66] = { "ROR", &CPU6502::ROR, &CPU6502::ZP0, 5 };
    lookup[0x76] = { "ROR", &CPU6502::ROR, &CPU6502::ZPX, 6 };
    lookup[0x6E] = { "ROR", &CPU6502::ROR, &CPU6502::ABS, 6 };
    lookup[0x7E] = { "ROR", &CPU6502::ROR, &CPU6502::ABX, 7 };

    // BIT
    lookup[0x24] = { "BIT", &CPU6502::BIT, &CPU6502::ZP0, 3 };
    lookup[0x2C] = { "BIT", &CPU6502::BIT, &CPU6502::ABS, 4 };

    // Branches
    lookup[0xF0] = { "BEQ", &CPU6502::BEQ, &CPU6502::REL, 2 };
    lookup[0x10] = { "BPL", &CPU6502::BPL, &CPU6502::REL, 2 };
    lookup[0x30] = { "BMI", &CPU6502::BMI, &CPU6502::REL, 2 };
    lookup[0x90] = { "BCC", &CPU6502::BCC, &CPU6502::REL, 2 };
    lookup[0xB0] = { "BCS", &CPU6502::BCS, &CPU6502::REL, 2 };
    lookup[0x50] = { "BVC", &CPU6502::BVC, &CPU6502::REL, 2 };
    lookup[0x70] = { "BVS", &CPU6502::BVS, &CPU6502::REL, 2 };

    // Illegal Opcodes
    lookup[0x03] = { "SLO", &CPU6502::SLO, &CPU6502::IZX, 8 };
    lookup[0x07] = { "SLO", &CPU6502::SLO, &CPU6502::ZP0, 5 };
    lookup[0x0F] = { "SLO", &CPU6502::SLO, &CPU6502::ABS, 6 };

    lookup[0x23] = { "RLA", &CPU6502::RLA, &CPU6502::IZX, 8 };
    lookup[0x27] = { "RLA", &CPU6502::RLA, &CPU6502::ZP0, 5 };

    lookup[0x47] = { "SRE", &CPU6502::SRE, &CPU6502::ZP0, 5 };
    lookup[0x63] = { "RRA", &CPU6502::RRA, &CPU6502::IZX, 8 };

    lookup[0xA7] = { "LAX", &CPU6502::LAX, &CPU6502::ZP0, 3 };
    lookup[0x87] = { "SAX", &CPU6502::SAX, &CPU6502::ZP0, 3 };

    lookup[0x0B] = { "ANC", &CPU6502::ANC, &CPU6502::IMM, 2 };
    lookup[0x4B] = { "ALR", &CPU6502::ALR, &CPU6502::IMM, 2 };
    lookup[0x6B] = { "ARR", &CPU6502::ARR, &CPU6502::IMM, 2 };

    lookup[0x02] = { "KIL", &CPU6502::KIL, &CPU6502::IMP, 2 };
}

// Arithmetic / Logic

uint8_t CPU6502::ADC()
{
    fetch();
    uint16_t temp = (uint16_t)A + (uint16_t)fetched + GetFlag(C);

    SetFlag(C, temp > 255);
    SetFlag(Z, (temp & 0x00FF) == 0);
    SetFlag(V, (~((uint16_t)A ^ (uint16_t)fetched) & ((uint16_t)A ^ temp)) & 0x0080);
    SetFlag(N, temp & 0x80);

    A = temp & 0x00FF;
    return 1;
}

uint8_t CPU6502::SBC()
{
    fetch();
    uint16_t value = ((uint16_t)fetched) ^ 0x00FF;
    uint16_t temp = (uint16_t)A + value + GetFlag(C);

    SetFlag(C, temp & 0xFF00);
    SetFlag(Z, (temp & 0x00FF) == 0);
    SetFlag(V, (temp ^ (uint16_t)A) & (temp ^ value) & 0x0080);
    SetFlag(N, temp & 0x80);

    A = temp & 0x00FF;
    return 1;
}

uint8_t CPU6502::AND()
{
    fetch();
    A &= fetched;
    SetFlag(Z, A == 0);
    SetFlag(N, A & 0x80);
    return 1;
}

uint8_t CPU6502::ORA()
{
    fetch();
    A |= fetched;
    SetFlag(Z, A == 0);
    SetFlag(N, A & 0x80);
    return 1;
}

uint8_t CPU6502::EOR()
{
    fetch();
    A ^= fetched;
    SetFlag(Z, A == 0);
    SetFlag(N, A & 0x80);
    return 1;
}

// Compare

uint8_t CPU6502::CMP()
{
    fetch();
    uint16_t temp = (uint16_t)A - (uint16_t)fetched;
    SetFlag(C, A >= fetched);
    SetFlag(Z, (temp & 0x00FF) == 0);
    SetFlag(N, temp & 0x0080);
    return 1;
}

uint8_t CPU6502::CPX()
{
    fetch();
    uint16_t temp = (uint16_t)X - (uint16_t)fetched;
    SetFlag(C, X >= fetched);
    SetFlag(Z, (temp & 0x00FF) == 0);
    SetFlag(N, temp & 0x0080);
    return 0;
}

uint8_t CPU6502::CPY()
{
    fetch();
    uint16_t temp = (uint16_t)Y - (uint16_t)fetched;
    SetFlag(C, Y >= fetched);
    SetFlag(Z, (temp & 0x00FF) == 0);
    SetFlag(N, temp & 0x0080);
    return 0;
}

// INC / DEC

uint8_t CPU6502::INC()
{
    fetch();
    uint8_t temp = fetched + 1;
    write(addr_abs, temp);
    SetFlag(Z, temp == 0);
    SetFlag(N, temp & 0x80);
    return 0;
}

uint8_t CPU6502::DEC()
{
    fetch();
    uint8_t temp = fetched - 1;
    write(addr_abs, temp);
    SetFlag(Z, temp == 0);
    SetFlag(N, temp & 0x80);
    return 0;
}

uint8_t CPU6502::INY()
{
    Y++;
    SetFlag(Z, Y == 0);
    SetFlag(N, Y & 0x80);
    return 0;
}

uint8_t CPU6502::DEX()
{
    X--;
    SetFlag(Z, X == 0);
    SetFlag(N, X & 0x80);
    return 0;
}

uint8_t CPU6502::DEY()
{
    Y--;
    SetFlag(Z, Y == 0);
    SetFlag(N, Y & 0x80);
    return 0;
}

// Loads / Stores

uint8_t CPU6502::LDX()
{
    fetch();
    X = fetched;
    SetFlag(Z, X == 0);
    SetFlag(N, X & 0x80);
    return 1;
}

uint8_t CPU6502::LDY()
{
    fetch();
    Y = fetched;
    SetFlag(Z, Y == 0);
    SetFlag(N, Y & 0x80);
    return 1;
}

uint8_t CPU6502::STX()
{
    write(addr_abs, X);
    return 0;
}

uint8_t CPU6502::STY()
{
    write(addr_abs, Y);
    return 0;
}

// Stack

uint8_t CPU6502::PHA() { push(A); return 0; }
uint8_t CPU6502::PLA()
{
    A = pull();
    SetFlag(Z, A == 0);
    SetFlag(N, A & 0x80);
    return 0;
}

uint8_t CPU6502::PHP()
{
    push(P | B | U);
    return 0;
}

uint8_t CPU6502::PLP()
{
    P = pull();
    SetFlag(U, true);
    return 0;
}

// Flag Operations

uint8_t CPU6502::CLC() { SetFlag(C, false); return 0; }
uint8_t CPU6502::SEC() { SetFlag(C, true);  return 0; }
uint8_t CPU6502::CLI() { SetFlag(I, false); return 0; }
uint8_t CPU6502::SEI() { SetFlag(I, true);  return 0; }
uint8_t CPU6502::CLV() { SetFlag(V, false); return 0; }
uint8_t CPU6502::CLD() { SetFlag(D, false); return 0; }
uint8_t CPU6502::SED() { SetFlag(D, true);  return 0; }


// Arithmetic Shifts and Rotations


uint8_t CPU6502::ASL()
{
    fetch();
    uint16_t temp = (uint16_t)fetched << 1;

    SetFlag(C, temp & 0xFF00);
    SetFlag(Z, (temp & 0x00FF) == 0);
    SetFlag(N, temp & 0x80);

    if (lookup[opcode].addrmode == &CPU6502::IMP)
        A = temp & 0x00FF;
    else
        write(addr_abs, temp & 0x00FF);

    return 0;
}

uint8_t CPU6502::LSR()
{
    fetch();
    SetFlag(C, fetched & 0x01);

    uint8_t temp = fetched >> 1;

    SetFlag(Z, temp == 0);
    SetFlag(N, false);

    if (lookup[opcode].addrmode == &CPU6502::IMP)
        A = temp;
    else
        write(addr_abs, temp);

    return 0;
}

uint8_t CPU6502::ROL()
{
    fetch();
    uint16_t temp = ((uint16_t)fetched << 1) | GetFlag(C);

    SetFlag(C, temp & 0xFF00);
    SetFlag(Z, (temp & 0x00FF) == 0);
    SetFlag(N, temp & 0x80);

    if (lookup[opcode].addrmode == &CPU6502::IMP)
        A = temp & 0x00FF;
    else
        write(addr_abs, temp & 0x00FF);

    return 0;
}

uint8_t CPU6502::ROR()
{
    fetch();
    uint16_t temp = ((uint16_t)GetFlag(C) << 7) | (fetched >> 1);

    SetFlag(C, fetched & 0x01);
    SetFlag(Z, (temp & 0x00FF) == 0);
    SetFlag(N, temp & 0x80);

    if (lookup[opcode].addrmode == &CPU6502::IMP)
        A = temp & 0x00FF;
    else
        write(addr_abs, temp & 0x00FF);

    return 0;
}

// BIT

uint8_t CPU6502::BIT()
{
    fetch();

    SetFlag(Z, (A & fetched) == 0);
    SetFlag(V, fetched & 0x40);
    SetFlag(N, fetched & 0x80);

    return 0;
}

// Branch Instructions

uint8_t CPU6502::BEQ()
{
    if (GetFlag(Z))
    {
        cycles++;
        uint16_t addr = PC + addr_rel;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            cycles++;
        PC = addr;
    }
    return 0;
}

uint8_t CPU6502::BPL()
{
    if (!GetFlag(N))
    {
        cycles++;
        uint16_t addr = PC + addr_rel;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            cycles++;
        PC = addr;
    }
    return 0;
}

uint8_t CPU6502::BMI()
{
    if (GetFlag(N))
    {
        cycles++;
        uint16_t addr = PC + addr_rel;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            cycles++;
        PC = addr;
    }
    return 0;
}

uint8_t CPU6502::BCC()
{
    if (!GetFlag(C))
    {
        cycles++;
        uint16_t addr = PC + addr_rel;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            cycles++;
        PC = addr;
    }
    return 0;
}

uint8_t CPU6502::BCS()
{
    if (GetFlag(C))
    {
        cycles++;
        uint16_t addr = PC + addr_rel;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            cycles++;
        PC = addr;
    }
    return 0;
}

uint8_t CPU6502::BVC()
{
    if (!GetFlag(V))
    {
        cycles++;
        uint16_t addr = PC + addr_rel;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            cycles++;
        PC = addr;
    }
    return 0;
}

uint8_t CPU6502::BVS()
{
    if (GetFlag(V))
    {
        cycles++;
        uint16_t addr = PC + addr_rel;
        if ((addr & 0xFF00) != (PC & 0xFF00))
            cycles++;
        PC = addr;
    }
    return 0;
}

#pragma region Illegal Opcodes

// SLO (ASL + ORA)
uint8_t CPU6502::SLO()
{
    ASL();
    ORA();
    return 0;
}

// RLA (ROL + AND)
uint8_t CPU6502::RLA()
{
    ROL();
    AND();
    return 0;
}

// SRE (LSR + EOR)
uint8_t CPU6502::SRE()
{
    LSR();
    EOR();
    return 0;
}

// RRA (ROR + ADC)
uint8_t CPU6502::RRA()
{
    ROR();
    ADC();
    return 0;
}

// DCP (DEC + CMP)
uint8_t CPU6502::DCP()
{
    DEC();
    CMP();
    return 0;
}

// ISC (INC + SBC)
uint8_t CPU6502::ISC()
{
    INC();
    SBC();
    return 0;
}

// LAX(LDA + LDX)
uint8_t CPU6502::LAX()
{
    fetch();
    A = X = fetched;
    SetFlag(Z, A == 0);
    SetFlag(N, A & 0x80);
    return 1;
}

// SAX (A & X store)
uint8_t CPU6502::SAX()
{
    write(addr_abs, A & X);
    return 0;
}

uint8_t CPU6502::ANC()
{
    fetch();
    A &= fetched;
    SetFlag(Z, A == 0);
    SetFlag(N, A & 0x80);
    SetFlag(C, A & 0x80);
    return 0;
}

uint8_t CPU6502::ALR()
{
    fetch();
    A &= fetched;
    SetFlag(C, A & 0x01);
    A >>= 1;
    SetFlag(Z, A == 0);
    SetFlag(N, false);
    return 0;
}

uint8_t CPU6502::ARR()
{
    fetch();
    A &= fetched;
    A = (A >> 1) | (GetFlag(C) << 7);

    SetFlag(Z, A == 0);
    SetFlag(N, A & 0x80);
    SetFlag(C, A & 0x40);
    SetFlag(V, ((A >> 6) ^ (A >> 5)) & 1);

    return 0;
}

uint8_t CPU6502::KIL()
{
    cycles = 0xFF;
    return 0;
}

#pragma endregion

std::string CPU6502::disassemble(uint16_t addr)
{
    uint8_t op = bus->cpuRead(addr);
    const auto& inst = lookup[op];

    char buffer[64];
    sprintf(buffer, "%04X  %02X  %s", addr, op, inst.name);
    return std::string(buffer);
}

void CPU6502::logState(std::ostream& os, uint16_t pc_before)
{
    os << disassemble(pc_before)
        << "  A:" << std::hex << (int)A
        << " X:" << (int)X
        << " Y:" << (int)Y
        << " P:" << (int)P
        << " SP:" << (int)SP
        << " CYC:" << totalCycles
        << "\n";
}
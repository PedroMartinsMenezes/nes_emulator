#include "cpu6502.h"
#include "bus.h"
#include "nes.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

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

void CPU6502::setLogger(std::ofstream* log)
{
    this->log = log;
}

uint8_t CPU6502::read(uint16_t addr)
{
    return bus->cpuRead(addr);
}

void CPU6502::write(uint16_t addr, uint8_t data)
{
    bus->cpuWrite(addr, data);
}

uint8_t CPU6502::fetch()
{
    if (!(lookup[opcode].addrmode == &CPU6502::IMP))
        fetched = read(addr_abs);
    return fetched;
}

void CPU6502::powerOn()
{
    A = X = Y = 0;
    SP        = 0xFD;
    P         = 0x34;
    PC        = read(0xFFFC) | (read(0xFFFD) << 8);
}

void CPU6502::reset()
{
    /*SP -= 3;
    SetFlag(I, true);
    PC = read(0xFFFC) | (read(0xFFFD) << 8);
    cycles = 7;*/
    A = 0;
    X = 0;
    Y = 0;

    SP = 0xFD; // MUST be 0xFD

    P = 0x24; // IRQ disabled + unused bit set

    uint16_t lo = read(0xFFFC);
    uint16_t hi = read(0xFFFD);
    PC          = (hi << 8) | lo;

    cycles = 7;
}

void CPU6502::ExecOp()
{
    do
    {
        clock();

    } while (!instructionComplete());
}

void CPU6502::clock()
{
    if (bus->dmaActive)
    {
        nes->clockCPU(); // PPU still runs
        bus->clockDMA();
        totalCycles++;
        return;
    }

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

        // LOG HERE: AFTER interrupt polling and BEFORE opcode fetch
        logState(PC);

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

uint8_t CPU6502::GetFlag(uint8_t f) const
{
    return (P & f) ? 1 : 0;
}
void CPU6502::SetFlag(uint8_t f, bool v)
{
    if (v)
        P |= f;
    else
        P &= ~f;
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
    push((PC >> 8) & 0xFF);
    push(PC & 0xFF);
    SetFlag(B, false);
    SetFlag(U, true);
    SetFlag(I, true);
    push(P);
    PC     = read(vector) | (read(vector + 1) << 8);
    cycles = 7;
}

//=============================================================================
//Addressing Modes (Cycle-Accurate Core)
//=============================================================================

uint8_t CPU6502::IMP()
{
    fetched = A;
    return 0;
}

uint8_t CPU6502::IMM()
{
    addr_abs = PC++;
    return 0;
}

uint8_t CPU6502::ZP0()
{
    addr_abs = read(PC++);
    return 0;
}

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
    addr_abs    = (hi << 8) | lo;
    return 0;
}

uint8_t CPU6502::ABX()
{
    uint16_t lo = read(PC++);
    uint16_t hi = read(PC++);
    addr_abs    = ((hi << 8) | lo) + X;
    return (addr_abs & 0xFF00) != (hi << 8);
}

uint8_t CPU6502::ABY()
{
    uint16_t lo = read(PC++);
    uint16_t hi = read(PC++);
    addr_abs    = ((hi << 8) | lo) + Y;
    return (addr_abs & 0xFF00) != (hi << 8);
}

uint8_t CPU6502::REL()
{
    addr_rel = read(PC++);
    if (addr_rel & 0x80)
        addr_rel |= 0xFF00;
    return 0;
}

uint8_t CPU6502::IND()
{
    uint16_t ptr_lo = read(PC++);
    uint16_t ptr_hi = read(PC++);
    uint16_t ptr    = (ptr_hi << 8) | ptr_lo;

    uint16_t lo = read(ptr);
    uint16_t hi = read((ptr & 0xFF00) | ((ptr + 1) & 0x00FF));

    addr_abs = (hi << 8) | lo;
    return 0;
}

uint8_t CPU6502::IZX()
{
    uint16_t t  = read(PC++);
    uint16_t lo = read((uint8_t)(t + X));
    uint16_t hi = read((uint8_t)(t + X + 1));
    addr_abs    = (hi << 8) | lo;
    return 0;
}

uint8_t CPU6502::IZY()
{
    uint16_t t  = read(PC++);
    uint16_t lo = read(t & 0x00FF);
    uint16_t hi = read((t + 1) & 0x00FF);
    addr_abs    = ((hi << 8) | lo) + Y;
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

//Transfer A to X
uint8_t CPU6502::TAX()
{
    X = A;
    SetFlag(Z, X == 0);
    SetFlag(N, X & 0x80);
    return 0;
}

//Transfer A to Y
uint8_t CPU6502::TAY()
{
    Y = A;
    SetFlag(Z, Y == 0);
    SetFlag(N, Y & 0x80);
    return 0;
}

//Transfer X to A
uint8_t CPU6502::TXA()
{
    A = X;
    SetFlag(Z, A == 0);
    SetFlag(N, A & 0x80);
    return 0;
}

//Transfer Y to A
uint8_t CPU6502::TYA()
{
    A = Y;
    SetFlag(Z, A == 0);
    SetFlag(N, A & 0x80);
    return 0;
}

//Transfer Stack Pointer to X
uint8_t CPU6502::TSX()
{
    X = SP;
    SetFlag(Z, X == 0);
    SetFlag(N, X & 0x80);
    return 0;
}

//Transfer X to Stack Pointer
uint8_t CPU6502::TXS()
{
    SP = X;
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
    PC          = ((hi << 8) | lo) + 1;
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

uint8_t CPU6502::NOP()
{
    return 0;
}

//Return from Interrupt
uint8_t CPU6502::RTI()
{
    P = pull();

    // NES / 6502 rules
    SetFlag(B, false);
    SetFlag(U, true);

    uint16_t lo = pull();
    uint16_t hi = pull();
    PC          = (hi << 8) | lo;

    return 0;
}

// Opcode Table (Partial for Now)

void CPU6502::buildOpcodeTable()
{
    for (auto& i : lookup)
        i = {"NOP", &CPU6502::NOP, &CPU6502::IMP, 2};

    // --- ADC ---
    lookup[0x69] = {"ADC", &CPU6502::ADC, &CPU6502::IMM, 2};
    lookup[0x65] = {"ADC", &CPU6502::ADC, &CPU6502::ZP0, 3};
    lookup[0x75] = {"ADC", &CPU6502::ADC, &CPU6502::ZPX, 4};
    lookup[0x6D] = {"ADC", &CPU6502::ADC, &CPU6502::ABS, 4};
    lookup[0x7D] = {"ADC", &CPU6502::ADC, &CPU6502::ABX, 4};
    lookup[0x79] = {"ADC", &CPU6502::ADC, &CPU6502::ABY, 4};
    lookup[0x61] = {"ADC", &CPU6502::ADC, &CPU6502::IZX, 6};
    lookup[0x71] = {"ADC", &CPU6502::ADC, &CPU6502::IZY, 5};

    // --- AND ---
    lookup[0x29] = {"AND", &CPU6502::AND, &CPU6502::IMM, 2};
    lookup[0x25] = {"AND", &CPU6502::AND, &CPU6502::ZP0, 3};
    lookup[0x35] = {"AND", &CPU6502::AND, &CPU6502::ZPX, 4};
    lookup[0x2D] = {"AND", &CPU6502::AND, &CPU6502::ABS, 4};
    lookup[0x3D] = {"AND", &CPU6502::AND, &CPU6502::ABX, 4};
    lookup[0x39] = {"AND", &CPU6502::AND, &CPU6502::ABY, 4};
    lookup[0x21] = {"AND", &CPU6502::AND, &CPU6502::IZX, 6};
    lookup[0x31] = {"AND", &CPU6502::AND, &CPU6502::IZY, 5};

    // --- ASL ---
    lookup[0x0A] = {"ASL", &CPU6502::ASL, &CPU6502::IMP, 2};
    lookup[0x06] = {"ASL", &CPU6502::ASL, &CPU6502::ZP0, 5};
    lookup[0x16] = {"ASL", &CPU6502::ASL, &CPU6502::ZPX, 6};
    lookup[0x0E] = {"ASL", &CPU6502::ASL, &CPU6502::ABS, 6};
    lookup[0x1E] = {"ASL", &CPU6502::ASL, &CPU6502::ABX, 7};

    // --- Branches ---
    lookup[0x90] = {"BCC", &CPU6502::BCC, &CPU6502::REL, 2};
    lookup[0xB0] = {"BCS", &CPU6502::BCS, &CPU6502::REL, 2};
    lookup[0xF0] = {"BEQ", &CPU6502::BEQ, &CPU6502::REL, 2};
    lookup[0x30] = {"BMI", &CPU6502::BMI, &CPU6502::REL, 2};
    lookup[0xD0] = {"BNE", &CPU6502::BNE, &CPU6502::REL, 2};
    lookup[0x10] = {"BPL", &CPU6502::BPL, &CPU6502::REL, 2};
    lookup[0x50] = {"BVC", &CPU6502::BVC, &CPU6502::REL, 2};
    lookup[0x70] = {"BVS", &CPU6502::BVS, &CPU6502::REL, 2};

    // --- BIT ---
    lookup[0x24] = {"BIT", &CPU6502::BIT, &CPU6502::ZP0, 3};
    lookup[0x2C] = {"BIT", &CPU6502::BIT, &CPU6502::ABS, 4};

    // --- BRK ---
    lookup[0x00] = {"BRK", &CPU6502::BRK, &CPU6502::IMP, 7};

    // --- Clear flags ---
    lookup[0x18] = {"CLC", &CPU6502::CLC, &CPU6502::IMP, 2};
    lookup[0xD8] = {"CLD", &CPU6502::CLD, &CPU6502::IMP, 2};
    lookup[0x58] = {"CLI", &CPU6502::CLI, &CPU6502::IMP, 2};
    lookup[0xB8] = {"CLV", &CPU6502::CLV, &CPU6502::IMP, 2};

    // --- CMP ---
    lookup[0xC9] = {"CMP", &CPU6502::CMP, &CPU6502::IMM, 2};
    lookup[0xC5] = {"CMP", &CPU6502::CMP, &CPU6502::ZP0, 3};
    lookup[0xD5] = {"CMP", &CPU6502::CMP, &CPU6502::ZPX, 4};
    lookup[0xCD] = {"CMP", &CPU6502::CMP, &CPU6502::ABS, 4};
    lookup[0xDD] = {"CMP", &CPU6502::CMP, &CPU6502::ABX, 4};
    lookup[0xD9] = {"CMP", &CPU6502::CMP, &CPU6502::ABY, 4};
    lookup[0xC1] = {"CMP", &CPU6502::CMP, &CPU6502::IZX, 6};
    lookup[0xD1] = {"CMP", &CPU6502::CMP, &CPU6502::IZY, 5};

    // --- CPX ---
    lookup[0xE0] = {"CPX", &CPU6502::CPX, &CPU6502::IMM, 2};
    lookup[0xE4] = {"CPX", &CPU6502::CPX, &CPU6502::ZP0, 3};
    lookup[0xEC] = {"CPX", &CPU6502::CPX, &CPU6502::ABS, 4};

    // --- CPY ---
    lookup[0xC0] = {"CPY", &CPU6502::CPY, &CPU6502::IMM, 2};
    lookup[0xC4] = {"CPY", &CPU6502::CPY, &CPU6502::ZP0, 3};
    lookup[0xCC] = {"CPY", &CPU6502::CPY, &CPU6502::ABS, 4};

    // --- DEC / DEX / DEY ---
    lookup[0xC6] = {"DEC", &CPU6502::DEC, &CPU6502::ZP0, 5};
    lookup[0xD6] = {"DEC", &CPU6502::DEC, &CPU6502::ZPX, 6};
    lookup[0xCE] = {"DEC", &CPU6502::DEC, &CPU6502::ABS, 6};
    lookup[0xDE] = {"DEC", &CPU6502::DEC, &CPU6502::ABX, 7};
    lookup[0xCA] = {"DEX", &CPU6502::DEX, &CPU6502::IMP, 2};
    lookup[0x88] = {"DEY", &CPU6502::DEY, &CPU6502::IMP, 2};

    // --- EOR ---
    lookup[0x49] = {"EOR", &CPU6502::EOR, &CPU6502::IMM, 2};
    lookup[0x45] = {"EOR", &CPU6502::EOR, &CPU6502::ZP0, 3};
    lookup[0x55] = {"EOR", &CPU6502::EOR, &CPU6502::ZPX, 4};
    lookup[0x4D] = {"EOR", &CPU6502::EOR, &CPU6502::ABS, 4};
    lookup[0x5D] = {"EOR", &CPU6502::EOR, &CPU6502::ABX, 4};
    lookup[0x59] = {"EOR", &CPU6502::EOR, &CPU6502::ABY, 4};
    lookup[0x41] = {"EOR", &CPU6502::EOR, &CPU6502::IZX, 6};
    lookup[0x51] = {"EOR", &CPU6502::EOR, &CPU6502::IZY, 5};

    // --- INC / INX / INY ---
    lookup[0xE6] = {"INC", &CPU6502::INC, &CPU6502::ZP0, 5};
    lookup[0xF6] = {"INC", &CPU6502::INC, &CPU6502::ZPX, 6};
    lookup[0xEE] = {"INC", &CPU6502::INC, &CPU6502::ABS, 6};
    lookup[0xFE] = {"INC", &CPU6502::INC, &CPU6502::ABX, 7};
    lookup[0xE8] = {"INX", &CPU6502::INX, &CPU6502::IMP, 2};
    lookup[0xC8] = {"INY", &CPU6502::INY, &CPU6502::IMP, 2};

    // --- JMP / JSR ---
    lookup[0x4C] = {"JMP", &CPU6502::JMP, &CPU6502::ABS, 3};
    lookup[0x6C] = {"JMP", &CPU6502::JMP, &CPU6502::IND, 5};
    lookup[0x20] = {"JSR", &CPU6502::JSR, &CPU6502::ABS, 6};

    // --- LDA ---
    lookup[0xA9] = {"LDA", &CPU6502::LDA, &CPU6502::IMM, 2};
    lookup[0xA5] = {"LDA", &CPU6502::LDA, &CPU6502::ZP0, 3};
    lookup[0xB5] = {"LDA", &CPU6502::LDA, &CPU6502::ZPX, 4};
    lookup[0xAD] = {"LDA", &CPU6502::LDA, &CPU6502::ABS, 4};
    lookup[0xBD] = {"LDA", &CPU6502::LDA, &CPU6502::ABX, 4};
    lookup[0xB9] = {"LDA", &CPU6502::LDA, &CPU6502::ABY, 4};
    lookup[0xA1] = {"LDA", &CPU6502::LDA, &CPU6502::IZX, 6};
    lookup[0xB1] = {"LDA", &CPU6502::LDA, &CPU6502::IZY, 5};

    // --- LDX ---
    lookup[0xA2] = {"LDX", &CPU6502::LDX, &CPU6502::IMM, 2};
    lookup[0xA6] = {"LDX", &CPU6502::LDX, &CPU6502::ZP0, 3};
    lookup[0xB6] = {"LDX", &CPU6502::LDX, &CPU6502::ZPY, 4};
    lookup[0xAE] = {"LDX", &CPU6502::LDX, &CPU6502::ABS, 4};
    lookup[0xBE] = {"LDX", &CPU6502::LDX, &CPU6502::ABY, 4};

    // --- LDY ---
    lookup[0xA0] = {"LDY", &CPU6502::LDY, &CPU6502::IMM, 2};
    lookup[0xA4] = {"LDY", &CPU6502::LDY, &CPU6502::ZP0, 3};
    lookup[0xB4] = {"LDY", &CPU6502::LDY, &CPU6502::ZPX, 4};
    lookup[0xAC] = {"LDY", &CPU6502::LDY, &CPU6502::ABS, 4};
    lookup[0xBC] = {"LDY", &CPU6502::LDY, &CPU6502::ABX, 4};

    // --- LSR ---
    lookup[0x4A] = {"LSR", &CPU6502::LSR, &CPU6502::IMP, 2};
    lookup[0x46] = {"LSR", &CPU6502::LSR, &CPU6502::ZP0, 5};
    lookup[0x56] = {"LSR", &CPU6502::LSR, &CPU6502::ZPX, 6};
    lookup[0x4E] = {"LSR", &CPU6502::LSR, &CPU6502::ABS, 6};
    lookup[0x5E] = {"LSR", &CPU6502::LSR, &CPU6502::ABX, 7};

    // --- NOP ---
    lookup[0xEA] = {"NOP", &CPU6502::NOP, &CPU6502::IMP, 2};

    // --- ORA ---
    lookup[0x09] = {"ORA", &CPU6502::ORA, &CPU6502::IMM, 2};
    lookup[0x05] = {"ORA", &CPU6502::ORA, &CPU6502::ZP0, 3};
    lookup[0x15] = {"ORA", &CPU6502::ORA, &CPU6502::ZPX, 4};
    lookup[0x0D] = {"ORA", &CPU6502::ORA, &CPU6502::ABS, 4};
    lookup[0x1D] = {"ORA", &CPU6502::ORA, &CPU6502::ABX, 4};
    lookup[0x19] = {"ORA", &CPU6502::ORA, &CPU6502::ABY, 4};
    lookup[0x01] = {"ORA", &CPU6502::ORA, &CPU6502::IZX, 6};
    lookup[0x11] = {"ORA", &CPU6502::ORA, &CPU6502::IZY, 5};

    // --- Stack ---
    lookup[0x48] = {"PHA", &CPU6502::PHA, &CPU6502::IMP, 3};
    lookup[0x68] = {"PLA", &CPU6502::PLA, &CPU6502::IMP, 4};
    lookup[0x08] = {"PHP", &CPU6502::PHP, &CPU6502::IMP, 3};
    lookup[0x28] = {"PLP", &CPU6502::PLP, &CPU6502::IMP, 4};

    // --- ROL / ROR ---
    lookup[0x2A] = {"ROL", &CPU6502::ROL, &CPU6502::IMP, 2};
    lookup[0x26] = {"ROL", &CPU6502::ROL, &CPU6502::ZP0, 5};
    lookup[0x36] = {"ROL", &CPU6502::ROL, &CPU6502::ZPX, 6};
    lookup[0x2E] = {"ROL", &CPU6502::ROL, &CPU6502::ABS, 6};
    lookup[0x3E] = {"ROL", &CPU6502::ROL, &CPU6502::ABX, 7};

    lookup[0x6A] = {"ROR", &CPU6502::ROR, &CPU6502::IMP, 2};
    lookup[0x66] = {"ROR", &CPU6502::ROR, &CPU6502::ZP0, 5};
    lookup[0x76] = {"ROR", &CPU6502::ROR, &CPU6502::ZPX, 6};
    lookup[0x6E] = {"ROR", &CPU6502::ROR, &CPU6502::ABS, 6};
    lookup[0x7E] = {"ROR", &CPU6502::ROR, &CPU6502::ABX, 7};

    // --- RTI / RTS ---
    lookup[0x40] = {"RTI", &CPU6502::RTI, &CPU6502::IMP, 6};
    lookup[0x60] = {"RTS", &CPU6502::RTS, &CPU6502::IMP, 6};

    // --- SBC ---
    lookup[0xE9] = {"SBC", &CPU6502::SBC, &CPU6502::IMM, 2};
    lookup[0xE5] = {"SBC", &CPU6502::SBC, &CPU6502::ZP0, 3};
    lookup[0xF5] = {"SBC", &CPU6502::SBC, &CPU6502::ZPX, 4};
    lookup[0xED] = {"SBC", &CPU6502::SBC, &CPU6502::ABS, 4};
    lookup[0xFD] = {"SBC", &CPU6502::SBC, &CPU6502::ABX, 4};
    lookup[0xF9] = {"SBC", &CPU6502::SBC, &CPU6502::ABY, 4};
    lookup[0xE1] = {"SBC", &CPU6502::SBC, &CPU6502::IZX, 6};
    lookup[0xF1] = {"SBC", &CPU6502::SBC, &CPU6502::IZY, 5};

    // --- Set flags ---
    lookup[0x38] = {"SEC", &CPU6502::SEC, &CPU6502::IMP, 2};
    lookup[0xF8] = {"SED", &CPU6502::SED, &CPU6502::IMP, 2};
    lookup[0x78] = {"SEI", &CPU6502::SEI, &CPU6502::IMP, 2};

    // --- STA ---
    lookup[0x85] = {"STA", &CPU6502::STA, &CPU6502::ZP0, 3};
    lookup[0x95] = {"STA", &CPU6502::STA, &CPU6502::ZPX, 4};
    lookup[0x8D] = {"STA", &CPU6502::STA, &CPU6502::ABS, 4};
    lookup[0x9D] = {"STA", &CPU6502::STA, &CPU6502::ABX, 5};
    lookup[0x99] = {"STA", &CPU6502::STA, &CPU6502::ABY, 5};
    lookup[0x81] = {"STA", &CPU6502::STA, &CPU6502::IZX, 6};
    lookup[0x91] = {"STA", &CPU6502::STA, &CPU6502::IZY, 6};

    // --- STX ---
    lookup[0x86] = {"STX", &CPU6502::STX, &CPU6502::ZP0, 3};
    lookup[0x96] = {"STX", &CPU6502::STX, &CPU6502::ZPY, 4};
    lookup[0x8E] = {"STX", &CPU6502::STX, &CPU6502::ABS, 4};

    // --- STY ---
    lookup[0x84] = {"STY", &CPU6502::STY, &CPU6502::ZP0, 3};
    lookup[0x94] = {"STY", &CPU6502::STY, &CPU6502::ZPX, 4};
    lookup[0x8C] = {"STY", &CPU6502::STY, &CPU6502::ABS, 4};

    // --- Transfers ---
    lookup[0xAA] = {"TAX", &CPU6502::TAX, &CPU6502::IMP, 2};
    lookup[0xA8] = {"TAY", &CPU6502::TAY, &CPU6502::IMP, 2};
    lookup[0xBA] = {"TSX", &CPU6502::TSX, &CPU6502::IMP, 2};
    lookup[0x8A] = {"TXA", &CPU6502::TXA, &CPU6502::IMP, 2};
    lookup[0x9A] = {"TXS", &CPU6502::TXS, &CPU6502::IMP, 2};
    lookup[0x98] = {"TYA", &CPU6502::TYA, &CPU6502::IMP, 2};
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
    uint16_t temp  = (uint16_t)A + value + GetFlag(C);

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

uint8_t CPU6502::PHA()
{
    push(A);
    return 0;
}
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
    SetFlag(B, false);
    SetFlag(U, true);
    return 0;
}

// Flag Operations

uint8_t CPU6502::CLC()
{
    SetFlag(C, false);
    return 0;
}
uint8_t CPU6502::SEC()
{
    SetFlag(C, true);
    return 0;
}
uint8_t CPU6502::CLI()
{
    SetFlag(I, false);
    return 0;
}
uint8_t CPU6502::SEI()
{
    SetFlag(I, true);
    return 0;
}
uint8_t CPU6502::CLV()
{
    SetFlag(V, false);
    return 0;
}
uint8_t CPU6502::CLD()
{
    SetFlag(D, false);
    return 0;
}
uint8_t CPU6502::SED()
{
    SetFlag(D, true);
    return 0;
}

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
    uint8_t     op   = bus->cpuRead(addr);
    const auto& inst = lookup[op];

    char buffer[64];
    sprintf(buffer, "%04X  %02X  %s", addr, op, inst.name);
    return std::string(buffer);
}

//void CPU6502::logState(std::ostream& os, uint16_t pc_before)
//{
//    std::stringstream msg_builder;
//
//    msg_builder << disassemble(pc_before)
//        << "  A:" << std::hex << (int)A
//        << " X:" << (int)X
//        << " Y:" << (int)Y
//        << " P:" << (int)P
//        << " SP:" << (int)SP
//        << " CYC:" << totalCycles;
//
//
//    os << msg_builder.str() << "\n";
//
//    static int count = 0;
//    if (count++ < 100)
//    {
//        std::cout << msg_builder.str() << "\n";
//    }
//}

void CPU6502::logState(uint16_t pc_before)
{
    if (log == nullptr)
        return;

    uint8_t op = bus->cpuRead(pc_before);
    uint8_t b1 = bus->cpuRead(pc_before + 1);
    uint8_t b2 = bus->cpuRead(pc_before + 2);

    std::stringstream line;
    line << std::uppercase << std::hex << std::setfill('0');

    // -------------------
    // PC
    // -------------------
    line << std::setw(4) << pc_before << "  ";

    // -------------------
    // Raw bytes
    // -------------------
    line << std::setw(2) << (int)op << " ";

    auto mode = lookup[op].addrmode;

    int instrSize = 1;

    if (mode == &CPU6502::IMM || mode == &CPU6502::ZP0 || mode == &CPU6502::ZPX || mode == &CPU6502::ZPY ||
        mode == &CPU6502::REL || mode == &CPU6502::IZX || mode == &CPU6502::IZY)
    {
        instrSize = 2;
    }
    else if (mode == &CPU6502::ABS || mode == &CPU6502::ABX || mode == &CPU6502::ABY || mode == &CPU6502::IND)
    {
        instrSize = 3;
    }

    if (instrSize >= 2)
        line << std::setw(2) << (int)b1 << " ";
    else
        line << "   ";

    if (instrSize == 3)
        line << std::setw(2) << (int)b2;
    else
        line << "  ";

    line << "  ";

    // -------------------
    // Mnemonic
    // -------------------
    line << lookup[op].name << " ";

    std::string operand = getOperand(mode, op, b1, b2, pc_before);

    line << operand;

    // -------------------
    // Padding to column 48
    // -------------------
    while (line.str().length() < 48)
        line << " ";

    // -------------------
    // Registers
    // -------------------

    line << "A:" << std::setw(2) << (int)A << " ";
    line << "X:" << std::setw(2) << (int)X << " ";
    line << "Y:" << std::setw(2) << (int)Y << " ";
    line << "P:" << std::setw(2) << (int)P << " ";
    line << "SP:" << std::setw(2) << (int)SP << " ";

    // -------------------
    // PPU
    // -------------------
    line << std::dec << std::setfill(' ');
    int16_t scanline = (bus->ppu->scanline == 261) ? 0 : bus->ppu->scanline;
    line << "PPU:" << std::setw(3) << scanline << "," << std::setw(3) << bus->ppu->cycle << " ";

    // -------------------
    // CPU cycles
    // -------------------
    line << "CYC:" << totalCycles;

    (*log) << line.str() << "\n";

    if (totalCycles > 10000)
    {
        log->close();
        log = nullptr;

        exit(0);
    }
}

std::string CPU6502::getOperand(uint8_t (CPU6502::*mode)(void), uint8_t op, uint8_t b1, uint8_t b2, uint16_t pc)
{
    char              buf[32]{};
    std::stringstream operand;
    operand << std::uppercase << std::hex << std::setfill('0');

    if (mode == &CPU6502::IMM)
    {
        operand << "#$" << std::setw(2) << (int)b1;
    }
    else if (mode == &CPU6502::ZP0)
    {
        operand << "$" << std::setw(2) << (int)b1;
    }
    else if (mode == &CPU6502::ZPX)
    {
        uint8_t base = b1;
        uint8_t ea   = (base + X) & 0xFF;
        uint8_t val  = bus->cpuRead(ea);
        snprintf(buf, sizeof(buf), "$%02X,X @ %02X = %02X", base, ea, val);
        return std::string(buf);
    }
    else if (mode == &CPU6502::ZPY)
    {
        uint8_t base = b1;
        uint8_t ea   = (base + Y) & 0xFF;
        uint8_t val  = bus->cpuRead(ea);
        snprintf(buf, sizeof(buf), "$%02X,Y @ %02X = %02X", base, ea, val);
        return std::string(buf);
    }
    else if (mode == &CPU6502::ABS)
    {
        operand << "$" << std::setw(4) << ((b2 << 8) | b1);
    }
    else if (mode == &CPU6502::ABX)
    {
        uint16_t base = (b2 << 8) | b1;
        uint16_t ea   = (base + X) & 0xFFFF;
        uint8_t  val  = bus->cpuRead(ea);
        snprintf(buf, sizeof(buf), "$%04X,X @ %04X = %02X", base, ea, val);
        return std::string(buf);
    }
    else if (mode == &CPU6502::ABY)
    {
        uint16_t base = (b2 << 8) | b1;
        uint16_t ea   = (base + Y) & 0xFFFF;
        uint8_t  val  = bus->cpuRead(ea);
        snprintf(buf, sizeof(buf), "$%04X,Y @ %04X = %02X", base, ea, val);
        return std::string(buf);
    }
    else if (mode == &CPU6502::REL)
    {
        int16_t target = pc + 2 + (int8_t)b1;
        operand << "$" << std::setw(4) << target;
    }
    else if (mode == &CPU6502::IND)
    {
        uint16_t ptr = (b2 << 8) | b1;
        uint8_t  lo  = bus->cpuRead(ptr);
        uint8_t  hi;
        // Emulate 6502 indirect JMP page-wrap bug
        if ((ptr & 0x00FF) == 0x00FF)
            hi = bus->cpuRead(ptr & 0xFF00);
        else
            hi = bus->cpuRead(ptr + 1);
        uint16_t target = (hi << 8) | lo;
        snprintf(buf, sizeof(buf), "($%04X) = %04X", ptr, target);
        operand << buf;
    }
    else if (mode == &CPU6502::IZX)
    {
        uint8_t  zp_addr = b1;
        uint8_t  zp_ptr  = (zp_addr + X) & 0xFF;
        uint8_t  lo      = bus->cpuRead(zp_ptr);
        uint8_t  hi      = bus->cpuRead((zp_ptr + 1) & 0xFF);
        uint16_t ea      = (hi << 8) | lo;
        uint8_t  val     = bus->cpuRead(ea);
        snprintf(buf, sizeof(buf), "($%02X,X) @ %02X = %04X = %02X", zp_addr, zp_ptr, ea, val);
        operand << buf;
    }
    else if (mode == &CPU6502::IZY)
    {
        uint8_t  zp_addr = b1;
        uint8_t  lo      = bus->cpuRead(zp_addr);
        uint8_t  hi      = bus->cpuRead((zp_addr + 1) & 0xFF);
        uint16_t base    = (hi << 8) | lo;
        uint16_t ea      = base + Y;
        uint8_t  val     = bus->cpuRead(ea);
        snprintf(buf, sizeof(buf), "($%02X),Y = %04X @ %04X = %02X", zp_addr, base, ea, val);
        operand << buf;
    }

    // Accumulator addressing (nestest formatting)
    if (lookup[op].addrmode == &CPU6502::IMP)
    {
        auto fn = lookup[op].operate;
        if (fn == &CPU6502::LSR || fn == &CPU6502::ASL || fn == &CPU6502::ROL || fn == &CPU6502::ROR)
            return "A";
    }

    if (isMemoryOpcode(op) && mode != &CPU6502::IMP && mode != &CPU6502::IMM && mode != &CPU6502::IZX &&
        mode != &CPU6502::IZY)
    {
        uint16_t ea    = computeEffectiveAddressForLog(op, b1, b2, pc);
        uint8_t  value = getEffectiveValueForLog(ea);
        char     tmp[8];
        snprintf(tmp, sizeof(tmp), " = %02X", value);
        operand << tmp;
    }

    return operand.str();
}

bool CPU6502::isMemoryOpcode(uint8_t op) const
{
    if (lookup[op].addrmode == &CPU6502::IMM)
        return false;

    auto fn = lookup[op].operate;

    return fn == &CPU6502::BIT || fn == &CPU6502::STA || fn == &CPU6502::STX || fn == &CPU6502::STY ||
           fn == &CPU6502::LDA || fn == &CPU6502::LDX || fn == &CPU6502::LDY || fn == &CPU6502::ORA ||
           fn == &CPU6502::AND || fn == &CPU6502::EOR || fn == &CPU6502::ADC || fn == &CPU6502::SBC ||
           fn == &CPU6502::CMP || fn == &CPU6502::CPX || fn == &CPU6502::CPY ||

           // Read-modify-write
           fn == &CPU6502::INC || fn == &CPU6502::DEC || fn == &CPU6502::ASL || fn == &CPU6502::LSR ||
           fn == &CPU6502::ROL || fn == &CPU6502::ROR ||

           // Illegal NOPs (DOP / TOP)
           lookup[op].name[0] == '*';
}

uint8_t CPU6502::getEffectiveValueForLog(uint16_t effectiveAddress)
{
    if (effectiveAddress >= 0x2000 && effectiveAddress <= 0x3FFF)
        return 0xFF;

    return bus->cpuRead(effectiveAddress);
}

uint16_t CPU6502::computeEffectiveAddressForLog(uint8_t op, uint8_t b1, uint8_t b2, uint16_t pc)
{
    auto mode = lookup[op].addrmode;

    if (mode == &CPU6502::ZP0)
        return b1;
    if (mode == &CPU6502::ZPX)
        return (b1 + X) & 0xFF;
    if (mode == &CPU6502::ZPY)
        return (b1 + Y) & 0xFF;
    if (mode == &CPU6502::ABS)
        return (b2 << 8) | b1;
    if (mode == &CPU6502::ABX)
        return ((b2 << 8) | b1) + X;
    if (mode == &CPU6502::ABY)
        return ((b2 << 8) | b1) + Y;

    if (mode == &CPU6502::IZX)
    {
        uint8_t t  = (b1 + X) & 0xFF;
        uint8_t lo = bus->cpuRead(t);
        uint8_t hi = bus->cpuRead((t + 1) & 0xFF);
        return (hi << 8) | lo;
    }

    if (mode == &CPU6502::IZY)
    {
        uint8_t lo = bus->cpuRead(b1);
        uint8_t hi = bus->cpuRead((b1 + 1) & 0xFF);
        return ((hi << 8) | lo) + Y;
    }

    return 0;
}

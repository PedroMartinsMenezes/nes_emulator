#include "cpu6502.h"
#include "bus.h"
#include <fstream>
#include <cstdio>
#include <sstream>


#pragma region Constructor

CPU6502::CPU6502() {
    lookup.fill({ "NOP", 1, &CPU6502::NOP, &CPU6502::IMP, 2 });

    // --- ADC ---
    lookup[0x69] = { "ADC",2,&CPU6502::ADC,&CPU6502::IMM,2 };
    lookup[0x65] = { "ADC",2,&CPU6502::ADC,&CPU6502::ZP0,3 };
    lookup[0x75] = { "ADC",2,&CPU6502::ADC,&CPU6502::ZPX,4 };
    lookup[0x6D] = { "ADC",3,&CPU6502::ADC,&CPU6502::ABS,4 };
    lookup[0x7D] = { "ADC",3,&CPU6502::ADC,&CPU6502::ABX,4 };
    lookup[0x79] = { "ADC",3,&CPU6502::ADC,&CPU6502::ABY,4 };
    lookup[0x61] = { "ADC",2,&CPU6502::ADC,&CPU6502::IZX,6 };
    lookup[0x71] = { "ADC",2,&CPU6502::ADC,&CPU6502::IZY,5 };

    // --- AND ---
    lookup[0x29] = { "AND",2,&CPU6502::AND,&CPU6502::IMM,2 };
    lookup[0x25] = { "AND",2,&CPU6502::AND,&CPU6502::ZP0,3 };
    lookup[0x35] = { "AND",2,&CPU6502::AND,&CPU6502::ZPX,4 };
    lookup[0x2D] = { "AND",3,&CPU6502::AND,&CPU6502::ABS,4 };
    lookup[0x3D] = { "AND",3,&CPU6502::AND,&CPU6502::ABX,4 };
    lookup[0x39] = { "AND",3,&CPU6502::AND,&CPU6502::ABY,4 };
    lookup[0x21] = { "AND",2,&CPU6502::AND,&CPU6502::IZX,6 };
    lookup[0x31] = { "AND",2,&CPU6502::AND,&CPU6502::IZY,5 };

    // --- ASL ---
    lookup[0x0A] = { "ASL",1,&CPU6502::ASL,&CPU6502::IMP,2 };
    lookup[0x06] = { "ASL",2,&CPU6502::ASL,&CPU6502::ZP0,5 };
    lookup[0x16] = { "ASL",2,&CPU6502::ASL,&CPU6502::ZPX,6 };
    lookup[0x0E] = { "ASL",3,&CPU6502::ASL,&CPU6502::ABS,6 };
    lookup[0x1E] = { "ASL",3,&CPU6502::ASL,&CPU6502::ABX,7 };

    // --- Branches ---
    lookup[0x90] = { "BCC",0,&CPU6502::BCC,&CPU6502::REL,2 };
    lookup[0xB0] = { "BCS",0,&CPU6502::BCS,&CPU6502::REL,2 };
    lookup[0xF0] = { "BEQ",0,&CPU6502::BEQ,&CPU6502::REL,2 };
    lookup[0x30] = { "BMI",0,&CPU6502::BMI,&CPU6502::REL,2 };
    lookup[0xD0] = { "BNE",0,&CPU6502::BNE,&CPU6502::REL,2 };
    lookup[0x10] = { "BPL",0,&CPU6502::BPL,&CPU6502::REL,2 };
    lookup[0x50] = { "BVC",0,&CPU6502::BVC,&CPU6502::REL,2 };
    lookup[0x70] = { "BVS",0,&CPU6502::BVS,&CPU6502::REL,2 };

    // --- BIT ---
    lookup[0x24] = { "BIT",2,&CPU6502::BIT,&CPU6502::ZP0,3 };
    lookup[0x2C] = { "BIT",3,&CPU6502::BIT,&CPU6502::ABS,4 };

    // --- BRK ---
    lookup[0x00] = { "BRK",1,&CPU6502::BRK,&CPU6502::IMP,7 };

    // --- Clear flags ---
    lookup[0x18] = { "CLC",1,&CPU6502::CLC,&CPU6502::IMP,2 };
    lookup[0xD8] = { "CLD",1,&CPU6502::CLD,&CPU6502::IMP,2 };
    lookup[0x58] = { "CLI",1,&CPU6502::CLI,&CPU6502::IMP,2 };
    lookup[0xB8] = { "CLV",1,&CPU6502::CLV,&CPU6502::IMP,2 };

    // --- CMP ---
    lookup[0xC9] = { "CMP",2,&CPU6502::CMP,&CPU6502::IMM,2 };
    lookup[0xC5] = { "CMP",2,&CPU6502::CMP,&CPU6502::ZP0,3 };
    lookup[0xD5] = { "CMP",2,&CPU6502::CMP,&CPU6502::ZPX,4 };
    lookup[0xCD] = { "CMP",3,&CPU6502::CMP,&CPU6502::ABS,4 };
    lookup[0xDD] = { "CMP",3,&CPU6502::CMP,&CPU6502::ABX,4 };
    lookup[0xD9] = { "CMP",3,&CPU6502::CMP,&CPU6502::ABY,4 };
    lookup[0xC1] = { "CMP",2,&CPU6502::CMP,&CPU6502::IZX,6 };
    lookup[0xD1] = { "CMP",2,&CPU6502::CMP,&CPU6502::IZY,5 };

    // --- CPX ---
    lookup[0xE0] = { "CPX",2,&CPU6502::CPX,&CPU6502::IMM,2 };
    lookup[0xE4] = { "CPX",2,&CPU6502::CPX,&CPU6502::ZP0,3 };
    lookup[0xEC] = { "CPX",3,&CPU6502::CPX,&CPU6502::ABS,4 };

    // --- CPY ---
    lookup[0xC0] = { "CPY",2,&CPU6502::CPY,&CPU6502::IMM,2 };
    lookup[0xC4] = { "CPY",2,&CPU6502::CPY,&CPU6502::ZP0,3 };
    lookup[0xCC] = { "CPY",3,&CPU6502::CPY,&CPU6502::ABS,4 };

    // --- DEC / DEX / DEY ---
    lookup[0xC6] = { "DEC",2,&CPU6502::DEC,&CPU6502::ZP0,5 };
    lookup[0xD6] = { "DEC",2,&CPU6502::DEC,&CPU6502::ZPX,6 };
    lookup[0xCE] = { "DEC",3,&CPU6502::DEC,&CPU6502::ABS,6 };
    lookup[0xDE] = { "DEC",3,&CPU6502::DEC,&CPU6502::ABX,7 };
    lookup[0xCA] = { "DEX",1,&CPU6502::DEX,&CPU6502::IMP,2 };
    lookup[0x88] = { "DEY",1,&CPU6502::DEY,&CPU6502::IMP,2 };

    // --- EOR ---
    lookup[0x49] = { "EOR",2,&CPU6502::EOR,&CPU6502::IMM,2 };
    lookup[0x45] = { "EOR",2,&CPU6502::EOR,&CPU6502::ZP0,3 };
    lookup[0x55] = { "EOR",2,&CPU6502::EOR,&CPU6502::ZPX,4 };
    lookup[0x4D] = { "EOR",3,&CPU6502::EOR,&CPU6502::ABS,4 };
    lookup[0x5D] = { "EOR",3,&CPU6502::EOR,&CPU6502::ABX,4 };
    lookup[0x59] = { "EOR",3,&CPU6502::EOR,&CPU6502::ABY,4 };
    lookup[0x41] = { "EOR",2,&CPU6502::EOR,&CPU6502::IZX,6 };
    lookup[0x51] = { "EOR",2,&CPU6502::EOR,&CPU6502::IZY,5 };

    // --- INC / INX / INY ---
    lookup[0xE6] = { "INC",2,&CPU6502::INC,&CPU6502::ZP0,5 };
    lookup[0xF6] = { "INC",2,&CPU6502::INC,&CPU6502::ZPX,6 };
    lookup[0xEE] = { "INC",3,&CPU6502::INC,&CPU6502::ABS,6 };
    lookup[0xFE] = { "INC",3,&CPU6502::INC,&CPU6502::ABX,7 };
    lookup[0xE8] = { "INX",1,&CPU6502::INX,&CPU6502::IMP,2 };
    lookup[0xC8] = { "INY",1,&CPU6502::INY,&CPU6502::IMP,2 };

    // --- JMP / JSR ---
    lookup[0x4C] = { "JMP",3,&CPU6502::JMP,&CPU6502::ABS,3 };
    lookup[0x6C] = { "JMP",3,&CPU6502::JMP,&CPU6502::IND,5 };
    lookup[0x20] = { "JSR",3,&CPU6502::JSR,&CPU6502::ABS,6 };

    // --- LDA ---
    lookup[0xA9] = { "LDA",2,&CPU6502::LDA,&CPU6502::IMM,2 };
    lookup[0xA5] = { "LDA",2,&CPU6502::LDA,&CPU6502::ZP0,3 };
    lookup[0xB5] = { "LDA",2,&CPU6502::LDA,&CPU6502::ZPX,4 };
    lookup[0xAD] = { "LDA",3,&CPU6502::LDA,&CPU6502::ABS,4 };
    lookup[0xBD] = { "LDA",3,&CPU6502::LDA,&CPU6502::ABX,4 };
    lookup[0xB9] = { "LDA",3,&CPU6502::LDA,&CPU6502::ABY,4 };
    lookup[0xA1] = { "LDA",2,&CPU6502::LDA,&CPU6502::IZX,6 };
    lookup[0xB1] = { "LDA",2,&CPU6502::LDA,&CPU6502::IZY,5 };

    // --- LDX ---
    lookup[0xA2] = { "LDX",2,&CPU6502::LDX,&CPU6502::IMM,2 };
    lookup[0xA6] = { "LDX",2,&CPU6502::LDX,&CPU6502::ZP0,3 };
    lookup[0xB6] = { "LDX",2,&CPU6502::LDX,&CPU6502::ZPY,4 };
    lookup[0xAE] = { "LDX",3,&CPU6502::LDX,&CPU6502::ABS,4 };
    lookup[0xBE] = { "LDX",3,&CPU6502::LDX,&CPU6502::ABY,4 };

    // --- LDY ---
    lookup[0xA0] = { "LDY",2,&CPU6502::LDY,&CPU6502::IMM,2 };
    lookup[0xA4] = { "LDY",2,&CPU6502::LDY,&CPU6502::ZP0,3 };
    lookup[0xB4] = { "LDY",2,&CPU6502::LDY,&CPU6502::ZPX,4 };
    lookup[0xAC] = { "LDY",3,&CPU6502::LDY,&CPU6502::ABS,4 };
    lookup[0xBC] = { "LDY",3,&CPU6502::LDY,&CPU6502::ABX,4 };

    // --- LSR ---
    lookup[0x4A] = { "LSR",1,&CPU6502::LSR,&CPU6502::IMP,2 };
    lookup[0x46] = { "LSR",2,&CPU6502::LSR,&CPU6502::ZP0,5 };
    lookup[0x56] = { "LSR",2,&CPU6502::LSR,&CPU6502::ZPX,6 };
    lookup[0x4E] = { "LSR",3,&CPU6502::LSR,&CPU6502::ABS,6 };
    lookup[0x5E] = { "LSR",3,&CPU6502::LSR,&CPU6502::ABX,7 };

    // --- NOP ---
    lookup[0xEA] = { "NOP",1,&CPU6502::NOP,&CPU6502::IMP,2 };

    // --- ORA ---
    lookup[0x09] = { "ORA",2,&CPU6502::ORA,&CPU6502::IMM,2 };
    lookup[0x05] = { "ORA",2,&CPU6502::ORA,&CPU6502::ZP0,3 };
    lookup[0x15] = { "ORA",2,&CPU6502::ORA,&CPU6502::ZPX,4 };
    lookup[0x0D] = { "ORA",3,&CPU6502::ORA,&CPU6502::ABS,4 };
    lookup[0x1D] = { "ORA",3,&CPU6502::ORA,&CPU6502::ABX,4 };
    lookup[0x19] = { "ORA",3,&CPU6502::ORA,&CPU6502::ABY,4 };
    lookup[0x01] = { "ORA",2,&CPU6502::ORA,&CPU6502::IZX,6 };
    lookup[0x11] = { "ORA",2,&CPU6502::ORA,&CPU6502::IZY,5 };

    // --- Stack ---
    lookup[0x48] = { "PHA",1,&CPU6502::PHA,&CPU6502::IMP,3 };
    lookup[0x68] = { "PLA",1,&CPU6502::PLA,&CPU6502::IMP,4 };
    lookup[0x08] = { "PHP",1,&CPU6502::PHP,&CPU6502::IMP,3 };
    lookup[0x28] = { "PLP",1,&CPU6502::PLP,&CPU6502::IMP,4 };

    // --- ROL / ROR ---
    lookup[0x2A] = { "ROL",1,&CPU6502::ROL,&CPU6502::IMP,2 };
    lookup[0x26] = { "ROL",2,&CPU6502::ROL,&CPU6502::ZP0,5 };
    lookup[0x36] = { "ROL",2,&CPU6502::ROL,&CPU6502::ZPX,6 };
    lookup[0x2E] = { "ROL",3,&CPU6502::ROL,&CPU6502::ABS,6 };
    lookup[0x3E] = { "ROL",3,&CPU6502::ROL,&CPU6502::ABX,7 };

    lookup[0x6A] = { "ROR",1,&CPU6502::ROR,&CPU6502::IMP,2 };
    lookup[0x66] = { "ROR",2,&CPU6502::ROR,&CPU6502::ZP0,5 };
    lookup[0x76] = { "ROR",2,&CPU6502::ROR,&CPU6502::ZPX,6 };
    lookup[0x6E] = { "ROR",3,&CPU6502::ROR,&CPU6502::ABS,6 };
    lookup[0x7E] = { "ROR",3,&CPU6502::ROR,&CPU6502::ABX,7 };

    // --- RTI / RTS ---
    lookup[0x40] = { "RTI",1,&CPU6502::RTI,&CPU6502::IMP,6 };
    lookup[0x60] = { "RTS",1,&CPU6502::RTS,&CPU6502::IMP,6 };

    // --- SBC ---
    lookup[0xE9] = { "SBC",2,&CPU6502::SBC,&CPU6502::IMM,2 };
    lookup[0xE5] = { "SBC",2,&CPU6502::SBC,&CPU6502::ZP0,3 };
    lookup[0xF5] = { "SBC",2,&CPU6502::SBC,&CPU6502::ZPX,4 };
    lookup[0xED] = { "SBC",3,&CPU6502::SBC,&CPU6502::ABS,4 };
    lookup[0xFD] = { "SBC",3,&CPU6502::SBC,&CPU6502::ABX,4 };
    lookup[0xF9] = { "SBC",3,&CPU6502::SBC,&CPU6502::ABY,4 };
    lookup[0xE1] = { "SBC",2,&CPU6502::SBC,&CPU6502::IZX,6 };
    lookup[0xF1] = { "SBC",2,&CPU6502::SBC,&CPU6502::IZY,5 };

    // --- Set flags ---
    lookup[0x38] = { "SEC",1,&CPU6502::SEC,&CPU6502::IMP,2 };
    lookup[0xF8] = { "SED",1,&CPU6502::SED,&CPU6502::IMP,2 };
    lookup[0x78] = { "SEI",1,&CPU6502::SEI,&CPU6502::IMP,2 };

    // --- STA ---
    lookup[0x85] = { "STA",2,&CPU6502::STA,&CPU6502::ZP0,3 };
    lookup[0x95] = { "STA",2,&CPU6502::STA,&CPU6502::ZPX,4 };
    lookup[0x8D] = { "STA",3,&CPU6502::STA,&CPU6502::ABS,4 };
    lookup[0x9D] = { "STA",3,&CPU6502::STA,&CPU6502::ABX,5 };
    lookup[0x99] = { "STA",3,&CPU6502::STA,&CPU6502::ABY,5 };
    lookup[0x81] = { "STA",2,&CPU6502::STA,&CPU6502::IZX,6 };
    lookup[0x91] = { "STA",2,&CPU6502::STA,&CPU6502::IZY,6 };

    // --- STX ---
    lookup[0x86] = { "STX",2,&CPU6502::STX,&CPU6502::ZP0,3 };
    lookup[0x96] = { "STX",2,&CPU6502::STX,&CPU6502::ZPY,4 };
    lookup[0x8E] = { "STX",3,&CPU6502::STX,&CPU6502::ABS,4 };

    // --- STY ---
    lookup[0x84] = { "STY",2,&CPU6502::STY,&CPU6502::ZP0,3 };
    lookup[0x94] = { "STY",2,&CPU6502::STY,&CPU6502::ZPX,4 };
    lookup[0x8C] = { "STY",3,&CPU6502::STY,&CPU6502::ABS,4 };

    // --- Transfers ---
    lookup[0xAA] = { "TAX",1,&CPU6502::TAX,&CPU6502::IMP,2 };
    lookup[0xA8] = { "TAY",1,&CPU6502::TAY,&CPU6502::IMP,2 };
    lookup[0xBA] = { "TSX",1,&CPU6502::TSX,&CPU6502::IMP,2 };
    lookup[0x8A] = { "TXA",1,&CPU6502::TXA,&CPU6502::IMP,2 };
    lookup[0x9A] = { "TXS",1,&CPU6502::TXS,&CPU6502::IMP,2 };
    lookup[0x98] = { "TYA",1,&CPU6502::TYA,&CPU6502::IMP,2 };

}

#pragma endregion

#pragma region Helpers

uint8_t CPU6502::getFlag(FLAGS f) const { 
    return (P & f) ? 1 : 0;
}

void CPU6502::setFlag(FLAGS f, bool v) {
    if (v) 
        P |= f;
    else   
        P &= ~f;
}

uint8_t CPU6502::read(uint16_t addr) {
    return bus->cpuRead(addr);
}

void CPU6502::write(uint16_t addr, uint8_t data) {
    bus->cpuWrite(addr, data);
}

uint8_t CPU6502::fetch() {
    if (!(lookup[opcode].addrmode == &CPU6502::IMP))
        fetched = read(addr_abs);
    return fetched;
}

void CPU6502::branch() {
    cycles++;
    uint16_t target = PC + addr_rel;
    if ((target & 0xFF00) != (PC & 0xFF00)) cycles++;
    PC = target;
}

#pragma endregion

#pragma region Core control

void CPU6502::reset() {
    A = X = Y = 0;
    SP = 0xFD;
    P = 0x24;

    addr_abs = 0xFFFC;

    if (nestestMode) {
        addr_abs = 0xC000;
    }

    uint16_t lo = read(addr_abs);
    uint16_t hi = read(addr_abs + 1);
    PC = (hi << 8) | lo;

    cycles = 8;
}

// Interrupt Request
void CPU6502::irq() {
    if (!getFlag(I)) {
        write(0x0100 + SP--, (PC >> 8) & 0xFF);
        write(0x0100 + SP--, PC & 0xFF);

        setFlag(B, false);
        setFlag(U, true);
        setFlag(I, true);
        write(0x0100 + SP--, P);

        addr_abs = 0xFFFE;
        uint16_t lo = read(addr_abs);
        uint16_t hi = read(addr_abs + 1);
        PC = (hi << 8) | lo;

        cycles = 7;
    }
}

// Non Maskable Interrupt
void CPU6502::nmi() {
    write(0x0100 + SP--, (PC >> 8) & 0xFF);
    write(0x0100 + SP--, PC & 0xFF);

    setFlag(B, false);
    setFlag(U, true);
    setFlag(I, true);
    write(0x0100 + SP--, P);

    addr_abs = 0xFFFA;
    uint16_t lo = read(addr_abs);
    uint16_t hi = read(addr_abs + 1);
    PC = (hi << 8) | lo;

    cycles = 8;
}

void CPU6502::clock() {
    if (cycles == 0) {
        opcode = read(PC++);
        auto& inst = lookup[opcode];
        cycles = inst.cycles;
        uint8_t a1 = (this->*inst.addrmode)();
        uint8_t a2 = (this->*inst.operate)();
        cycles += (a1 & a2);
    }
    cycles--;
    totalCycles++;
}

#pragma endregion

#pragma region Stack

void CPU6502::push(uint8_t v) {
    write(0x0100 + SP--, v);
}

uint8_t CPU6502::pull() {
    return read(0x0100 + ++SP);
}

uint8_t CPU6502::pullProcessorStatus() {
    //https://www.nesdev.org/wiki/Instruction_reference#PLP
    //The B flag and extra bit are ignored

    uint8_t currentB = P & B ? B : 0;
    uint8_t currentU = P & U ? U : 0;

    P = pull();

    P &= ~B;
    P |= currentB;

    P &= ~U;
    P |= currentU;

    return P;
}

#pragma endregion

#pragma region Addressing Modes - https://www.nesdev.org/obelisk-6502-guide/addressing.html

//Implicit
uint8_t CPU6502::IMP() { 
    fetched = A; 
    return 0; 
}

//Immediate
uint8_t CPU6502::IMM() { 
    addr_abs = PC++; 
    return 0; 
}

//Zero Page
uint8_t CPU6502::ZP0() {
    addr_abs = read(PC++);
    addr_abs &= 0x00FF;
    return 0;
}

//Zero Page + X
uint8_t CPU6502::ZPX() {
    addr_abs = (read(PC++) + X) & 0x00FF;
    return 0;
}

//Zero Page + Y
uint8_t CPU6502::ZPY() {
    addr_abs = (read(PC++) + Y) & 0x00FF;
    return 0;
}

//Absolute
uint8_t CPU6502::ABS() {
    uint16_t lo = read(PC++);
    uint16_t hi = read(PC++);
    addr_abs = (hi << 8) | lo;
    return 0;
}

//Absolute + X
uint8_t CPU6502::ABX() {
    uint16_t lo = read(PC++);
    uint16_t hi = read(PC++);
    uint16_t base = (hi << 8) | lo;
    addr_abs = base + X;
    // Page crossed?
    if ((addr_abs & 0xFF00) != (base & 0xFF00))
        return 1;
    return 0;
}

//Absolute + Y
uint8_t CPU6502::ABY() {
    uint16_t lo = read(PC++);
    uint16_t hi = read(PC++);
    uint16_t base = (hi << 8) | lo;
    addr_abs = base + Y;
    if ((addr_abs & 0xFF00) != (base & 0xFF00))
        return 1;
    return 0;
}

//Indirect
uint8_t CPU6502::IND() {
    uint16_t ptr_lo = read(PC++);
    uint16_t ptr_hi = read(PC++);
    uint16_t ptr = (ptr_hi << 8) | ptr_lo;
    uint16_t lo = read(ptr);
    // Hardware bug emulation
    uint16_t hi;
    if ((ptr & 0x00FF) == 0x00FF)
        hi = read(ptr & 0xFF00);
    else
        hi = read(ptr + 1);
    addr_abs = (hi << 8) | lo;
    return 0;
}

//Indexed Indirect + X
uint8_t CPU6502::IZX() {
    uint16_t t = (read(PC++) + X) & 0x00FF;
    uint16_t lo = read(t & 0x00FF);
    uint16_t hi = read((t + 1) & 0x00FF);
    addr_abs = (hi << 8) | lo;
    return 0;
}

//Indirect Indexed + Y
uint8_t CPU6502::IZY() {
    uint16_t t = read(PC++);
    uint16_t lo = read(t & 0x00FF);
    uint16_t hi = read((t + 1) & 0x00FF);
    uint16_t base = (hi << 8) | lo;
    addr_abs = base + Y;
    if ((addr_abs & 0xFF00) != (base & 0xFF00))
        return 1;
    return 0;
}

//Relative
uint8_t CPU6502::REL() {
    addr_rel = read(PC++);
    if (addr_rel & 0x80) addr_rel |= 0xFF00;
    return 0;
}

#pragma endregion

#pragma region Instruction Set - https://www.nesdev.org/wiki/Instruction_reference

#pragma region Load and Store

//Load Accumulator
uint8_t CPU6502::LDA() { fetch(); A = fetched; setFlag(Z, A == 0); setFlag(N, A & 0x80); return 1; }

//Load X Register
uint8_t CPU6502::LDX() { fetch(); X = fetched; setFlag(Z, X == 0); setFlag(N, X & 0x80); return 1; }

//Load Y Register
uint8_t CPU6502::LDY() { fetch(); Y = fetched; setFlag(Z, Y == 0); setFlag(N, Y & 0x80); return 1; }

//Store Accumulator
uint8_t CPU6502::STA() { write(addr_abs, A); return 0; }

//Store X Register
uint8_t CPU6502::STX() { write(addr_abs, X); return 0; }

//Store Y Register
uint8_t CPU6502::STY() { write(addr_abs, Y); return 0; }

#pragma endregion

#pragma region Register Transfers

//Transfer A to X
uint8_t CPU6502::TAX() { X = A; setFlag(Z, X == 0); setFlag(N, X & 0x80); return 0; }

//Transfer A to Y
uint8_t CPU6502::TAY() { Y = A; setFlag(Z, Y == 0); setFlag(N, Y & 0x80); return 0; }

//Transfer X to A
uint8_t CPU6502::TXA() { A = X; setFlag(Z, A == 0); setFlag(N, A & 0x80); return 0; }

//Transfer Y to A
uint8_t CPU6502::TYA() { A = Y; setFlag(Z, A == 0); setFlag(N, A & 0x80); return 0; }

#pragma endregion

#pragma region Stack Operations

//Transfer Stack Pointer to X
uint8_t CPU6502::TSX() { X = SP; setFlag(Z, X == 0); setFlag(N, X & 0x80); return 0; }

//Transfer X to Stack Pointer
uint8_t CPU6502::TXS() { SP = X; return 0; }

//Push Accumulator
uint8_t CPU6502::PHA() { push(A); return 0; }

//Push Processor Status
uint8_t CPU6502::PHP() { push(P | B | U); return 0; }

//Pull Accumulator
uint8_t CPU6502::PLA() { A = pull(); setFlag(Z, A == 0); setFlag(N, A & 0x80); return 0; }

//Pull Processor Status
uint8_t CPU6502::PLP() { P = pullProcessorStatus(); return 0; }

#pragma endregion

#pragma region Logical Operations

//Bitwise AND with Accumulator
uint8_t CPU6502::AND() { fetch(); A &= fetched; setFlag(Z, A == 0); setFlag(N, A & 0x80); return 1; }

//Bitwise Exclusive OR with Accumulator
uint8_t CPU6502::EOR() { fetch(); A ^= fetched; setFlag(Z, A == 0); setFlag(N, A & 0x80); return 1; }

//Bitwise OR with Accumulator
uint8_t CPU6502::ORA() { fetch(); A |= fetched; setFlag(Z, A == 0); setFlag(N, A & 0x80); return 1; }

//Test Bits
uint8_t CPU6502::BIT() { fetch(); setFlag(Z, (A & fetched) == 0); setFlag(V, fetched & 0x40); setFlag(N, fetched & 0x80); return 0; }

#pragma endregion

#pragma region Arithmetic Operations

//Add with Carry
uint8_t CPU6502::ADC() {
    fetch();
    uint16_t sum = (uint16_t)A + (uint16_t)fetched + getFlag(C);
    setFlag(C, sum > 0xFF);
    setFlag(Z, (sum & 0xFF) == 0);
    setFlag(V, (~(A ^ fetched) & (A ^ sum)) & 0x80);
    setFlag(N, sum & 0x80);
    A = sum & 0xFF;
    return 1;
}

//Subtract with Carry
uint8_t CPU6502::SBC() {
    fetch();
    uint16_t value = fetched ^ 0x00FF;
    uint16_t sum = (uint16_t)A + value + getFlag(C);
    setFlag(C, sum & 0xFF00);
    setFlag(Z, (sum & 0xFF) == 0);
    setFlag(V, (sum ^ A) & (sum ^ value) & 0x80);
    setFlag(N, sum & 0x80);
    A = sum & 0xFF;
    return 1;
}

#pragma endregion

#pragma region Comparison Operations

//Compare Accumulator
uint8_t CPU6502::CMP() { fetch(); uint16_t t = A - fetched; setFlag(C, A >= fetched); setFlag(Z, (t & 0xFF) == 0); setFlag(N, t & 0x80); return 1; }

//Compare X Register
uint8_t CPU6502::CPX() { fetch(); uint16_t t = X - fetched; setFlag(C, X >= fetched); setFlag(Z, (t & 0xFF) == 0); setFlag(N, t & 0x80); return 0; }

//Compare Y Register
uint8_t CPU6502::CPY() { fetch(); uint16_t t = Y - fetched; setFlag(C, Y >= fetched); setFlag(Z, (t & 0xFF) == 0); setFlag(N, t & 0x80); return 0; }

#pragma endregion

#pragma region Increments and Decrements

//Increment Memory
uint8_t CPU6502::INC() { fetch(); uint8_t v = fetched + 1; write(addr_abs, v); setFlag(Z, v == 0); setFlag(N, v & 0x80); return 0; }

//Increment X Register
uint8_t CPU6502::INX() { X++; setFlag(Z, X == 0); setFlag(N, X & 0x80); return 0; }

//Increment Y Register
uint8_t CPU6502::INY() { Y++; setFlag(Z, Y == 0); setFlag(N, Y & 0x80); return 0; }

//Decrement Memory
uint8_t CPU6502::DEC() { fetch(); uint8_t v = fetched - 1; write(addr_abs, v); setFlag(Z, v == 0); setFlag(N, v & 0x80); return 0; }

//Decrement X Register
uint8_t CPU6502::DEX() { X--; setFlag(Z, X == 0); setFlag(N, X & 0x80); return 0; }

//Decrement Y Register
uint8_t CPU6502::DEY() { Y--; setFlag(Z, Y == 0); setFlag(N, Y & 0x80); return 0; }

#pragma endregion

#pragma region Shifts and Rotates

//Arithmetic Shift Left
uint8_t CPU6502::ASL() {
    fetch();
    uint16_t r = fetched << 1;
    setFlag(C, r & 0xFF00);
    setFlag(Z, (r & 0xFF) == 0);
    setFlag(N, r & 0x80);
    if (lookup[opcode].addrmode == &CPU6502::IMP) A = r & 0xFF;
    else write(addr_abs, r & 0xFF);
    return 0;
}

//Logical Shift Right
uint8_t CPU6502::LSR() {
    fetch();
    setFlag(C, fetched & 0x01);
    uint8_t r = fetched >> 1;
    setFlag(Z, r == 0);
    setFlag(N, false);
    if (lookup[opcode].addrmode == &CPU6502::IMP) A = r;
    else write(addr_abs, r);
    return 0;
}

//Rotate Left
uint8_t CPU6502::ROL() {
    fetch();
    uint16_t r = (fetched << 1) | getFlag(C);
    setFlag(C, r & 0xFF00);
    setFlag(Z, (r & 0xFF) == 0);
    setFlag(N, r & 0x80);
    if (lookup[opcode].addrmode == &CPU6502::IMP) A = r & 0xFF;
    else write(addr_abs, r & 0xFF);
    return 0;
}

//Rotate Right
uint8_t CPU6502::ROR() {
    fetch();
    uint16_t r = (getFlag(C) << 7) | (fetched >> 1);
    setFlag(C, fetched & 0x01);
    setFlag(Z, (r & 0xFF) == 0);
    setFlag(N, r & 0x80);
    if (lookup[opcode].addrmode == &CPU6502::IMP) A = r & 0xFF;
    else write(addr_abs, r & 0xFF);
    return 0;
}

#pragma endregion

#pragma region Jumps and Calls

//Jump to Address
uint8_t CPU6502::JMP() {
    PC = addr_abs;
    return 0;
}

//Jump to Subroutine
uint8_t CPU6502::JSR() {
    PC--;
    push((PC >> 8) & 0xFF);
    push(PC & 0xFF);
    PC = addr_abs;
    return 0;
}

//Return from Subroutine
uint8_t CPU6502::RTS() {
    uint16_t lo = pull();
    uint16_t hi = pull();
    PC = ((hi << 8) | lo) + 1;
    return 0;
}

#pragma endregion

#pragma region Branches

//Branch on Carry Clear
uint8_t CPU6502::BCC() { if (!getFlag(C)) branch(); return 0; }

//Branch on Carry Set
uint8_t CPU6502::BCS() { if (getFlag(C)) branch(); return 0; }

//Branch on Equal
uint8_t CPU6502::BEQ() { if (getFlag(Z)) branch(); return 0; }

//Branch on Minus
uint8_t CPU6502::BMI() { if (getFlag(N)) branch(); return 0; }

//Branch on Not Equal
uint8_t CPU6502::BNE() { if (!getFlag(Z)) branch(); return 0; }

//Branch on Plus
uint8_t CPU6502::BPL() { if (!getFlag(N)) branch(); return 0; }

//Branch on Overflow Clear
uint8_t CPU6502::BVC() { if (!getFlag(V)) branch(); return 0; }

//Branch on Overflow Set
uint8_t CPU6502::BVS() { if (getFlag(V)) branch(); return 0; }

#pragma endregion

#pragma region Status Flag Changes

//Clear Carry
uint8_t CPU6502::CLC() { setFlag(C, false); return 0; }

//Set Carry
uint8_t CPU6502::SEC() { setFlag(C, true); return 0; }

//Clear Interrupt
uint8_t CPU6502::CLI() { setFlag(I, false); return 0; }

//Set Interrupt
uint8_t CPU6502::SEI() { setFlag(I, true); return 0; }

//Clear Decimal
uint8_t CPU6502::CLD() { setFlag(D, false); return 0; }

//Set Decimal
uint8_t CPU6502::SED() { setFlag(D, true); return 0; }

//Clear Overflow
uint8_t CPU6502::CLV() { setFlag(V, false); return 0; }

#pragma endregion

#pragma region System Functions

//Break
uint8_t CPU6502::BRK() {
    PC++;
    push((PC >> 8) & 0xFF); push(PC & 0xFF);
    push(P | B | U);
    setFlag(I, true);
    PC = read(0xFFFE) | (read(0xFFFF) << 8);
    return 0;
}

//No Operation
uint8_t CPU6502::NOP() {
    return 0;
}

//Return from Interrupt
uint8_t CPU6502::RTI() {
    P = pull();
    uint16_t lo = pull();
    uint16_t hi = pull();
    PC = (hi << 8) | lo;
    return 0;
}

#pragma endregion

std::string CPU6502::flagsToString() const {
    return std::string{
        getFlag(N) ? 'N' : '.',
        getFlag(V) ? 'V' : '.',
        '-',
        getFlag(B) ? 'B' : '.',
        getFlag(D) ? 'D' : '.',
        getFlag(I) ? 'I' : '.',
        getFlag(Z) ? 'Z' : '.',
        getFlag(C) ? 'C' : '.'
    };
}

uint8_t CPU6502::peek(uint16_t addr) {
    return bus->cpuRead(addr, true);
}

void CPU6502::logState(std::ofstream& log) {
    uint16_t pc = PC;

    uint8_t op = peek(pc);
    
    char byte1[3] = "  ";
    if (lookup[op].bytes > 1 || lookup[op].addrmode == &CPU6502::REL)
        std::sprintf(byte1, "%X", peek(pc + 1));

    char byte2[3] = "  ";
    if (lookup[op].bytes > 2)
        std::sprintf(byte2, "%X", peek(pc + 2));

    std::string operand = formatOperand(pc);

    // nestest logs cycles at instruction START
    uint64_t cyc = totalCycles - 1;

    int ppuX = (int)(cyc * 3) % 341;
    int ppuY = (int)(cyc * 3) / 341;

    char buffer[160];

    snprintf(
        buffer, sizeof(buffer),
        "%04X  %02X %02s %02s  %-3s %-27s A:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3d,%3d CYC:%llu",
        pc,
        op, 
        byte1,
        byte2,
        lookup[op].name,
        operand.c_str(),
        A, X, Y, P, SP,
        ppuY, ppuX,
        cyc
    );

    log << buffer << "\n";
}

std::string CPU6502::formatOperand(uint16_t pc) {
    uint8_t op = peek(pc);
    uint8_t b1 = peek(pc + 1);
    uint8_t b2 = peek(pc + 2);

    char buf[32]{};

    auto mode = lookup[op].addrmode;

    if (mode == &CPU6502::IMM) {
        snprintf(buf, sizeof(buf), "#$%02X", b1);
    }
    else if (mode == &CPU6502::ZP0) {
        snprintf(buf, sizeof(buf), "$%02X", b1);
    }
    else if (mode == &CPU6502::ZPX) {
        snprintf(buf, sizeof(buf), "$%02X,X", b1);
    }
    else if (mode == &CPU6502::ZPY) {
        snprintf(buf, sizeof(buf), "$%02X,Y", b1);
    }
    else if (mode == &CPU6502::ABS) {
        uint16_t addr = (b2 << 8) | b1;
        snprintf(buf, sizeof(buf), "$%04X", addr);
    }
    else if (mode == &CPU6502::ABX) {
        uint16_t addr = (b2 << 8) | b1;
        snprintf(buf, sizeof(buf), "$%04X,X", addr);
    }
    else if (mode == &CPU6502::ABY) {
        uint16_t addr = (b2 << 8) | b1;
        snprintf(buf, sizeof(buf), "$%04X,Y", addr);
    }
    else if (mode == &CPU6502::IND) {
        uint16_t addr = (b2 << 8) | b1;
        snprintf(buf, sizeof(buf), "($%04X)", addr);
    }
    else if (mode == &CPU6502::IZX) {
        snprintf(buf, sizeof(buf), "($%02X,X)", b1);
    }
    else if (mode == &CPU6502::IZY) {
        snprintf(buf, sizeof(buf), "($%02X),Y", b1);
    }
    else if (mode == &CPU6502::REL) {
        int8_t offset = static_cast<int8_t>(b1);
        uint16_t target = pc + 2 + offset;
        snprintf(buf, sizeof(buf), "$%04X", target);
    }
    else {
        buf[0] = '\0'; // IMP
    }

    return std::string(buf);
}

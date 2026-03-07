#pragma once

#include <array>
#include <cstdint>
#include <ostream>
#include <string>

class Bus;
class NES;

class CPU6502
{
  public:
    CPU6502();

    void connectBus(Bus* b);
    void setNES(NES* n);
    void setLogger(std::ofstream* log);

    void powerOn();
    void reset();

    void ExecOp();
    void clock();

    bool instructionComplete() const;

    void requestNMI();
    void requestIRQ();

    uint8_t GetFlag(uint8_t f) const;
    void    SetFlag(uint8_t f, bool v);

    void        logState(uint16_t pc_before);
    std::string getOperand(uint8_t (CPU6502::*mode)(void), uint8_t& b1, uint8_t& b2, uint16_t pc_before);

    bool isMemoryOpcode(uint8_t op) const;

  public:
    uint8_t        A           = 0;
    uint8_t        X           = 0;
    uint8_t        Y           = 0;
    uint8_t        SP          = 0xFD;
    uint8_t        P           = 0x24;
    uint16_t       PC          = 0;
    uint64_t       totalCycles = 0;
    std::ofstream* log         = nullptr;

  private:
    enum FLAGS6502
    {
        C = 1 << 0,
        Z = 1 << 1,
        I = 1 << 2,
        D = 1 << 3,
        B = 1 << 4,
        U = 1 << 5,
        V = 1 << 6,
        N = 1 << 7,
    };

    struct INSTRUCTION
    {
        const char* name;
        uint8_t (CPU6502::*operate)(void);
        uint8_t (CPU6502::*addrmode)(void);
        uint8_t cycles;
    };

  private:
    Bus* bus = nullptr;
    NES* nes = nullptr;

    std::array<INSTRUCTION, 256> lookup;

    uint8_t  opcode   = 0;
    uint8_t  fetched  = 0;
    uint16_t addr_abs = 0;
    uint16_t addr_rel = 0;

    uint8_t cycles = 0;

    bool nmi_pending = false;
    bool irq_pending = false;

  private:
    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t data);

    uint8_t fetch();

    void    push(uint8_t v);
    uint8_t pull();

    void handleInterrupt(uint16_t vector);

    void buildOpcodeTable();

    std::string disassemble(uint16_t addr);

    // Addressing modes
    uint8_t IMP();
    uint8_t IMM();
    uint8_t ZP0();
    uint8_t ZPX();
    uint8_t ZPY();
    uint8_t ABS();
    uint8_t ABX();
    uint8_t ABY();
    uint8_t REL();
    uint8_t IND();
    uint8_t IZX();
    uint8_t IZY();

    // Official opcodes (core)
    uint8_t NOP();
    uint8_t LDA();
    uint8_t STA();
    uint8_t TAX();
    uint8_t INX();
    uint8_t JMP();
    uint8_t JSR();
    uint8_t RTS();
    uint8_t BRK();
    uint8_t BNE();

    // Arithmetic / Logic
    uint8_t ADC();
    uint8_t SBC();
    uint8_t AND();
    uint8_t ORA();
    uint8_t EOR();
    uint8_t CMP();
    uint8_t CPX();
    uint8_t CPY();

    // Memory modify
    uint8_t INC();
    uint8_t INY();
    uint8_t DEX();
    uint8_t DEY();
    uint8_t DEC();

    // Loads / Stores
    uint8_t LDX();
    uint8_t LDY();
    uint8_t STX();
    uint8_t STY();

    // Stack
    uint8_t PHA();
    uint8_t PLA();
    uint8_t PHP();
    uint8_t PLP();

    // Flag ops
    uint8_t CLC();
    uint8_t SEC();
    uint8_t CLI();
    uint8_t SEI();
    uint8_t CLV();
    uint8_t CLD();
    uint8_t SED();

    // Shifts / Rotates
    uint8_t ASL();
    uint8_t LSR();
    uint8_t ROL();
    uint8_t ROR();

    // BIT
    uint8_t BIT();

    // Branches
    uint8_t BEQ();
    uint8_t BPL();
    uint8_t BMI();
    uint8_t BCC();
    uint8_t BCS();
    uint8_t BVC();
    uint8_t BVS();

    // Illegal opcodes
    uint8_t SLO();
    uint8_t RLA();
    uint8_t SRE();
    uint8_t RRA();
    uint8_t DCP();
    uint8_t ISC();
    uint8_t LAX();
    uint8_t SAX();
    uint8_t ANC();
    uint8_t ALR();
    uint8_t ARR();
    uint8_t KIL();
};

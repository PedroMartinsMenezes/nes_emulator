#pragma once
#include <cstdint>
#include <array>
#include <functional>

struct Bus;

class CPU6502 {
public:
    CPU6502();

    // Core control
    void reset();
    void irq();
    void nmi();
    void clock();

    void connectBus(Bus* b) { bus = b; }
    bool complete() const { return cycles == 0; }

    // Registers
    uint8_t  A = 0;
    uint8_t  X = 0;
    uint8_t  Y = 0;
    uint8_t  SP = 0xFD;
    uint16_t PC = 0x0000;
    uint8_t  P = 0x24; // IRQ disabled, unused set

private:
    Bus* bus = nullptr;

    uint8_t fetched = 0x00;
    uint16_t addr_abs = 0x0000;
    uint16_t addr_rel = 0x0000;
    uint8_t opcode = 0x00;
    uint8_t cycles = 0;

    // Flags
    enum FLAGS {
        C = (1 << 0),
        Z = (1 << 1),
        I = (1 << 2),
        D = (1 << 3),
        B = (1 << 4),
        U = (1 << 5),
        V = (1 << 6),
        N = (1 << 7),
    };

    //Flags
    uint8_t getFlag(FLAGS f);
    void    setFlag(FLAGS f, bool v);

    //Read and Write
    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t data);
    uint8_t fetch();

    //Stack
    void push(uint8_t v);
    uint8_t pull();

    //Branch
    void branch();

    // Addressing modes
    uint8_t IMP(); uint8_t IMM();
    uint8_t ZP0(); uint8_t ZPX(); uint8_t ZPY();
    uint8_t ABS(); uint8_t ABX(); uint8_t ABY();
    uint8_t IND(); uint8_t IZX(); uint8_t IZY();
    uint8_t REL();

    // Opcodes ==================================================================
    // Load / Store
    uint8_t LDA();
    uint8_t LDX();
    uint8_t LDY();

    uint8_t STA();
    uint8_t STX();
    uint8_t STY();

    // Register Transfers
    uint8_t TAX();
    uint8_t TAY();
    uint8_t TSX();
    uint8_t TXA();
    uint8_t TXS();
    uint8_t TYA();

    // Stack Operations
    uint8_t PHA();
    uint8_t PHP();
    uint8_t PLA();
    uint8_t PLP();

    // Logical Operations
    uint8_t AND();
    uint8_t ORA();
    uint8_t EOR();
    uint8_t BIT();

    // Arithmetic
    uint8_t ADC();
    uint8_t SBC();
    uint8_t CMP();
    uint8_t CPX();
    uint8_t CPY();

    // Increments / Decrements
    uint8_t INC();
    uint8_t INX();
    uint8_t INY();

    uint8_t DEC();
    uint8_t DEX();
    uint8_t DEY();

    // Shifts / Rotates
    uint8_t ASL();
    uint8_t LSR();
    uint8_t ROL();
    uint8_t ROR();

    // Jumps & Calls
    uint8_t JMP();
    uint8_t JSR();
    uint8_t RTS();
    uint8_t RTI();

    // Branches
    uint8_t BCC();
    uint8_t BCS();
    uint8_t BEQ();
    uint8_t BMI();
    uint8_t BNE();
    uint8_t BPL();
    uint8_t BVC();
    uint8_t BVS();

    // Status Flag Changes
    uint8_t CLC();
    uint8_t CLD();
    uint8_t CLI();
    uint8_t CLV();

    uint8_t SEC();
    uint8_t SED();
    uint8_t SEI();

    // System / Control
    uint8_t BRK();
    uint8_t NOP();

private:
    struct Instruction {
        const char* name;
        uint8_t(CPU6502::* operate)(void) = nullptr;
        uint8_t(CPU6502::* addrmode)(void) = nullptr;
        uint8_t cycles = 0;
    };

    std::array<Instruction, 256> lookup;
};

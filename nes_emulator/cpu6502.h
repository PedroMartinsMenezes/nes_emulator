#pragma once
#include <cstdint>
#include <string>
#include <array>
#include <functional>

class Bus;

class CPU6502 {
public:
    CPU6502();

    //Core control
    void reset();
    void irq();
    void nmi();
    void clock();

    void connectBus(Bus* b) { bus = b; }
    bool complete() const { return cycles == 0; }

    // Test Mode (used for testing)
    bool nestestMode = false;

    enum FLAGS;

    //Helpers
    uint8_t getFlag(FLAGS f) const;
    void    setFlag(FLAGS f, bool v);
    uint8_t read(uint16_t addr);
    void    write(uint16_t addr, uint8_t data);
    uint8_t fetch();
    void branch();

    //Stack
    void push(uint8_t v);
    uint8_t pull();

    // Addressing modes - https://www.nesdev.org/obelisk-6502-guide/addressing.html
    uint8_t IMP(); uint8_t IMM();
    uint8_t ZP0(); uint8_t ZPX(); uint8_t ZPY();
    uint8_t ABS(); uint8_t ABX(); uint8_t ABY();
    uint8_t IND(); uint8_t IZX(); uint8_t IZY();
    uint8_t REL();

    // Instruction Set - https://www.nesdev.org/wiki/Instruction_reference
    
    // Load and Store
    uint8_t LDA();
    uint8_t LDX();
    uint8_t LDY();
    uint8_t STA();
    uint8_t STX();
    uint8_t STY();

    // Register Transfers
    uint8_t TAX();
    uint8_t TAY();
    uint8_t TXA();
    uint8_t TYA();
    
    // Stack Operations
    uint8_t TSX();
    uint8_t TXS();
    uint8_t PHA();
    uint8_t PHP();
    uint8_t PLA();
    uint8_t PLP();

    // Logical Operations
    uint8_t AND();
    uint8_t EOR();
    uint8_t ORA();
    uint8_t BIT();

    // Arithmetic Operations
    uint8_t ADC();
    uint8_t SBC();

    // Comparison Operations
    uint8_t CMP();
    uint8_t CPX();
    uint8_t CPY();

    // Increments and Decrements
    uint8_t INC();
    uint8_t INX();
    uint8_t INY();
    uint8_t DEC();
    uint8_t DEX();
    uint8_t DEY();

    // Shifts and Rotates
    uint8_t ASL();
    uint8_t LSR();
    uint8_t ROL();
    uint8_t ROR();

    // Jumps and Calls
    uint8_t JMP();
    uint8_t JSR();
    uint8_t RTS();

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
    uint8_t SEC();
    uint8_t CLI();
    uint8_t SEI();
    uint8_t CLD();
    uint8_t SED();
    uint8_t CLV();

    // System Functions
    uint8_t BRK();
    uint8_t NOP();
    uint8_t RTI();
    
    //Illegal Instructions
    uint8_t NOP1();
    uint8_t LAX();
    uint8_t SAX();
    uint8_t DCP();
    uint8_t ISC();
    uint8_t SLO();
    uint8_t RLA();
    uint8_t SRE();
    uint8_t RRA();


    enum FLAGS {
        C = 0x01,   // Carry Flag
        Z = 0x02,   // Zero Flag
        I = 0x04,   // Interrupt Disable
        D = 0x08,   // Decimal Mode
        B = 0x10,   // Break Command
        U = 0x20,   // Always set to 1 when pushed; no CPU effect
        V = 0x40,   // Overflow Flag
        N = 0x80,   // Negative Flag
    };

    struct Instruction {
        const char* name;
        uint8_t bytes = 0;
        uint8_t(CPU6502::* operate)(void) = nullptr;
        uint8_t(CPU6502::* addrmode)(void) = nullptr;
        uint8_t cycles = 0;
    };

public:

    //clock functions
    inline int ppuCycle() const { return (int)(totalCycles * 3) % 341; }
    inline int ppuScanline() const { return (int)(totalCycles * 3) / 341; }
    uint64_t totalCycles = 0;

    //log functions
    void logState(std::ofstream& log);
    bool isMemoryOpcode(uint8_t op) const;
    std::string formatOperand(uint16_t pc);
    uint8_t peek(uint16_t addr);
    uint16_t computeEffectiveAddressForLog(uint16_t pc);
    uint8_t getEffectiveValueForLog(uint16_t effectiveAddress);

public: //Inner Variables

    std::array<Instruction, 256> lookup;

    Bus*     bus        = nullptr;

    uint8_t  fetched    = 0x00;
    uint16_t addr_abs   = 0x0000;
    uint16_t addr_rel   = 0x0000;
    uint8_t  opcode     = 0x00;    
    uint8_t  cycles     = 0;

    // Registers - https://www.nesdev.org/obelisk-6502-guide/registers.html
    uint16_t PC         = 0x0000;   // Program Counter
    uint8_t  SP         = 0xFD;     // Stack Pointer
    uint8_t  A          = 0;        // Accumulator
    uint8_t  X          = 0;        // Index Register X
    uint8_t  Y          = 0;        // Index Register Y
    uint8_t  P          = 0x24;     // Processor Status

    //Stall
    uint16_t stall      = 0;        // ?
};

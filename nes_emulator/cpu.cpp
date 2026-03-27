#include "cpu.h"
#include "bus.h"
#include <cstring>
#include <cstdio>

// ---------------------------------------------------------------------------
// Addressing mode identifiers (matches Nintendulator naming)
// ---------------------------------------------------------------------------
enum AddrMode : uint8_t
{
    IMP, ACC, IMM, ZPG, ZPX, ZPY,
    REL, ABS, ABX, ABY, IND, INX, INY,
    ADR  // absolute address only, no value shown (JMP abs, JSR)
};

// ---------------------------------------------------------------------------
// Opcode table
// ---------------------------------------------------------------------------
struct OpInfo { const char* name; uint8_t mode; uint8_t cycles; };

static const OpInfo OPS[256] =
{
//       name     mode  cyc
/*00*/ { " BRK",   IMP,  7 },
/*01*/ { " ORA",  INX,  6 },
/*02*/ { "*KIL",  IMP,  2 },
/*03*/ { "*SLO",  INX,  8 },
/*04*/ { "*NOP",  ZPG,  3 },
/*05*/ { " ORA",  ZPG,  3 },
/*06*/ { " ASL",  ZPG,  5 },
/*07*/ { "*SLO",  ZPG,  5 },
/*08*/ { " PHP",  IMP,  3 },
/*09*/ { " ORA",  IMM,  2 },
/*0A*/ { " ASL",  ACC,  2 },
/*0B*/ { "*ANC",  IMM,  2 },
/*0C*/ { "*NOP",  ABS,  4 },
/*0D*/ { " ORA",  ABS,  4 },
/*0E*/ { " ASL",  ABS,  6 },
/*0F*/ { "*SLO",  ABS,  6 },
/*10*/ { " BPL",  REL,  2 },
/*11*/ { " ORA",  INY,  5 },
/*12*/ { "*KIL",  IMP,  2 },
/*13*/ { "*SLO",  INY,  8 },
/*14*/ { "*NOP",  ZPX,  4 },
/*15*/ { " ORA",  ZPX,  4 },
/*16*/ { " ASL",  ZPX,  6 },
/*17*/ { "*SLO",  ZPX,  6 },
/*18*/ { " CLC",  IMP,  2 },
/*19*/ { " ORA",  ABY,  4 },
/*1A*/ { "*NOP",  IMP,  2 },
/*1B*/ { "*SLO",  ABY,  7 },
/*1C*/ { "*NOP",  ABX,  4 },
/*1D*/ { " ORA",  ABX,  4 },
/*1E*/ { " ASL",  ABX,  7 },
/*1F*/ { "*SLO",  ABX,  7 },
/*20*/ { " JSR",  ADR,  6 },
/*21*/ { " AND",  INX,  6 },
/*22*/ { "*KIL",  IMP,  2 },
/*23*/ { "*RLA",  INX,  8 },
/*24*/ { " BIT",  ZPG,  3 },
/*25*/ { " AND",  ZPG,  3 },
/*26*/ { " ROL",  ZPG,  5 },
/*27*/ { "*RLA",  ZPG,  5 },
/*28*/ { " PLP",  IMP,  4 },
/*29*/ { " AND",  IMM,  2 },
/*2A*/ { " ROL",  ACC,  2 },
/*2B*/ { "*ANC",  IMM,  2 },
/*2C*/ { " BIT",  ABS,  4 },
/*2D*/ { " AND",  ABS,  4 },
/*2E*/ { " ROL",  ABS,  6 },
/*2F*/ { "*RLA",  ABS,  6 },
/*30*/ { " BMI",  REL,  2 },
/*31*/ { " AND",  INY,  5 },
/*32*/ { "*KIL",  IMP,  2 },
/*33*/ { "*RLA",  INY,  8 },
/*34*/ { "*NOP",  ZPX,  4 },
/*35*/ { " AND",  ZPX,  4 },
/*36*/ { " ROL",  ZPX,  6 },
/*37*/ { "*RLA",  ZPX,  6 },
/*38*/ { " SEC",  IMP,  2 },
/*39*/ { " AND",  ABY,  4 },
/*3A*/ { "*NOP",  IMP,  2 },
/*3B*/ { "*RLA",  ABY,  7 },
/*3C*/ { "*NOP",  ABX,  4 },
/*3D*/ { " AND",  ABX,  4 },
/*3E*/ { " ROL",  ABX,  7 },
/*3F*/ { "*RLA",  ABX,  7 },
/*40*/ { " RTI",  IMP,  6 },
/*41*/ { " EOR",  INX,  6 },
/*42*/ { "*KIL",  IMP,  2 },
/*43*/ { "*SRE",  INX,  8 },
/*44*/ { "*NOP",  ZPG,  3 },
/*45*/ { " EOR",  ZPG,  3 },
/*46*/ { " LSR",  ZPG,  5 },
/*47*/ { "*SRE",  ZPG,  5 },
/*48*/ { " PHA",  IMP,  3 },
/*49*/ { " EOR",  IMM,  2 },
/*4A*/ { " LSR",  ACC,  2 },
/*4B*/ { "*ALR",  IMM,  2 },
/*4C*/ { " JMP",  ADR,  3 },
/*4D*/ { " EOR",  ABS,  4 },
/*4E*/ { " LSR",  ABS,  6 },
/*4F*/ { "*SRE",  ABS,  6 },
/*50*/ { " BVC",  REL,  2 },
/*51*/ { " EOR",  INY,  5 },
/*52*/ { "*KIL",  IMP,  2 },
/*53*/ { "*SRE",  INY,  8 },
/*54*/ { "*NOP",  ZPX,  4 },
/*55*/ { " EOR",  ZPX,  4 },
/*56*/ { " LSR",  ZPX,  6 },
/*57*/ { "*SRE",  ZPX,  6 },
/*58*/ { " CLI",  IMP,  2 },
/*59*/ { " EOR",  ABY,  4 },
/*5A*/ { "*NOP",  IMP,  2 },
/*5B*/ { "*SRE",  ABY,  7 },
/*5C*/ { "*NOP",  ABX,  4 },
/*5D*/ { " EOR",  ABX,  4 },
/*5E*/ { " LSR",  ABX,  7 },
/*5F*/ { "*SRE",  ABX,  7 },
/*60*/ { " RTS",  IMP,  6 },
/*61*/ { " ADC",  INX,  6 },
/*62*/ { "*KIL",  IMP,  2 },
/*63*/ { "*RRA",  INX,  8 },
/*64*/ { "*NOP",  ZPG,  3 },
/*65*/ { " ADC",  ZPG,  3 },
/*66*/ { " ROR",  ZPG,  5 },
/*67*/ { "*RRA",  ZPG,  5 },
/*68*/ { " PLA",  IMP,  4 },
/*69*/ { " ADC",  IMM,  2 },
/*6A*/ { " ROR",  ACC,  2 },
/*6B*/ { "*ARR",  IMM,  2 },
/*6C*/ { " JMP",  IND,  5 },
/*6D*/ { " ADC",  ABS,  4 },
/*6E*/ { " ROR",  ABS,  6 },
/*6F*/ { "*RRA",  ABS,  6 },
/*70*/ { " BVS",  REL,  2 },
/*71*/ { " ADC",  INY,  5 },
/*72*/ { "*KIL",  IMP,  2 },
/*73*/ { "*RRA",  INY,  8 },
/*74*/ { "*NOP",  ZPX,  4 },
/*75*/ { " ADC",  ZPX,  4 },
/*76*/ { " ROR",  ZPX,  6 },
/*77*/ { "*RRA",  ZPX,  6 },
/*78*/ { " SEI",  IMP,  2 },
/*79*/ { " ADC",  ABY,  4 },
/*7A*/ { "*NOP",  IMP,  2 },
/*7B*/ { "*RRA",  ABY,  7 },
/*7C*/ { "*NOP",  ABX,  4 },
/*7D*/ { " ADC",  ABX,  4 },
/*7E*/ { " ROR",  ABX,  7 },
/*7F*/ { "*RRA",  ABX,  7 },
/*80*/ { "*NOP",  IMM,  2 },
/*81*/ { " STA",  INX,  6 },
/*82*/ { "*NOP",  IMM,  2 },
/*83*/ { "*SAX",  INX,  6 },
/*84*/ { " STY",  ZPG,  3 },
/*85*/ { " STA",  ZPG,  3 },
/*86*/ { " STX",  ZPG,  3 },
/*87*/ { "*SAX",  ZPG,  3 },
/*88*/ { " DEY",  IMP,  2 },
/*89*/ { "*NOP",  IMM,  2 },
/*8A*/ { " TXA",  IMP,  2 },
/*8B*/ { "*XAA",  IMM,  2 },
/*8C*/ { " STY",  ABS,  4 },
/*8D*/ { " STA",  ABS,  4 },
/*8E*/ { " STX",  ABS,  4 },
/*8F*/ { "*SAX",  ABS,  4 },
/*90*/ { " BCC",  REL,  2 },
/*91*/ { " STA",  INY,  6 },
/*92*/ { "*KIL",  IMP,  2 },
/*93*/ { "*AHX",  INY,  6 },
/*94*/ { " STY",  ZPX,  4 },
/*95*/ { " STA",  ZPX,  4 },
/*96*/ { " STX",  ZPY,  4 },
/*97*/ { "*SAX",  ZPY,  4 },
/*98*/ { " TYA",  IMP,  2 },
/*99*/ { " STA",  ABY,  5 },
/*9A*/ { " TXS",  IMP,  2 },
/*9B*/ { "*TAS",  ABY,  5 },
/*9C*/ { "*SHY",  ABX,  5 },
/*9D*/ { " STA",  ABX,  5 },
/*9E*/ { "*SHX",  ABY,  5 },
/*9F*/ { "*AHX",  ABY,  5 },
/*A0*/ { " LDY",  IMM,  2 },
/*A1*/ { " LDA",  INX,  6 },
/*A2*/ { " LDX",  IMM,  2 },
/*A3*/ { "*LAX",  INX,  6 },
/*A4*/ { " LDY",  ZPG,  3 },
/*A5*/ { " LDA",  ZPG,  3 },
/*A6*/ { " LDX",  ZPG,  3 },
/*A7*/ { "*LAX",  ZPG,  3 },
/*A8*/ { " TAY",  IMP,  2 },
/*A9*/ { " LDA",  IMM,  2 },
/*AA*/ { " TAX",  IMP,  2 },
/*AB*/ { "*LAX",  IMM,  2 },
/*AC*/ { " LDY",  ABS,  4 },
/*AD*/ { " LDA",  ABS,  4 },
/*AE*/ { " LDX",  ABS,  4 },
/*AF*/ { "*LAX",  ABS,  4 },
/*B0*/ { " BCS",  REL,  2 },
/*B1*/ { " LDA",  INY,  5 },
/*B2*/ { "*KIL",  IMP,  2 },
/*B3*/ { "*LAX",  INY,  5 },
/*B4*/ { " LDY",  ZPX,  4 },
/*B5*/ { " LDA",  ZPX,  4 },
/*B6*/ { " LDX",  ZPY,  4 },
/*B7*/ { "*LAX",  ZPY,  4 },
/*B8*/ { " CLV",  IMP,  2 },
/*B9*/ { " LDA",  ABY,  4 },
/*BA*/ { " TSX",  IMP,  2 },
/*BB*/ { "*LAS",  ABY,  4 },
/*BC*/ { " LDY",  ABX,  4 },
/*BD*/ { " LDA",  ABX,  4 },
/*BE*/ { " LDX",  ABY,  4 },
/*BF*/ { "*LAX",  ABY,  4 },
/*C0*/ { " CPY",  IMM,  2 },
/*C1*/ { " CMP",  INX,  6 },
/*C2*/ { "*NOP",  IMM,  2 },
/*C3*/ { "*DCP",  INX,  8 },
/*C4*/ { " CPY",  ZPG,  3 },
/*C5*/ { " CMP",  ZPG,  3 },
/*C6*/ { " DEC",  ZPG,  5 },
/*C7*/ { "*DCP",  ZPG,  5 },
/*C8*/ { " INY",  IMP,  2 },
/*C9*/ { " CMP",  IMM,  2 },
/*CA*/ { " DEX",  IMP,  2 },
/*CB*/ { "*AXS",  IMM,  2 },
/*CC*/ { " CPY",  ABS,  4 },
/*CD*/ { " CMP",  ABS,  4 },
/*CE*/ { " DEC",  ABS,  6 },
/*CF*/ { "*DCP",  ABS,  6 },
/*D0*/ { " BNE",  REL,  2 },
/*D1*/ { " CMP",  INY,  5 },
/*D2*/ { "*KIL",  IMP,  2 },
/*D3*/ { "*DCP",  INY,  8 },
/*D4*/ { "*NOP",  ZPX,  4 },
/*D5*/ { " CMP",  ZPX,  4 },
/*D6*/ { " DEC",  ZPX,  6 },
/*D7*/ { "*DCP",  ZPX,  6 },
/*D8*/ { " CLD",  IMP,  2 },
/*D9*/ { " CMP",  ABY,  4 },
/*DA*/ { "*NOP",  IMP,  2 },
/*DB*/ { "*DCP",  ABY,  7 },
/*DC*/ { "*NOP",  ABX,  4 },
/*DD*/ { " CMP",  ABX,  4 },
/*DE*/ { " DEC",  ABX,  7 },
/*DF*/ { "*DCP",  ABX,  7 },
/*E0*/ { " CPX",  IMM,  2 },
/*E1*/ { " SBC",  INX,  6 },
/*E2*/ { "*NOP",  IMM,  2 },
/*E3*/ { "*ISB",  INX,  8 },
/*E4*/ { " CPX",  ZPG,  3 },
/*E5*/ { " SBC",  ZPG,  3 },
/*E6*/ { " INC",  ZPG,  5 },
/*E7*/ { "*ISB",  ZPG,  5 },
/*E8*/ { " INX",  IMP,  2 },
/*E9*/ { " SBC",  IMM,  2 },
/*EA*/ { " NOP",  IMP,  2 },
/*EB*/ { "*SBC",  IMM,  2 },
/*EC*/ { " CPX",  ABS,  4 },
/*ED*/ { " SBC",  ABS,  4 },
/*EE*/ { " INC",  ABS,  6 },
/*EF*/ { "*ISB",  ABS,  6 },
/*F0*/ { " BEQ",  REL,  2 },
/*F1*/ { " SBC",  INY,  5 },
/*F2*/ { "*KIL",  IMP,  2 },
/*F3*/ { "*ISB",  INY,  8 },
/*F4*/ { "*NOP",  ZPX,  4 },
/*F5*/ { " SBC",  ZPX,  4 },
/*F6*/ { " INC",  ZPX,  6 },
/*F7*/ { "*ISB",  ZPX,  6 },
/*F8*/ { " SED",  IMP,  2 },
/*F9*/ { " SBC",  ABY,  4 },
/*FA*/ { "*NOP",  IMP,  2 },
/*FB*/ { "*ISB",  ABY,  7 },
/*FC*/ { "*NOP",  ABX,  4 },
/*FD*/ { " SBC",  ABX,  4 },
/*FE*/ { " INC",  ABX,  7 },
/*FF*/ { "*ISB",  ABX,  7 },
};

// ---------------------------------------------------------------------------
// Memory helpers
// ---------------------------------------------------------------------------
uint8_t CPU::mem_read(uint16_t addr)              { return m_bus->read(addr); }
void    CPU::mem_write(uint16_t addr, uint8_t v)  { m_bus->write(addr, v); }

uint16_t CPU::mem_read16(uint16_t addr)
{
    return (uint16_t)mem_read(addr) | ((uint16_t)mem_read(addr + 1) << 8);
}

// Zero-page wrap: $00FF + 1 wraps to $0000, not $0100
uint16_t CPU::mem_read16_zp(uint8_t addr)
{
    uint8_t lo = mem_read(addr);
    uint8_t hi = mem_read((uint8_t)(addr + 1));
    return (uint16_t)lo | ((uint16_t)hi << 8);
}

// ---------------------------------------------------------------------------
// Stack
// ---------------------------------------------------------------------------
void     CPU::push(uint8_t v)   { mem_write(0x0100 | SP--, v); }
uint8_t  CPU::pop()             { return mem_read(0x0100 | ++SP); }
void     CPU::push16(uint16_t v){ push(v >> 8); push(v & 0xFF); }
uint16_t CPU::pop16()           { uint16_t lo = pop(); return lo | ((uint16_t)pop() << 8); }

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------
void CPU::reset()
{
    A  = 0x00; X = 0x00; Y = 0x00;
    SP = 0xFD; P = 0x24;
    PC = mem_read16(0xFFFC);
    cycles = 7;  // reset sequence takes 7 cycles
}

// ---------------------------------------------------------------------------
// ALU helpers
// ---------------------------------------------------------------------------
void CPU::do_adc(uint8_t val)
{
    uint16_t r = (uint16_t)A + val + (get_C() ? 1 : 0);
    set_C(r > 0xFF);
    set_V(!((A ^ val) & 0x80) && ((A ^ r) & 0x80));
    A = (uint8_t)r;
    set_NZ(A);
}

void CPU::do_sbc(uint8_t val)
{
    do_adc(val ^ 0xFF);
}

void CPU::do_cmp(uint8_t reg, uint8_t val)
{
    set_C(reg >= val);
    set_NZ(reg - val);
}

uint8_t CPU::do_asl(uint8_t val) { set_C(val & 0x80); val <<= 1; set_NZ(val); return val; }
uint8_t CPU::do_lsr(uint8_t val) { set_C(val & 0x01); val >>= 1; set_NZ(val); return val; }
uint8_t CPU::do_rol(uint8_t val) { uint8_t c=get_C(); set_C(val&0x80); val=(val<<1)|c; set_NZ(val); return val; }
uint8_t CPU::do_ror(uint8_t val) { uint8_t c=get_C(); set_C(val&0x01); val=(val>>1)|(c<<7); set_NZ(val); return val; }

// ---------------------------------------------------------------------------
// Log prefix — matches Nintendulator DecodeInstruction str1 format
// ---------------------------------------------------------------------------
void CPU::build_log_prefix(uint16_t pc, uint8_t op,
                            uint8_t op1, uint8_t op2,
                            const char* name, uint8_t mode,
                            uint16_t addr, uint8_t val,
                            uint16_t mid_addr, uint16_t eff_addr,
                            char* out) const
{
    //simulating the logic of 'ReadUnsafe' function
    int bank = (addr >> 12) & 0xF;
    val      = (bank > 3 && strcmp(name, " STA") == 0) ? 0xFF : val;

    switch (mode)
    {
    case IMP:
        sprintf(out, "%04X  %02X       %s                           ",
            pc, op, name);
        break;
    case ACC:
        sprintf(out, "%04X  %02X       %s A                         ",
            pc, op, name);
        break;
    case IMM:
        sprintf(out, "%04X  %02X %02X    %s #$%02X                      ",
            pc, op, op1, name, op1);
        break;
    case REL:
        // addr = resolved branch target (16-bit)
        sprintf(out, "%04X  %02X %02X    %s $%04X                     ",
            pc, op, op1, name, addr);
        break;
    case ZPG:
        sprintf(out, "%04X  %02X %02X    %s $%02X = %02X                  ",
            pc, op, op1, name, (uint8_t)addr, val);
        break;
    case ZPX:
        // op1=base, eff_addr=base+X (zp), val=mem[eff_addr]
        sprintf(out, "%04X  %02X %02X    %s $%02X,X @ %02X = %02X           ",
            pc, op, op1, name, op1, (uint8_t)eff_addr, val);
        break;
    case ZPY:
        sprintf(out, "%04X  %02X %02X    %s $%02X,Y @ %02X = %02X           ",
            pc, op, op1, name, op1, (uint8_t)eff_addr, val);
        break;
    case INX:
        // op1=base, mid_addr=base+X (zp), eff_addr=mem16[mid_addr], val=mem[eff_addr]
        sprintf(out, "%04X  %02X %02X    %s ($%02X,X) @ %02X = %04X = %02X  ",
            pc, op, op1, name, op1, (uint8_t)mid_addr, eff_addr, val);
        break;
    case INY:
        // op1=base, mid_addr=mem16[base], eff_addr=mid_addr+Y, val=mem[eff_addr]
        sprintf(out, "%04X  %02X %02X    %s ($%02X),Y = %04X @ %04X = %02X",
            pc, op, op1, name, op1, mid_addr, eff_addr, val);
        break;
    case ADR:
        // JMP abs / JSR: show address, no value
        sprintf(out, "%04X  %02X %02X %02X %s $%04X                     ",
            pc, op, op1, op2, name, addr);
        break;
    case ABS:
        sprintf(out, "%04X  %02X %02X %02X %s $%04X = %02X                ",
            pc, op, op1, op2, name, addr, val);
        break;
    case IND:
        // JMP ind: op shows operand address, eff_addr = jump target
        sprintf(out, "%04X  %02X %02X %02X %s ($%04X) = %04X            ",
            pc, op, op1, op2, name, addr, eff_addr);
        break;
    case ABX:
        sprintf(out, "%04X  %02X %02X %02X %s $%04X,X @ %04X = %02X       ",
            pc, op, op1, op2, name, addr, eff_addr, val);
        break;
    case ABY:
        sprintf(out, "%04X  %02X %02X %02X %s $%04X,Y @ %04X = %02X       ",
            pc, op, op1, op2, name, addr, eff_addr, val);
        break;
    default:
        sprintf(out, "%04X  %02X       ???                          ", pc, op);
        break;
    }
}

// ---------------------------------------------------------------------------
// step() — fetch, decode, log, execute one instruction
// ---------------------------------------------------------------------------
void CPU::step()
{
    // ---- Capture state for log (before execution) ----
    uint16_t log_pc  = PC;
    uint8_t  log_a   = A, log_x = X, log_y = Y, log_p = P, log_sp = SP;
    uint64_t log_cyc = cycles;

    // ---- Fetch opcode ----
    uint8_t opcode = mem_read(PC++);
    const OpInfo& op = OPS[opcode];

    // ---- Address resolution ----
    uint8_t  op1 = 0, op2 = 0;
    uint16_t addr = 0;          // operand address (for ABS/ZPG) or raw IMM byte addr
    uint16_t eff_addr = 0;      // effective address after indexing
    uint16_t mid_addr = 0;      // intermediate address (INX/INY)
    uint8_t  val = 0;           // memory value at effective address (for log)
    bool     page_cross = false;

    switch (op.mode)
    {
    case IMP:
    case ACC:
        break;

    case IMM:
        op1 = mem_read(PC);
        addr = PC++;
        val = op1;
        break;

    case ZPG:
        op1 = mem_read(PC++);
        addr = op1;
        eff_addr = addr;
        val = mem_read(addr);
        break;

    case ZPX:
        op1 = mem_read(PC++);
        eff_addr = (uint8_t)(op1 + X);
        addr = op1;
        val = mem_read(eff_addr);
        break;

    case ZPY:
        op1 = mem_read(PC++);
        eff_addr = (uint8_t)(op1 + Y);
        addr = op1;
        val = mem_read(eff_addr);
        break;

    case REL:
    {
        op1 = mem_read(PC++);
        int8_t offset = (int8_t)op1;
        uint16_t target = PC + offset;
        page_cross = ((PC & 0xFF00) != (target & 0xFF00));
        addr = target;  // resolved branch target (used in log as $TTTT)
        break;
    }

    case ADR:  // JMP abs / JSR
        op1  = mem_read(PC++);
        op2  = mem_read(PC++);
        addr = (uint16_t)op1 | ((uint16_t)op2 << 8);
        break;

    case ABS:
        op1      = mem_read(PC++);
        op2      = mem_read(PC++);
        addr     = (uint16_t)op1 | ((uint16_t)op2 << 8);
        eff_addr = addr;
        val      = mem_read(addr);
        break;

    case ABX:
        op1      = mem_read(PC++);
        op2      = mem_read(PC++);
        addr     = (uint16_t)op1 | ((uint16_t)op2 << 8);
        eff_addr = addr + X;
        page_cross = ((addr & 0xFF00) != (eff_addr & 0xFF00));
        val = mem_read(eff_addr);
        break;

    case ABY:
        op1      = mem_read(PC++);
        op2      = mem_read(PC++);
        addr     = (uint16_t)op1 | ((uint16_t)op2 << 8);
        eff_addr = addr + Y;
        page_cross = ((addr & 0xFF00) != (eff_addr & 0xFF00));
        val = mem_read(eff_addr);
        break;

    case IND:
    {
        op1 = mem_read(PC++);
        op2 = mem_read(PC++);
        uint16_t ptr = (uint16_t)op1 | ((uint16_t)op2 << 8);
        // 6502 page-wrap bug: if ptr = $xxFF, high byte wraps to $xx00
        uint16_t lo = mem_read(ptr);
        uint16_t hi = mem_read((ptr & 0xFF00) | ((ptr + 1) & 0x00FF));
        eff_addr = lo | (hi << 8);
        addr = ptr;
        break;
    }

    case INX:
    {
        op1 = mem_read(PC++);
        uint8_t zp = (uint8_t)(op1 + X);
        mid_addr = zp;
        eff_addr = mem_read16_zp(zp);
        val = mem_read(eff_addr);
        addr = op1;
        break;
    }

    case INY:
    {
        op1 = mem_read(PC++);
        mid_addr = mem_read16_zp(op1);
        eff_addr = mid_addr + Y;
        page_cross = ((mid_addr & 0xFF00) != (eff_addr & 0xFF00));
        val = mem_read(eff_addr);
        addr = op1;
        break;
    }
    }

    // ---- Build log prefix (before execution) ----
    char prefix[64];
    build_log_prefix(log_pc, opcode, op1, op2, op.name, op.mode,
                     addr, val, mid_addr, eff_addr, prefix);

    // ---- Execute ----
    int extra = 0;  // extra cycles (page cross, branch taken)

    switch (opcode)
    {
    // ---- LDA ----
    case 0xA9: case 0xA5: case 0xB5: case 0xAD: case 0xBD:
    case 0xB9: case 0xA1: case 0xB1:
        A = val; set_NZ(A);
        if (page_cross) extra = 1;
        break;

    // ---- LDX ----
    case 0xA2: case 0xA6: case 0xB6: case 0xAE: case 0xBE:
        X = val; set_NZ(X);
        if (page_cross) extra = 1;
        break;

    // ---- LDY ----
    case 0xA0: case 0xA4: case 0xB4: case 0xAC: case 0xBC:
        Y = val; set_NZ(Y);
        if (page_cross) extra = 1;
        break;

    // ---- STA ----
    case 0x85: case 0x95: case 0x8D: case 0x9D: case 0x99:
    case 0x81: case 0x91:
        mem_write(eff_addr, A);
        break;

    // ---- STX ----
    case 0x86: case 0x96: case 0x8E:
        mem_write(eff_addr, X);
        break;

    // ---- STY ----
    case 0x84: case 0x94: case 0x8C:
        mem_write(eff_addr, Y);
        break;

    // ---- ADC ----
    case 0x69: case 0x65: case 0x75: case 0x6D: case 0x7D:
    case 0x79: case 0x61: case 0x71:
        do_adc(val);
        if (page_cross) extra = 1;
        break;

    // ---- SBC ----
    case 0xE9: case 0xEB: case 0xE5: case 0xF5: case 0xED:
    case 0xFD: case 0xF9: case 0xE1: case 0xF1:
        do_sbc(val);
        if (page_cross) extra = 1;
        break;

    // ---- AND ----
    case 0x29: case 0x25: case 0x35: case 0x2D: case 0x3D:
    case 0x39: case 0x21: case 0x31:
        A &= val; set_NZ(A);
        if (page_cross) extra = 1;
        break;

    // ---- ORA ----
    case 0x09: case 0x05: case 0x15: case 0x0D: case 0x1D:
    case 0x19: case 0x01: case 0x11:
        A |= val; set_NZ(A);
        if (page_cross) extra = 1;
        break;

    // ---- EOR ----
    case 0x49: case 0x45: case 0x55: case 0x4D: case 0x5D:
    case 0x59: case 0x41: case 0x51:
        A ^= val; set_NZ(A);
        if (page_cross) extra = 1;
        break;

    // ---- CMP ----
    case 0xC9: case 0xC5: case 0xD5: case 0xCD: case 0xDD:
    case 0xD9: case 0xC1: case 0xD1:
        do_cmp(A, val);
        if (page_cross) extra = 1;
        break;

    // ---- CPX ----
    case 0xE0: case 0xE4: case 0xEC:
        do_cmp(X, val);
        break;

    // ---- CPY ----
    case 0xC0: case 0xC4: case 0xCC:
        do_cmp(Y, val);
        break;

    // ---- BIT ----
    case 0x24: case 0x2C:
        set_N(val & 0x80);
        set_V(val & 0x40);
        set_Z((A & val) == 0);
        break;

    // ---- ASL (accumulator) ----
    case 0x0A:
        A = do_asl(A);
        break;

    // ---- ASL (memory) ----
    case 0x06: case 0x16: case 0x0E: case 0x1E:
    { uint8_t r = do_asl(val); mem_write(eff_addr, r); break; }

    // ---- LSR (accumulator) ----
    case 0x4A:
        A = do_lsr(A);
        break;

    // ---- LSR (memory) ----
    case 0x46: case 0x56: case 0x4E: case 0x5E:
    { uint8_t r = do_lsr(val); mem_write(eff_addr, r); break; }

    // ---- ROL (accumulator) ----
    case 0x2A:
        A = do_rol(A);
        break;

    // ---- ROL (memory) ----
    case 0x26: case 0x36: case 0x2E: case 0x3E:
    { uint8_t r = do_rol(val); mem_write(eff_addr, r); break; }

    // ---- ROR (accumulator) ----
    case 0x6A:
        A = do_ror(A);
        break;

    // ---- ROR (memory) ----
    case 0x66: case 0x76: case 0x6E: case 0x7E:
    { uint8_t r = do_ror(val); mem_write(eff_addr, r); break; }

    // ---- INC ----
    case 0xE6: case 0xF6: case 0xEE: case 0xFE:
    { uint8_t r = val + 1; set_NZ(r); mem_write(eff_addr, r); break; }

    // ---- DEC ----
    case 0xC6: case 0xD6: case 0xCE: case 0xDE:
    { uint8_t r = val - 1; set_NZ(r); mem_write(eff_addr, r); break; }

    // ---- INX / INY / DEX / DEY ----
    case 0xE8: X++; set_NZ(X); break;
    case 0xC8: Y++; set_NZ(Y); break;
    case 0xCA: X--; set_NZ(X); break;
    case 0x88: Y--; set_NZ(Y); break;

    // ---- Transfer ----
    case 0xAA: X = A; set_NZ(X); break;
    case 0xA8: Y = A; set_NZ(Y); break;
    case 0x8A: A = X; set_NZ(A); break;
    case 0x98: A = Y; set_NZ(A); break;
    case 0xBA: X = SP; set_NZ(X); break;
    case 0x9A: SP = X; break;

    // ---- Flag ops ----
    case 0x18: set_C(false); break;
    case 0x38: set_C(true);  break;
    case 0x58: set_I(false); break;
    case 0x78: set_I(true);  break;
    case 0xB8: set_V(false); break;
    case 0xD8: set_D(false); break;
    case 0xF8: set_D(true);  break;

    // ---- JMP absolute ----
    case 0x4C:
        PC = addr;
        break;

    // ---- JMP indirect ----
    case 0x6C:
        PC = eff_addr;
        break;

    // ---- JSR ----
    case 0x20:
        push16(PC - 1);
        PC = addr;
        break;

    // ---- RTS ----
    case 0x60:
        PC = pop16() + 1;
        break;

    // ---- RTI ----
    case 0x40:
        P  = (pop() & 0xEF) | 0x20;  // clear B, set unused
        PC = pop16();
        break;

    // ---- PHA / PLA / PHP / PLP ----
    case 0x48: push(A); break;
    case 0x68: A = pop(); set_NZ(A); break;
    case 0x08: push(P | 0x30); break;  // B and unused both set on push
    case 0x28: P = (pop() & 0xEF) | 0x20; break;

    // ---- BRK ----
    case 0x00:
        push16(PC + 1);
        push(P | 0x30);
        set_I(true);
        PC = mem_read16(0xFFFE);
        break;

    // ---- Branches ----
    case 0x10: // BPL
        if (!get_N()) { extra = page_cross ? 2 : 1; PC = addr; }
        break;
    case 0x30: // BMI
        if ( get_N()) { extra = page_cross ? 2 : 1; PC = addr; }
        break;
    case 0x50: // BVC
        if (!get_V()) { extra = page_cross ? 2 : 1; PC = addr; }
        break;
    case 0x70: // BVS
        if ( get_V()) { extra = page_cross ? 2 : 1; PC = addr; }
        break;
    case 0x90: // BCC
        if (!get_C()) { extra = page_cross ? 2 : 1; PC = addr; }
        break;
    case 0xB0: // BCS
        if ( get_C()) { extra = page_cross ? 2 : 1; PC = addr; }
        break;
    case 0xD0: // BNE
        if (!get_Z()) { extra = page_cross ? 2 : 1; PC = addr; }
        break;
    case 0xF0: // BEQ
        if ( get_Z()) { extra = page_cross ? 2 : 1; PC = addr; }
        break;

    // ---- NOP (official + unofficial) ----
    case 0xEA: case 0x1A: case 0x3A: case 0x5A: case 0x7A:
    case 0xDA: case 0xFA:
        break;
    // NOP with address reads (IGN) - read but discard, page cross +1
    case 0x04: case 0x44: case 0x64:
    case 0x0C:
    case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4:
    case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC:
    case 0x80: case 0x82: case 0x89: case 0xC2: case 0xE2:
        if (page_cross) extra = 1;
        break;

    // ---- *KIL (halts CPU — treat as NOP for emulation purposes) ----
    case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
    case 0x62: case 0x72: case 0x92: case 0xB2: case 0xD2: case 0xF2:
        break;

    // ---- *LAX: LDA + LDX ----
    case 0xA3: case 0xA7: case 0xAF: case 0xB3: case 0xB7: case 0xBF:
        A = X = val; set_NZ(A);
        if (page_cross) extra = 1;
        break;
    case 0xAB: // *LAX imm (unstable: typically A = (A | 0xFF) & X & imm)
        A = X = val; set_NZ(A);
        break;

    // ---- *SAX: store A & X ----
    case 0x83: case 0x87: case 0x8F: case 0x97:
        mem_write(eff_addr, A & X);
        break;

    // ---- *DCP: DEC then CMP ----
    case 0xC3: case 0xC7: case 0xCF: case 0xD3: case 0xD7:
    case 0xDB: case 0xDF:
    { uint8_t r = val - 1; mem_write(eff_addr, r); do_cmp(A, r); break; }

    // ---- *ISB/*ISC: INC then SBC ----
    case 0xE3: case 0xE7: case 0xEF: case 0xF3: case 0xF7:
    case 0xFB: case 0xFF:
    { uint8_t r = val + 1; mem_write(eff_addr, r); do_sbc(r); break; }

    // ---- *SLO: ASL then ORA ----
    case 0x03: case 0x07: case 0x0F: case 0x13: case 0x17:
    case 0x1B: case 0x1F:
    { uint8_t r = do_asl(val); mem_write(eff_addr, r); A |= r; set_NZ(A); break; }

    // ---- *SRE: LSR then EOR ----
    case 0x43: case 0x47: case 0x4F: case 0x53: case 0x57:
    case 0x5B: case 0x5F:
    { uint8_t r = do_lsr(val); mem_write(eff_addr, r); A ^= r; set_NZ(A); break; }

    // ---- *RLA: ROL then AND ----
    case 0x23: case 0x27: case 0x2F: case 0x33: case 0x37:
    case 0x3B: case 0x3F:
    { uint8_t r = do_rol(val); mem_write(eff_addr, r); A &= r; set_NZ(A); break; }

    // ---- *RRA: ROR then ADC ----
    case 0x63: case 0x67: case 0x6F: case 0x73: case 0x77:
    case 0x7B: case 0x7F:
    { uint8_t r = do_ror(val); mem_write(eff_addr, r); do_adc(r); break; }

    // ---- *ANC: AND then set C from N ----
    case 0x0B: case 0x2B:
        A &= val; set_NZ(A); set_C(get_N());
        break;

    // ---- *ALR: AND then LSR A ----
    case 0x4B:
        A &= val; A = do_lsr(A);
        break;

    // ---- *ARR: AND then ROR A (with special carry/overflow) ----
    case 0x6B:
        A &= val;
        A = (A >> 1) | (get_C() ? 0x80 : 0x00);
        set_N(A & 0x80);
        set_Z(A == 0);
        set_C(A & 0x40);
        set_V((A & 0x40) ^ ((A & 0x20) << 1));
        break;

    // ---- *AXS: A & X - imm → X (no borrow from C) ----
    case 0xCB:
    {
        uint8_t r = A & X;
        set_C(r >= val);
        X = r - val;
        set_NZ(X);
        break;
    }

    // ---- *LAS: mem & SP → A, X, SP ----
    case 0xBB:
        A = X = SP = (val & SP); set_NZ(A);
        if (page_cross) extra = 1;
        break;

    // ---- *XAA: TXA + AND (unstable, common magic = 0xFF) ----
    case 0x8B:
        A = X & val; set_NZ(A);
        break;

    // ---- *AHX: store A & X & (high(addr)+1) ----
    case 0x93: case 0x9F:
    {
        uint8_t r = A & X & (uint8_t)((eff_addr >> 8) + 1);
        mem_write(eff_addr, r);
        break;
    }

    // ---- *SHX: X & (high(addr)+1) → mem ----
    case 0x9E:
    {
        uint8_t r = X & (uint8_t)((eff_addr >> 8) + 1);
        mem_write(eff_addr, r);
        break;
    }

    // ---- *SHY: Y & (high(addr)+1) → mem ----
    case 0x9C:
    {
        uint8_t r = Y & (uint8_t)((eff_addr >> 8) + 1);
        mem_write(eff_addr, r);
        break;
    }

    // ---- *TAS: SP = A & X, store A & X & (high(addr)+1) ----
    case 0x9B:
    {
        SP = A & X;
        uint8_t r = A & X & (uint8_t)((eff_addr >> 8) + 1);
        mem_write(eff_addr, r);
        break;
    }

    default:
        break;
    }

    // ---- Update cycles ----
    cycles += op.cycles + extra;

    // ---- Write log line ----
    if (log_file)
    {
        uint64_t ppu_cycle    = log_cyc * 3;
        int      ppu_scanline = (int)(ppu_cycle / 341) % 262;
        int      ppu_dot      = (int)(ppu_cycle % 341);

        fprintf(log_file,
            "%s  A:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3i,%3i CYC:%llu\n",
            prefix,
            log_a, log_x, log_y, log_p, log_sp,
            ppu_scanline, ppu_dot,
            (unsigned long long)log_cyc);
    }
}

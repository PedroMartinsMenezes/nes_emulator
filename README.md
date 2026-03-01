# An NES emulator written in C++

This is a NES emulator I wrote to learn about game emulation. This is a working in progress.

## Current Status

See bBelow the features implemented.

```
NES Emulator
├── CPU (Ricoh 2A03 – 6502-based)
│   ├── Core Execution Model
│   │   ├── Cycle-accurate clock() execution
│   │   ├── Instruction-boundary detection
│   │   ├── Proper cycle countdown per opcode
│   │   └── Page-cross cycle penalties
│   │
│   ├── Instruction Set
│   │   ├── All 151 official opcodes
│   │   ├── Full illegal opcode set
│   │   │   ├── SLO, RLA, SRE, RRA
│   │   │   ├── DCP, ISC
│   │   │   ├── LAX, SAX
│   │   │   ├── ANC, ALR, ARR
│   │   │   └── KIL (jam)
│   │   ├── Correct ADC/SBC overflow logic
│   │   ├── Correct flag behavior (N,V,Z,C)
│   │   └── RMW instruction behavior
│   │
│   ├── Addressing Modes
│   │   ├── Immediate, Zero Page
│   │   ├── Absolute, Indexed
│   │   ├── Indirect, IZX, IZY
│   │   ├── Relative (branch)
│   │   ├── Page crossing detection
│   │   └── JMP indirect page-wrap bug
│   │
│   ├── Interrupt System
│   │   ├── Edge-triggered NMI
│   │   ├── Level-triggered IRQ
│   │   ├── Instruction-boundary polling
│   │   ├── Correct interrupt priority (NMI > IRQ > BRK)
│   │   └── Proper stack frame pushes
│   │
│   ├── DMA Interaction
│   │   ├── CPU stall during OAM DMA
│   │   ├── 513/514 cycle behavior
│   │   └── Even/odd cycle alignment handling
│   │
│   ├── Open Bus
│   │   └── CPU data bus latch behavior
│   │
│   └── Debug Support
│       ├── Disassembly hook
│       └── nestest logging structure
│
├── PPU (Ricoh 2C02 – NTSC)
│   ├── Timing Model
│   │   ├── 262 scanlines
│   │   ├── 341 cycles per scanline
│   │   ├── Pre-render scanline (261)
│   │   ├── Visible (0–239)
│   │   ├── Post-render (240)
│   │   ├── VBlank (241–260)
│   │   └── Odd frame cycle skip
│   │
│   ├── VBlank & NMI
│   │   ├── VBlank set at 241, cycle 1
│   │   ├── VBlank clear at pre-render, cycle 1
│   │   ├── $2002 read clears VBlank
│   │   ├── NMI edge detection
│   │   └── $2000 NMI-enable race handling
│   │
│   ├── Registers
│   │   ├── PPUCTRL
│   │   ├── PPUMASK
│   │   ├── PPUSTATUS
│   │   ├── OAMADDR / OAMDATA
│   │   ├── PPUSCROLL (write toggle logic)
│   │   ├── PPUADDR (write toggle logic)
│   │   └── PPUDATA (buffered read behavior)
│   │
│   ├── VRAM Addressing Logic
│   │   ├── v (current VRAM address)
│   │   ├── t (temporary VRAM address)
│   │   ├── x (fine X scroll)
│   │   ├── w write toggle
│   │   ├── Coarse X increment
│   │   ├── Fine Y increment (cycle 256)
│   │   ├── Horizontal copy (cycle 257)
│   │   └── Vertical copy (cycles 280–304)
│   │
│   ├── Background Pipeline
│   │   ├── Name table fetch
│   │   ├── Attribute fetch
│   │   ├── Pattern low/high fetch
│   │   ├── 16-bit pattern shifters
│   │   ├── 16-bit attribute shifters
│   │   └── Tile loading every 8 cycles
│   │
│   ├── Sprite System
│   │   ├── Primary OAM (256 bytes)
│   │   ├── Secondary OAM (32 bytes)
│   │   ├── Sprite evaluation window (cycles 65–256)
│   │   ├── Sprite overflow flag behavior
│   │   ├── Sprite pattern fetch (257–320)
│   │   ├── Sprite shifters
│   │   ├── Sprite priority logic
│   │   └── Sprite 0 hit timing
│   │
│   ├── Pixel Combiner
│   │   ├── Background pixel extraction
│   │   ├── Sprite pixel extraction
│   │   ├── Transparent pixel rules
│   │   ├── Sprite priority resolution
│   │   ├── Left-edge masking ($2001 bits 1 & 2)
│   │   └── Final palette lookup
│   │
│   ├── Palette System
│   │   ├── $3F00–$3F1F memory
│   │   ├── Palette mirroring rules (10→00, 14→04, etc.)
│   │   └── Background vs sprite palette offsets
│   │
│   ├── DMA Interaction
│   │   ├── OAM writes via DMA
│   │   ├── OAM corruption window simulation
│   │   └── CPU stall synchronization
│   │
│   └── Mapper Hook
│       └── Scanline notification stub (MMC3-ready)
│
├── Memory
│   ├── CPU RAM
│   │   ├── 2 KB internal RAM
│   │   └── Mirrored across $0000–$1FFF
│   │
│   ├── PPU VRAM
│   │   ├── 2 KB internal VRAM
│   │   ├── Name table region
│   │   ├── Attribute tables
│   │   └── Mirroring support via mapper
│   │
│   ├── Palette RAM
│   │   ├── 32 bytes
│   │   └── Hardware mirroring behavior
│   │
│   ├── OAM Memory
│   │   ├── 256-byte primary OAM
│   │   └── 32-byte secondary OAM
│   │
│   └── Open Bus Behavior
│       ├── CPU data bus latch
│       └── PPU internal data latch
│
├── APU (Audio Processing Unit)
│   ├── Frame Counter Timing
│   │   ├── ~29830-cycle IRQ timing (NTSC)
│   │   ├── IRQ inhibit handling
│   │   └── CPU IRQ request
│   │
│   ├── DMC Stub
│   │   ├── CPU cycle stealing hook
│   │   └── DMA placeholder logic
│   │
│   └── IRQ Integration
│       └── Proper CPU priority handling
│
├── BUS
│   ├── Component Wiring
│   │   ├── CPU connected
│   │   ├── PPU connected
│   │   ├── APU connected
│   │   └── Cartridge connected
│   │
│   ├── Address Decoding
│   │   ├── $0000–$1FFF → CPU RAM (mirrored)
│   │   ├── $2000–$3FFF → PPU registers (mirrored)
│   │   ├── $4000–$4017 → APU / IO region
│   │   └── Cartridge passthrough
│   │
│   ├── OAM DMA Controller
│   │   ├── 513/514 cycle precision
│   │   ├── Even/odd alignment logic
│   │   ├── CPU stall control
│   │   └── PPU OAM writes
│   │
│   └── Open Bus
│       └── Last-read data retention
│
└── Mappers
    ├── IMapper interface abstraction
    ├── Mapper 0 (NROM)
    │   ├── PRG ROM mapping
    │   └── CHR ROM mapping
    │
    ├── Mapper 1 (MMC1)
    │   ├── Shift register logic
    │   ├── PRG bank switching
    │   └── CHR bank switching
    │
    └── IRQ-Ready Structure
        └── Scanline hook for MMC3-style IRQ
```
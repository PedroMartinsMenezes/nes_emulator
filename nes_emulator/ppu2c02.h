#pragma once
#include <cstdint>


//Picture Processing Unit
class PPU2C02 {
public:
    PPU2C02();

    void reset();
    uint8_t cpuRead(uint16_t addr, bool readOnly = false);
    void    cpuWrite(uint16_t addr, uint8_t data);
    void    clock();
    uint8_t ppuRead(uint16_t addr);
    void    ppuWrite(uint16_t addr, uint8_t data);
    void    setCpuDataBus(uint8_t data);

    uint16_t cycle      = 0;        // dot (0–340)
    uint16_t scanline   = 0;        // 0–261
    uint64_t frame      = 0;        // current frame

    bool nmi            = false;    // Non-Maskable Interrupt

    // Registers
    uint8_t PPUCTRL     = 0x00;     // $2000    
    uint8_t PPUMASK     = 0x00;     // $2001
    uint8_t PPUSTATUS   = 0x00;     // $2002   PPU Status Flags
    uint8_t OAMADDR     = 0x00;     // $2003   Object Attribute Memory Address

    uint16_t vram_addr  = 0;        // Current VRAM address
    uint16_t tram_addr  = 0;        // Temporary VRAM address
    uint8_t fine_x      = 0;        // Fine X scroll
    bool write_latch    = false;    // Toggle for $2005 / $2006

    uint8_t data_buffer = 0;        // PPU read buffer (used by $2007)

    // Pattern Tables
    // Each tile is 8x8 pixels of 2bits = 16 bytes
    // +-----------------+---------------+
    // | Table           | Address Range |
    // +-----------------+---------------+
    // | Pattern Table 0 | $0000 – $1000 | 16 x 16 = 256 tiles of 16 bytes = 4096 bytes
    // | Pattern Table 1 | $1000 – $2000 | 16 x 16 = 256 tiles of 16 bytes = 4096 bytes
    // +-----------------+---------------+
    uint8_t tblPattern[2][4096];    
    
    // Name Tables (Tile Maps)
    // Each table contains 32 x 30 = 960 tile indices + 64 attribute bytes = 1024 bytes
    // +--------------+---------------+
    // | Table        | Address       |
    // +--------------+---------------+
    // | Name Table 0 | $2000 – $2400 | 32 x 30 tiles
    // | Name Table 1 | $2400 – $2800 | 32 x 30 tiles
    // | Name Table 2 | $2800 – $2C00 | Mirror of Name Table 0
    // | Name Table 3 | $2C00 – $3000 | Mirror of Name Table 1
    // +--------------+---------------+
    uint8_t tblName[2][1024];
    
    // Palette RAM
    // +---------------+---------------------+
    // | Address       | Meaning             |
    // +---------------+---------------------+
    // | $3F00 – $3F10 | Background palettes | 16 bytes
    // | $3F10 – $3F20 | Sprite palettes     | 16 bytes
    // 
    // +---------------+---------------------+
    uint8_t tblPalette[32];
    
    // Primary Object Attribute Memory (Sprite RAM)
    // Contains 64 sprites of 4 bytes each
    // +------+------------+
    // | Byte | Meaning    |
    // +------+------------+
    // | 0    | Y position |
    // | 1    | Tile index |
    // | 2    | Attributes |
    // | 3    | X position |
    // +------+------------+
    uint8_t oam[256];               

    // Secondary OAM (Sprite Scanline Buffer)
    // Contains up to 8 sprites of 4 bytes each
    uint8_t secondary_oam[32];

    // Sprite Shift Registers (Pixel Pipeline)
    // The pixel shift registers for 8 active sprites.
    uint8_t sprite_shifter_lo[8];
    uint8_t sprite_shifter_hi[8];

    // Sprite X Counters (Horizontal Delay)
    // Delay counters before sprite pixels appear. Sprites don't always appear immediately.
    // Example = 20. So the PPU waits 20 cycles before rendering its pixels.
    uint8_t sprite_x_counter[8];
    
    // Sprite Attributes
    // Cached sprite attributes for active scanline sprites.
    // +-----+---------------------------------+
    // | Bit | Meaning                         |
    // +-----+---------------------------------+
    // | 7   | Vertical flip                   |
    // | 6   | Horizontal flip                 |
    // | 5   | Priority (in front / behind bg) |
    // | 1–0 | Palette select                  | 
    // +-----+---------------------------------+
    uint8_t sprite_attr[8];


    uint8_t cpuDataBus = 0x00;

    enum class PPU_Status : uint8_t {
        SpriteOverflow  = 0x20,
        SpriteZero      = 0x40,
        VBlank          = 0x80
    };
};
#pragma once

#include <cstdint>
#include <memory>

#include "apu2a03.h"
#include "bus.h"
#include "cartridge.h"
#include "cpu6502.h"
#include "ppu2c02.h"

class NES
{
  public:
    NES();
    ~NES();

  public:
    bool insertCartridge(const std::shared_ptr<Cartridge>& cart);
    void reset();

    // Main execution loop (Nintendulator style)
    void runFrame(); // Run until one frame completes
    void run();      // Continuous execution

    // Called once per CPU cycle (from CPU::RunCycle)
    void clockCPU();

  public:
    CPU6502  cpu;
    PPU2C02  ppu;
    APU2A03  apu;
    Bus      bus;

    // command line arguments
    uint16_t last_pc = 0;
    int      max_logs = 1000;
    bool     detailed_log = false;


  private:
    std::shared_ptr<Cartridge> cartridge;

    bool running = false;
};

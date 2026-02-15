#include "nes.h"
#include <filesystem>
#include <string>
#include <iostream>
namespace fs = std::filesystem;


NES::NES(const std::string& romPath)
:
    cart(romPath)
{
    bus.cpu = &cpu;
    bus.ppu = &ppu;
    bus.cart = &cart;
    cpu.connectBus(&bus);
    ppu.connectBus(&bus);
    this->romPath = romPath;
    open_log(romPath);
}

NES::~NES() 
{
    log.close();
}

void NES::open_log(const std::string& romPath) 
{
    fs::path rom_path = romPath;
    fs::path rom_dir = rom_path.parent_path();
    std::string log_path = (rom_dir / "nes_emulator.log").string();

    log.open(log_path);

    if (!log) 
    {
        throw std::runtime_error("Failed to open log file");
    }
}

void NES::reset() 
{
    bus.reset();
    //cart.reset();
    ppu.reset();
    apu.reset();
    cpu.reset();
}

void NES::clock()
{
    // 1. Clock PPU every master cycle
    ppu.clock();

    // -------------------------------------------------
    // 2. Detect NMI edge (PPU -> CPU)
    // -------------------------------------------------

    static bool prevNMILine = false;

    bool currentNMILine = ppu.nmiLine;

    // Rising edge detection
    if (currentNMILine && !prevNMILine)
    {
        cpu.requestNMI();   // latch request (do NOT execute immediately)
    }

    prevNMILine = currentNMILine;

    // -------------------------------------------------
    // 3. CPU runs every 3 PPU cycles
    // -------------------------------------------------

    if (bus.systemClockCounter % 3 == 0)
    {
        if (bus.dmaActive)
        {
            bus.clockDMA();
        }
        else
        {
            cpu.clock(log, romPath);
        }
    }

    bus.systemClockCounter++;
}

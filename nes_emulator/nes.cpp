#include "nes.h"
#include <filesystem>
#include <string>
#include <iostream>
namespace fs = std::filesystem;

NES::NES(const std::string& romPath) : cart(romPath) {
    bus.cpu = &cpu;
    bus.ppu = &ppu;
    bus.cart = &cart;
    cpu.connectBus(&bus);
    open_log(romPath);
}

NES::~NES() {
    log.close();
}

void NES::open_log(const std::string& romPath) {
    fs::path rom_path = romPath;
    fs::path rom_dir = rom_path.parent_path();
    std::string log_path = (rom_dir / "nes_emulator.log").string();

    log.open(log_path);

    if (!log) {
        throw std::runtime_error("Failed to open log file");
    }
}

void NES::reset() {
    // Reset system clock counter (optional but recommended)
    systemClockCounter = 0;
    // Reset all components
    cart.reset();
    ppu.reset();
    apu.reset();
    cpu.reset();
}

void NES::clock()
{
    ppu.clock();
    if (systemClockCounter % 3 == 0) 
    {
        if (bus.dmaActive)
        {
            bus.clockDMA();
        }
        else
        {
            cpu.clock();
            if (cpu.complete())
            {
                if (ppu.nmi)
                {
                    ppu.nmi = false;
                    cpu.nmi();
                }
                cpu.logState(log);
            }
        }
        apu.clock();
    }
    systemClockCounter++;
}


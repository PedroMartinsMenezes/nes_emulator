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
    //cart.reset();
    ppu.reset();
    apu.reset();
    cpu.reset();
}

void NES::clock()
{
    if (bus.dmaActive)
    {
        bus.clockDMA();
    }
    else
    {
        #pragma region CPU Clock (cpu.clock()))
        if (cpu.cycles == 0)
        {
            #pragma region Checking the PPU cycles
            uint64_t cyc = cpu.totalCycles;
            int ppuX = (int)(cyc * 3) % 341;
            int ppuY = ((int)(cyc * 3) / 341) % 262;
            if (ppuX != ppu.cycle || ppuY != ppu.scanline)
            {
                exit(0);
            }
            #pragma  endregion

            cpu.logState(log, ppu.cpuDataBus, ppu.PPUSTATUS);

            cpu.opcode = cpu.read(cpu.PC++);

            auto& inst = cpu.lookup[cpu.opcode];

            cpu.totalCycles += inst.cycles;

            ppu.clocks(inst.cycles);

            bool extraCycles1 = (cpu.*inst.addrmode)() == 1;

            bool extraCycles2 = (cpu.*inst.operate)() == 1;

            uint8_t extraCycle = (extraCycles1 && extraCycles2) ? 1 : 0;

            cpu.cycles += extraCycle;
            cpu.totalCycles += cpu.cycles;

            ppu.clocks(cpu.cycles);

            if (cpu.cycles > 0)
                cpu.cycles--;
        }
        else
        {
            ppu.clock();
            ppu.clock();
            ppu.clock();

            cpu.totalCycles++;

            if (cpu.cycles > 0)
                cpu.cycles--;
        }
        #pragma endregion

        

        if (cpu.complete())
        {
            if (ppu.nmi)
            {
                ppu.nmi = false;
                cpu.nmi();
            }
        }
    }
    apu.clock();
}


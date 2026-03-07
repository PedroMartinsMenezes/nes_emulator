#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

#include "cartridge.h"
#include "nes.h"

namespace fs = std::filesystem;

int run_nestest(const char* romPath);

int main(int argc, char** argv)
{
    std::cout << "+--------------+\n";
    std::cout << "| NES Emulator |\n";
    std::cout << "+--------------+\n";

    if (argc < 2)
    {
        std::cout << "Usage: nes_emulator <rom.nes>\n";
        return 1;
    }

    std::string rom_path = argv[1];

    if (!fs::exists(rom_path))
    {
        std::cout << "ROM file not found.\n";
        return 1;
    }

    std::cout << "Loading: " << fs::path(rom_path).filename().string() << "\n";

    // Create NES core
    NES nes;

    // Load cartridge
    auto cart = std::make_shared<Cartridge>(rom_path);

    if (!cart->isValid())
    {
        std::cout << "Invalid or unsupported ROM.\n";
        return 1;
    }

    nes.insertCartridge(cart);
    nes.reset();

    std::cout << "Starting execution...\n";

    if (fs::path(rom_path).filename().string() == "nestest.nes")
    {
        //nes.bus.cpu->nestestMode = true;
        //return run_nestest(rom_path.c_str());
        nes.bus.cpu->PC = 0xC000;
    }

    std::ofstream log("C:/dev/nes_emulator/roms/nestest/nes_emulator.log");
    nes.cpu.setLogger(&log);

    // Nintendulator-style execution:
    // CPU drives timing internally.
    while (true)
    {
        nes.cpu.ExecOp();
    }

    log.close();

    return 0;
}

//int run_nestest(const char* romPath)
//{
//    Bus bus;
//    CPU6502 cpu;
//    PPU2C02 ppu;
//    APU2A03 apu;
//    auto cart = std::make_shared<Cartridge>(romPath);
//
//    bus.connectCPU(&cpu);
//    bus.connectPPU(&ppu);
//    bus.connectAPU(&apu);
//
//    cpu.connectBus(&bus);
//    apu.connectCPU(&cpu);
//
//    //cpu.setNES(NES);
//
//    bus.insertCartridge(cart);
//
//    cpu.reset();
//    ppu.reset();
//    apu.reset();
//
//    // Disable DMA & interrupts for nestest
//    //bus.dmaActive = false;
//
//    // Force nestest start address
//    cpu.PC = 0xC000;
//
//    std::ofstream log("nestest.log");
//
//    while (true)
//    {
//        if (cpu.instructionComplete())
//        {
//            cpu.logState(log, cpu.PC);
//
//            if (cpu.PC == 0xC66E)
//                break;
//        }
//
//        cpu.clock();
//    }
//
//    log.close();
//
//    std::cout << "nestest finished successfully\n";
//    return 0;
//}

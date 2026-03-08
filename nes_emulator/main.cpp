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

    // specific PC for 'nestest.nes'
    if (fs::path(rom_path).filename().string() == "nestest.nes")
    {
        nes.bus.cpu->PC = 0xC000;
        nes.last_pc = 0xC66E;
    }

    // @@@ remove this !
    std::ofstream log("C:/dev/nes_emulator/roms/nestest/nes_emulator.log");

    nes.cpu.setLogger(&log);

    // Nintendulator-style execution. CPU drives timing internally.
    while (true)
    {
        nes.cpu.ExecOp();
    }

    log.close();

    return 0;
}

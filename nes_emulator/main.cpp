#include <iostream>
#include <memory>
#include <filesystem>

#include "nes.h"
#include "cartridge.h"

namespace fs = std::filesystem;

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

    // Nintendulator-style execution:
    // CPU drives timing internally.
    while (true)
    {
        nes.cpu.ExecOp();
    }

    return 0;
}
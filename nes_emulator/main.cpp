#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

#include "cartridge.h"
#include "nes.h"

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

    std::string file_name = fs::path(rom_path).filename().string();

    std::string log_name = fs::path(rom_path).stem().string() + ".log";

    std::cout << "Loading: " << file_name << "\n";

    NES nes;

    auto cart = std::make_shared<Cartridge>(rom_path);

    if (!cart->isValid())
    {
        std::cout << "Invalid or unsupported ROM.\n";
        return 1;
    }

    nes.insertCartridge(cart);
    nes.reset();

    std::cout << "Starting execution...\n";

    // specific settings for 'nestest.nes'
    if (file_name == "nestest.nes")
    {
        nes.bus.cpu->PC = 0xC000;
        nes.last_pc = 0xC66E;
    }
    else if (file_name == "ppu_vbl_nmi.nes")
    {
        nes.detailed_log = true;
    }

    std::ofstream log(log_name);

    nes.cpu.setLogger(&log);

    // Nintendulator-style execution. CPU drives timing internally.
    while (true)
    {
        nes.cpu.ExecOp();
    }

    log.close();

    return 0;
}

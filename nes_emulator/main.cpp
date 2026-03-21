#include <filesystem>
#include <iostream>
#include <memory>
#include "rom.h"
#include "bus.h"
#include "cpu.h"

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
        std::cout << "ROM not found: " << rom_path << "\n";
        return 1;
    }

    // Load ROM
    auto rom = std::make_unique<ROM>();
    if (!rom->load(rom_path))
        return 1;

    // Build system
    auto bus = std::make_unique<Bus>();
    bus->attach_rom(rom.get());

    auto cpu = std::make_unique<CPU>();
    cpu->attach_bus(bus.get());

    // Open log file in the same folder as the ROM
    std::string log_path = (fs::path(rom_path).parent_path() / (fs::path(rom_path).stem().string() + ".log")).string();
    FILE* log_file = fopen(log_path.c_str(), "w");
    if (!log_file)
    {
        std::cerr << "Cannot open log: " << log_path << "\n";
        return 1;
    }
    cpu->log_file = log_file;

    // nestest automation: reset then override PC to $C000
    cpu->reset();
    cpu->set_pc(0xC000);

    std::cout << "Running nestest (PC=$C000)...\n\n";

    // Run until PC loops to itself (infinite loop = test done)
    // or until cycle limit
    const uint64_t MAX_CYCLES = 500'000;
    uint16_t prev_pc = 0xFFFF;

    int iterations = 0;
    int last_pc = -1;

    if (fs::path(rom_path).stem().string() == "nestest")
    {
        last_pc = 0xC66E;
    }

    while (true)
    {
        iterations++;
        uint16_t pc = cpu->PC;
        cpu->step();

        if (cpu->PC == pc)
        {
            std::cout << "Infinite loop detected at PC=$" << std::hex << pc << ". Stopping execution.\n";
            break;
        }
        if (pc == last_pc)
        {            
            std::cout << "Reached final PC=$" << std::hex << pc << ". Stopping execution.\n";
             break;
        }
        if (cpu->cycles >= MAX_CYCLES)
        {
            std::cout << "Cycle limit reached (" << MAX_CYCLES << "). Stopping execution.\n";
            break;
        }
    }

    fclose(log_file);

    std::cout << std::dec;

    std::cout << "Iterations: " << iterations << "\n";    
    std::cout << "Total cycles: " << cpu->cycles << "\n";
    std::cout << "Log: " << log_path << "\n";

    #pragma region nestest specific
    // Read result codes written by nestest
    uint8_t res_official   = bus->ram_peek(0x0002);
    uint8_t res_unofficial = bus->ram_peek(0x0003);
    std::cout << "\nResult $02 (official):   " << (res_official == 0 ? "PASS" : "FAIL") << " (code=" << (int)res_official << ")\n";
    std::cout << "Result $03 (unofficial): " << (res_unofficial == 0 ? "PASS" : "FAIL") << " (code=" << (int)res_unofficial << ")\n";
    #pragma endregion

    return (res_official == 0 && res_unofficial == 0) ? 0 : 1;
}

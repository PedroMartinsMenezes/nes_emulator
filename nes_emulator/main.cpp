#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

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

    return 0;
}

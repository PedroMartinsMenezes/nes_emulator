#include <iostream>
#include <cstdint>


int main(int argc, char* argv[])
{
    std::cout << "+--------------+\n";
    std::cout << "| NES Emulator |\n";
    std::cout << "+--------------+\n";

    if (argc < 2)
    {
        std::cout << "[error] Missing ROM file\n";
        return 1;
    }

    std::cout << "File: " << argv[1] << "\n";
    std::cout << "Checking the emulator with 'nestest.nes' ROM\n";

    return 0;
}

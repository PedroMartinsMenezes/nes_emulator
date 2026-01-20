#include <iostream>
#include <cstdint>

#include "cpu6502.h"
#include "bus.h"

Bus bus;
CPU6502 processor;

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

    bus.nestestMode = true;
    processor.nestestMode = true;

    processor.reset();
    processor.connectBus(&bus);

    return 0;
}

#include <iostream>
#include <fstream>

#include "bus.h"
#include "cpu6502.h"
#include "cartridge.h"
// #include "ppu2c02.h"   // stub or real later

int main(int argc, char** argv) {

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

    // Configuration
    const bool runNestest = true;
    const char* romPath = argv[1];

    // Create core components
    Bus bus;
    CPU6502 cpu;
    Cartridge cart(romPath);

    bus.cpu = &cpu;
    bus.cart = &cart;
    cpu.connectBus(&bus);

    bus.nestestMode = runNestest;
    cpu.nestestMode = runNestest;

    // Optional PPU (stub OK for nestest)
    // PPU2C02 ppu;
    // bus.ppu = &ppu;

    // Reset CPU
    cpu.reset();

    if (runNestest) {
        cpu.PC = 0xC000; // nestest entry point
    }

    // Open trace log
    std::ofstream log;
    if (runNestest) {
        log.open("my_nestest.log");
        if (!log) {
            std::cerr << "Failed to open log file\n";
            return 1;
        }
    }

    // Main execution loop
    bool running = true;

    while (running) {

        if (runNestest && cpu.complete()) {
            cpu.logState(log);
        }

        cpu.clock();

        if (runNestest && cpu.PC == 0xC66E) {
            running = false;
        }

        if (cpu.totalCycles > 100000) //@@@ remove this
            break;
    }

    // Cleanup
    if (runNestest) {
        log.close();
        std::cout << "nestest finished. Log written to my_nestest.log\n";
    }

    return 0;
}

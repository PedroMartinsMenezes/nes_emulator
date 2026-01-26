#include <iostream>
#include <fstream>

#include "bus.h"
#include "cpu6502.h"
#include "cartridge.h"
#include "nes.h"

int main(int argc, char** argv) {

    // Configuration
    const bool runNestest = true;

    std::cout << "+--------------+\n";
    std::cout << "| NES Emulator |\n";
    std::cout << "+--------------+\n";

    if (argc < 2)
    {
        std::cout << "[error] Missing ROM file\n";
        return 1;
    }
    if (runNestest) {
    
        std::cout << "File: " << argv[1] << "\n";
        std::cout << "Checking the emulator with 'nestest.nes' ROM\n";
    
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

            if (runNestest && cpu.PC == 0xC66E && cpu.complete()) {
                running = false;
            }

            if (runNestest && cpu.complete()) {
                cpu.logState(log);
            }

            cpu.clock();
        }

        // Cleanup
        if (runNestest) {
            log.close();
            std::cout << "nestest finished. Log written to my_nestest.log\n";
        }
    }
    else {
        NES nes(argv[1]);
        nes.reset();
        bool running = true;
        while (running) {
            nes.clock();
        }
    }

    return 0;
}

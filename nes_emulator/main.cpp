#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

#include "bus.h"
#include "cpu6502.h"
#include "cartridge.h"
#include "nes.h"


namespace fs = std::filesystem;

int run_nestest(const char* romPath);
int run_ppu_vbl_nmi(const char* romPath);


int main(int argc, char** argv) {

    // Configuration
    bool is_nestest = false;
    bool is_ppu_vbl_nmi = false;

    std::cout << "+--------------+\n";
    std::cout << "| NES Emulator |\n";
    std::cout << "+--------------+\n";

    if (argc < 2)
    {
        std::cout << "Usage: nes_emulator 'rom.nest'[error] Missing ROM file\n";
        return 1;
    }

    std::string rom_path = argv[1];
    std::string rom_name = fs::path(rom_path).filename().string();

    std::cout << "Running the file: " << rom_name << "\n";

    is_nestest = rom_path.find("nestest.nes") != std::string::npos;
    is_ppu_vbl_nmi = rom_path.find("ppu_vbl_nmi.nes") != std::string::npos;

    std::cout << "File: " << argv[1] << "\n";

    if (is_nestest) {
        return run_nestest(argv[1]);
    }

    if (is_ppu_vbl_nmi) {
        return run_ppu_vbl_nmi(argv[1]);
    }

    NES nes(argv[1]);
    nes.reset();
    bool running = true;
    while (running) {
        nes.clock();
    }

    return 0;
}

int run_nestest(const char* romPath) {

    fs::path rom_path = romPath;
    fs::path rom_dir = rom_path.parent_path();

    // Create core components
    Bus bus;
    CPU6502 cpu;
    Cartridge cart(rom_path.string());

    bus.cpu = &cpu;
    bus.cart = &cart;
    cpu.connectBus(&bus);

    bus.nestestMode = true;
    cpu.nestestMode = true;

    // Optional PPU (stub OK for nestest)
    // PPU2C02 ppu;
    // bus.ppu = &ppu;

    // Reset CPU
    cpu.reset();

    //fixed nestest entry point
    cpu.PC = 0xC000;

    // Open trace log
    std::ofstream log;

    std::string log_path = (rom_dir / "nes_emulator.log").string();

    std::cout << "Log : " << log_path << "\n";

    log.open(log_path);
    if (!log) {
        std::cerr << "Failed to open log file\n";
        return 1;
    }

    // Main execution loop
    bool running = true;

    while (running) {
        // check fixed nestest exit point
        if (cpu.PC == 0xC66E && cpu.complete()) {
            running = false;
        }
        if (cpu.complete()) {
            cpu.logState(log);
        }
        cpu.clock();
    }

    // Cleanup
    log.close();
    
    std::cout << "Finished !\n";
    return 0;
}

int run_ppu_vbl_nmi(const char* romPath) {

    NES nes(romPath);
    nes.reset();

    while (true)
    {
        nes.clock();

        // reading fixed address for test status
        uint8_t result = nes.bus.cpuRead(0x0000);
        if (result == 0x01)
        {
            std::cout << "PASS\n";
            return 0;
        }
        if (result == 0xFF)
        {
            std::cout << "FAIL\n";
            return 0;
        }
    }

    std::cout << "INCONCLUSIVE\n";
    return 0;
}

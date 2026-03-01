#pragma once

#include <cstdint>
#include <memory>

class CPU6502;
class PPU2C02;
class APU2A03;
class Cartridge;

class Bus
{
public:
    Bus();

    // Connections
    void connectCPU(CPU6502* c);
    void connectPPU(PPU2C02* p);
    void connectAPU(APU2A03* a);

    void insertCartridge(const std::shared_ptr<Cartridge>& cart);

    // CPU interface
    uint8_t cpuRead(uint16_t addr);
    void cpuWrite(uint16_t addr, uint8_t data);

    // DMA handling (called once per CPU cycle)
    void clockDMA();

public:
    bool dmaActive = false;
    uint8_t dmaPage = 0x00;
    uint8_t dmaAddr = 0x00;
    uint8_t dmaData = 0x00;

private:
    CPU6502* cpu = nullptr;
    PPU2C02* ppu = nullptr;
    APU2A03* apu = nullptr;

    std::shared_ptr<Cartridge> cartridge;

    uint8_t cpuRam[2048];
};
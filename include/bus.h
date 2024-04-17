#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include <cartridge.h>

class CPUMemoryBus
{
public:
    CPUMemoryBus();

    void connect_cartridge(std::shared_ptr<Cartridge> cartridge_);
    uint8_t read(const uint16_t address) const;
    void write(const uint16_t address, const uint8_t data);
private:
    std::array<uint8_t, 2048> ram;
    std::shared_ptr<Cartridge> cartridge;
};

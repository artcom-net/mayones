#include "bus.h"


CPUMemoryBus::CPUMemoryBus() :
    cartridge()
{
}

void CPUMemoryBus::connect_cartridge(std::shared_ptr<Cartridge> cartridge_)
{
    cartridge = cartridge_;
}

uint8_t CPUMemoryBus::read(uint16_t address) const
{
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        return ram[address & 0x07FF];
    }
    if (address >= 0x2000 && address <= 0x3FFF)
    {
        // PPU register
        return 0;
    }
    if (address >= 0x4000 && address <= 0x4017)
    {
        // APU and I/O registers
        return 0;
    }
    if (address >= 0x4018 && address <= 0x401F)
    {
        // APU and I/O functionality that is normally disabled
        return 0;
    }
    if (address >= 0x4020 && address <= 0xFFFF)
    {
        // # PRG ROM, PRG RAM and mapper registers
        return cartridge->read(address);
    }
    return 0;
}

void CPUMemoryBus::write(uint16_t address, const uint8_t data)
{
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        ram[address & 0x07FF] = data;
        return;
    }
    if (address >= 0x2000 && address <= 0x3FFF)
    {
        // PPU register
        return;
    }
    if (address == 0x4014)
    {
        // DMA
        return;
    }
    if (address >= 0x4000 && address <= 0x4017)
    {
        // APU and I/O registers
        return;
    }
    if (address >= 0x4018 && address <= 0x401F)
    {
        // APU and I/O functionality that is normally disabled
        return;
    }
    if (address >= 0x4020 && address <= 0xFFFF)
    {
        // # PRG ROM, PRG RAM and mapper registers
        cartridge->write(address, data);
    }
}

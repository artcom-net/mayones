#include <algorithm>

#include "mapper.h"


Mapper0::Mapper0()
{
}

Mapper0::Mapper0(std::vector<uint8_t>&& prg_rom, std::vector<uint8_t>&& chr_rom) :
    prg_rom(std::move(prg_rom)),
    chr_rom(std::move(chr_rom))
{
}

Mapper0& Mapper0::operator=(Mapper0&& mapper)
{
    if (this == &mapper)
    {
        return *this;
    }
    std::swap(prg_rom, mapper.prg_rom);
    std::swap(chr_rom, mapper.chr_rom);
    return *this;
}

uint8_t Mapper0::read(uint16_t address) const
{
    if (address >= 0x8000 && address <= 0xFFFF)
    {
        return prg_rom[address & 0x3FFF];
    }
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        return chr_rom[address];
    }
}

void Mapper0::write(uint16_t address, uint8_t data)
{
}

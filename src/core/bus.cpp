#include <cstdint>
#include <variant>

#include "mayones/core/bus.hpp"
#include "mayones/core/mapper.hpp"

namespace mayones::core {

CpuBus::CpuBus(rom::Mapper& mapper) :
    ram_{},
    mapper_{ mapper }
{
}

std::uint8_t CpuBus::read(std::uint16_t address) const
{
    /* clang-format off
    https://www.nesdev.org/wiki/CPU_memory_map

    Address range	Size	Device
    $0000–$07FF	    $0800	2 KB internal RAM
    $0800–$0FFF	    $0800	Mirrors of $0000–$07FF
    $1000–$17FF	    $0800   Mirrors of $0000–$07FF
    $1800–$1FFF	    $0800   Mirrors of $0000–$07FF
    $2000–$2007	    $0008	NES PPU registers
    $2008–$3FFF	    $1FF8	Mirrors of $2000–$2007 (repeats every 8 bytes)
    $4000–$4017	    $0018	NES APU and I/O registers
    $4018–$401F	    $0008	APU and I/O functionality that is normally disabled. See CPU Test Mode.
    $4020–$FFFF     $BFE0   Unmapped. Available for cartridge use.
    • $6000–$7FFF   $2000   Usually cartridge RAM, when present.
    • $8000–$FFFF	$8000   Usually cartridge ROM and mapper registers.
    clang-format on */

    if (address < 0x2000)
    {
        return ram_[address & RAM_MASK];
    }
    if (address < 0x4000)
    {
        // PPU
        return 0x80;
    }
    if (address < 0x4020)
    {
        // APU and IO
        return 0;
    }
    // Mapper
    return std::visit(
      [address](const auto& mapper) -> std::uint8_t { return mapper.read_prg(address); }, mapper_);
}

void CpuBus::write(std::uint16_t address, std::uint8_t data)
{
    if (address < 0x2000)
    {
        ram_[address & RAM_MASK] = data;
    }
    if (address < 0x4000)
    {
        // PPU
    }
    if (address < 0x4020)
    {
        // APU and IO
    }
    // Mapper
    std::visit([address, data](auto& mapper) { return mapper.write_prg(address, data); }, mapper_);
}

} // namespace mayones::core

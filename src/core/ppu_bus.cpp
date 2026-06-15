#include <cstdint>
#include <variant>

#include "mayones/core/mapper.hpp"
#include "mayones/core/ppu_bus.hpp"

namespace mayones::core {

PpuBus::PpuBus(rom::Mapper& mapper) :
    mapper_{ mapper }
{
}

std::uint8_t PpuBus::read(std::uint16_t address) const
{
    if (address < 0x2000)
    {
        return std::visit([address](auto& mapper) { return mapper.read_chr(address); }, mapper_);
    }
    auto mirrored_address = std::visit(
      [address](auto& mapper) { return mapper.mirror_nametable_address(address); }, mapper_);

    return vram_[mirrored_address];
}

void PpuBus::write(std::uint16_t address, std::uint8_t data)
{
    if (address < 0x2000)
    {
        std::visit([address, data](auto& mapper) { return mapper.write_chr(address, data); },
                   mapper_);
    }
    else
    {
        auto mirrored_address = std::visit(
          [address](auto& mapper) { return mapper.mirror_nametable_address(address); }, mapper_);

        vram_[mirrored_address] = data;
    }
}

} // namespace mayones::core

#include <utility>

#include "mayones/core/cartridge.hpp"
#include "mayones/core/mapper.hpp"
#include "mayones/core/rom.hpp"

namespace mayones::core {

Cartridge::Cartridge(rom::RomInfo rom_info, rom::Mapper mapper) :
    m_mapper{ std::move(mapper) },
    m_rom_info{ rom_info }
{
}

const rom::RomInfo& Cartridge::rom_info() const noexcept
{
    return m_rom_info;
}

} // namespace mayones::core

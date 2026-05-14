#include <cstdint>
#include <expected>
#include <string>
#include <utility>
#include <vector>

#include "mayones/core/cartridge.hpp"
#include "mayones/core/core.hpp"
#include "mayones/core/mapper.hpp"
#include "mayones/core/rom_loader.hpp"

namespace mayones::core {

NesCore::NesCore() :
    m_cartridge{ Cartridge{ {}, rom::DummyMapper{} } }
{
}

std::expected<void, std::string> NesCore::load_rom(std::vector<std::uint8_t> rom_data)
{
    auto load_result = rom::load_rom(std::move(rom_data));
    if (!load_result)
    {
        return std::unexpected{ std::move(load_result).error() };
    }

    m_cartridge = std::move(load_result).value();

    return {};
}

const rom::RomInfo& NesCore::rom_info() const noexcept
{
    return m_cartridge.rom_info();
}

} // namespace mayones::core

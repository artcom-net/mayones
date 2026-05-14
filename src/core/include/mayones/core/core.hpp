#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

#include "mayones/core/cartridge.hpp"
#include "mayones/core/rom.hpp"

namespace mayones::core {

class NesCore {
public:
    explicit NesCore();

    std::expected<void, std::string> load_rom(std::vector<std::uint8_t> rom_data);
    [[nodiscard]] const rom::RomInfo& rom_info() const noexcept;

private:
    Cartridge m_cartridge;
};

} // namespace mayones::core

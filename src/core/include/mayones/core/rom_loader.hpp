#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

#include "mayones/core/cartridge.hpp"

namespace mayones::core::rom {

std::expected<Cartridge, std::string> load_rom(std::vector<std::uint8_t> rom_data);

} // namespace mayones::core::rom

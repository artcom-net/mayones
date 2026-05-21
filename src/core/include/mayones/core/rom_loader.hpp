#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

#include "mayones/core/rom.hpp"

namespace mayones::core::rom {

std::expected<RomData, std::string> load_rom(std::vector<std::uint8_t> rom_data);

} // namespace mayones::core::rom

#pragma once

#include "mapper.hpp"
#include "mayones/core/rom.hpp"

namespace mayones::core {

class Cartridge {
public:
    explicit Cartridge(rom::RomInfo rom_info, rom::Mapper mapper);

    [[nodiscard]] const rom::RomInfo& rom_info() const noexcept;

private:
    rom::Mapper m_mapper;
    rom::RomInfo m_rom_info;
};

} // namespace mayones::core

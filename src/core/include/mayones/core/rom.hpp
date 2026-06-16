#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "mayones/core/const.hpp"

namespace mayones::core::rom {

inline constexpr std::size_t PRG_BANK_SIZE{ 16 * KB };
inline constexpr std::size_t CHR_BANK_SIZE{ 8 * KB };
inline constexpr std::size_t PRG_RAM_BANK_SIZE{ 8 * KB };

enum class RomFormat : std::uint8_t {
    INES,
    INES20,
    ARCHAIC_INES
};

enum class Mirroring : std::uint8_t {
    VERTICAL,
    HORIZONTAL
};

enum class ConsoleType : std::uint8_t {
    NES,
    VS_UNISYSTEM,
    PLAYCHOICE_10
};

enum class TVSystem : std::uint8_t {
    NTSC,
    PAL,
    DENDY,
    MULTIPLE_REGION
};

struct RomInfo {
    std::size_t prg_rom_size;
    std::size_t chr_rom_size;
    std::size_t prg_ram_size;
    std::size_t prg_nvram_size;
    std::size_t chr_ram_size;
    std::size_t chr_nvram_size;
    std::uint16_t mapper_id;
    std::uint8_t submapper_id;
    RomFormat rom_format;
    Mirroring mirroring;
    ConsoleType console_type;
    TVSystem tv_system;
    bool has_battery;
    bool has_trainer;
    bool has_alternate_nt_layout;
};

struct RomData {
    std::vector<std::uint8_t> prg_rom;
    std::vector<std::uint8_t> chr_rom;
    RomInfo rom_info;
};

std::string_view to_string(RomFormat rom_format);
std::string_view to_string(Mirroring mirroring);
std::string_view to_string(ConsoleType console_type);
std::string_view to_string(TVSystem tv_system);

} // namespace mayones::core::rom

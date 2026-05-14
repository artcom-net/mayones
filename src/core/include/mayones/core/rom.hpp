#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "mayones/core/const.hpp"

namespace mayones::core::rom {

inline constexpr std::size_t HEADER_ID_SIZE{ 4 };
inline constexpr std::size_t PADDING_SIZE{ 5 };
inline constexpr std::size_t TRAINER_SIZE{ 512 };
inline constexpr std::size_t PRG_BANK_SIZE{ 16 * KB };
inline constexpr std::size_t CHR_BANK_SIZE{ 8 * KB };
inline constexpr std::string_view HEADER_ID{ "NES\x1A" };

enum class RomFormat : std::uint8_t {
    UNKNOWN,
    INES,
    ARCHAIC_INES,
    NES20
};

enum class Mirroring : std::uint8_t {
    UNKNOWN,
    HORIZONTAL,
    VERTICAL
};

enum class ConsoleType : std::uint8_t {
    UNKNOWN,
    NES,
    VS_UNISYSTEM,
    PLAYCHOICE_10
};

enum class TVSystem : std::uint8_t {
    UNKNOWN,
    NTSC,
    PAL
};

struct RomInfo {
    std::array<std::uint8_t, HEADER_ID_SIZE> header_id;
    std::uint8_t prg_rom_banks;
    std::uint8_t chr_rom_banks;
    std::uint8_t prg_ram_banks;
    std::uint8_t mapper_id;
    std::uint8_t reserved_bits;
    std::array<uint8_t, PADDING_SIZE> padding;
    Mirroring mirroring;
    ConsoleType console_type;
    TVSystem tv_system;
    bool has_battery;
    bool has_trainer;
    bool has_alternate_nt_layout;
};

} // namespace mayones::core::rom

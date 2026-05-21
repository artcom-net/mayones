#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "mayones/core/const.hpp"

namespace mayones::core::rom {

inline constexpr std::size_t HEADER_ID_SIZE{ 4 };
inline constexpr std::size_t PADDING_SIZE{ 5 };
inline constexpr std::size_t TRAINER_SIZE{ 512 };
inline constexpr std::size_t PRG_BANK_SIZE{ 16 * KB };
inline constexpr std::size_t CHR_BANK_SIZE{ 8 * KB };
inline constexpr std::array<std::uint8_t, HEADER_ID_SIZE> HEADER_ID{ 'N', 'E', 'S', 0x1A };

enum class RomFormat : std::uint8_t {
    INES,
    ARCHAIC_INES,
    INES20
};

enum class Mirroring : std::uint8_t {
    HORIZONTAL,
    VERTICAL
};

enum class ConsoleType : std::uint8_t {
    NES,
    VS_UNISYSTEM,
    PLAYCHOICE_10
};

enum class TVSystem : std::uint8_t {
    NTSC,
    PAL
};

struct RomInfo {
    std::uint8_t prg_rom_banks;
    std::uint8_t chr_rom_banks;
    std::uint8_t prg_ram_banks;
    std::uint8_t mapper_id;
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

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef> // std::size_t
#include <cstdint> // std::uint*_t
#include <expected>
#include <ranges>
#include <span>
#include <string>
#include <utility> // std::move, std::unreachable
#include <vector>

#include "mayones/core/rom.hpp"
#include "mayones/core/rom_loader.hpp"

namespace {

using namespace mayones::core::rom;

struct RawHeader {
    std::array<std::uint8_t, HEADER_ID_SIZE> header_id;
    std::uint8_t prg_rom_banks;
    std::uint8_t chr_rom_banks;
    std::uint8_t flags6;
    std::uint8_t flags7;
    std::uint8_t flags8;
    std::uint8_t flags9;
    std::uint8_t flags10;
    std::array<uint8_t, PADDING_SIZE> padding;
};

constexpr std::size_t HEADER_SIZE{ sizeof(RawHeader) };

struct Flags6 {
    enum BitMask : std::uint8_t {
        MIRRORING = 1 << 0,
        HAS_BATTERY = 1 << 1,
        HAS_TRAINER = 1 << 2,
        HAS_ALTERNATE_NT_LAYOUT = 1 << 3,
        MAPPER_LOW_BITS = 0xF0
    };

    Mirroring mirroring;
    bool has_alternate_nt_layout;
    bool has_battery;
    bool has_trainer;
    std::uint8_t mapper_low_bits;
};

struct Flags7 {
    enum BitMask : std::uint8_t {
        VS_UNISYSTEM = 1 << 0,
        PLAYCHOICE_10 = 1 << 1,
        ROM_FORMAT_BITS = 0x0C,
        MAPPER_HIGH_BITS = 0xF0
    };

    RomFormat rom_format;
    ConsoleType console_type;
    std::uint8_t mapper_high_bits;
};

struct Flags9 {
    enum BitMask : std::uint8_t {
        TV_SYSTEM = 1 << 0,
        RESERVED_BITS = 0xFE
    };

    TVSystem tv_system;
    std::uint8_t reserved_bits;
};

std::expected<RomFormat, std::string> identify_rom_format(std::span<const std::uint8_t> rom_data)
{
    constexpr std::size_t FLAGS7_INDEX{ 7 };
    constexpr std::uint8_t INES_FORMAT_BITS{ 0x00 };
    constexpr std::uint8_t ARCHAIC_INES_FORMAT_BITS{ 0x04 };
    constexpr std::uint8_t NES20_FORMAT_BITS{ 0x08 };

    if (rom_data.size() <= FLAGS7_INDEX)
    {
        return std::unexpected{ "ROM is too small to identify format" };
    }

    switch (rom_data[FLAGS7_INDEX] & Flags7::ROM_FORMAT_BITS)
    {
        case INES_FORMAT_BITS:
            return RomFormat::INES;
        case ARCHAIC_INES_FORMAT_BITS:
            return RomFormat::ARCHAIC_INES;
        case NES20_FORMAT_BITS:
            return RomFormat::INES20;
        default:
            return std::unexpected{ "Unknown ROM format" };
    }
}

inline bool is_set_flags(std::uint8_t value, std::uint8_t flags) noexcept
{
    return (value & flags) != 0;
}

Flags6 parse_flags6(std::uint8_t flags) noexcept
{
    return Flags6{ .mirroring = is_set_flags(flags, Flags6::MIRRORING) ? Mirroring::VERTICAL
                                                                       : Mirroring::HORIZONTAL,
                   .has_alternate_nt_layout = is_set_flags(flags, Flags6::HAS_ALTERNATE_NT_LAYOUT),
                   .has_battery = is_set_flags(flags, Flags6::HAS_BATTERY),
                   .has_trainer = is_set_flags(flags, Flags6::HAS_TRAINER),
                   .mapper_low_bits =
                     static_cast<std::uint8_t>((flags & Flags6::MAPPER_LOW_BITS) >> 4) };
}

Flags7 parse_flags7(std::uint8_t flags) noexcept
{
    ConsoleType console_type{};
    if (is_set_flags(flags, Flags7::VS_UNISYSTEM))
    {
        console_type = ConsoleType::VS_UNISYSTEM;
    }
    else if (is_set_flags(flags, Flags7::PLAYCHOICE_10))
    {
        console_type = ConsoleType::PLAYCHOICE_10;
    }
    else
    {
        console_type = ConsoleType::NES;
    }
    return Flags7{ .rom_format = RomFormat::INES,
                   .console_type = console_type,
                   .mapper_high_bits =
                     static_cast<std::uint8_t>(flags & Flags7::MAPPER_HIGH_BITS) };
}

Flags9 parse_flags9(std::uint8_t flags)
{
    return Flags9{ .tv_system =
                     is_set_flags(flags, Flags9::TV_SYSTEM) ? TVSystem::PAL : TVSystem::NTSC,
                   .reserved_bits = static_cast<std::uint8_t>(flags & Flags9::RESERVED_BITS) };
}

std::expected<RomInfo, std::string> parse_header(std::span<std::uint8_t> rom_data)
{
    if (rom_data.size() < HEADER_SIZE)
    {
        return std::unexpected{ "ROM header is too small" };
    }

    std::array<std::uint8_t, HEADER_SIZE> header_bytes{};
    std::ranges::copy(rom_data.first<HEADER_SIZE>(), header_bytes.begin());
    auto raw_header = std::bit_cast<RawHeader>(header_bytes);

    if (raw_header.prg_rom_banks == 0)
    {
        return std::unexpected{ "Invalid PRG ROM bank count: 0 (at least 1 required)" };
    }
    if (raw_header.chr_rom_banks == 0)
    {
        return std::unexpected{ "Invalid CHR ROM bank count: 0 (at least 1 required)" };
    }
    if (!std::ranges::all_of(raw_header.padding, [](std::uint8_t v) { return v == 0; }))
    {
        return std::unexpected{ "Header padding contains non-zero bytes" };
    }

    Flags6 flags6 = parse_flags6(raw_header.flags6);
    Flags7 flags7 = parse_flags7(raw_header.flags7);

    Flags9 flags9 = parse_flags9(raw_header.flags9);
    if (flags9.reserved_bits != 0)
    {
        return std::unexpected{ "Reserved bits in flags9 are non-zero" };
    }
    // Skip Flags10: https://www.nesdev.org/wiki/INES#Flags_10

    return RomInfo{ .prg_rom_banks = raw_header.prg_rom_banks,
                    .chr_rom_banks = raw_header.chr_rom_banks,
                    .prg_ram_banks = raw_header.flags8,
                    .mapper_id =
                      static_cast<std::uint8_t>(flags7.mapper_high_bits | flags6.mapper_low_bits),
                    .rom_format = flags7.rom_format,
                    .mirroring = flags6.mirroring,
                    .console_type = flags7.console_type,
                    .tv_system = flags9.tv_system,
                    .has_battery = flags6.has_battery,
                    .has_trainer = flags6.has_trainer,
                    .has_alternate_nt_layout = flags6.has_alternate_nt_layout };
}

std::expected<RomData, std::string> parse_ines_rom(std::vector<std::uint8_t> rom_data)
{
    auto parse_result = parse_header(rom_data);
    if (!parse_result)
    {
        return std::unexpected{ std::move(parse_result).error() };
    }

    const RomInfo rom_info{ parse_result.value() };

    const std::size_t prg_rom_size = PRG_BANK_SIZE * rom_info.prg_rom_banks;
    const std::size_t chr_rom_size = CHR_BANK_SIZE * rom_info.chr_rom_banks;
    const std::size_t prg_rom_start_idx = HEADER_SIZE + (rom_info.has_trainer ? TRAINER_SIZE : 0);

    if ((prg_rom_start_idx + prg_rom_size + chr_rom_size) != rom_data.size())
    {
        return std::unexpected{ "ROM size mismatch" };
    }

    // clang-format off
    auto prg_rom = rom_data |
                   std::views::drop(prg_rom_start_idx) |
                   std::views::take(prg_rom_size) |
                   std::ranges::to<std::vector>();
    auto chr_rom = rom_data |
                   std::views::drop(prg_rom_start_idx + prg_rom_size) |
                   std::views::take(chr_rom_size) |
                   std::ranges::to<std::vector>();
    // clang-format on

    // TODO: call shrink_to_fit
    return RomData{ .prg_rom = std::move(prg_rom),
                    .chr_rom = std::move(chr_rom),
                    .rom_info = rom_info };
}

std::expected<void, std::string> validate_header_id(std::span<const std::uint8_t> rom_data)
{
    if (rom_data.size() < HEADER_ID_SIZE)
    {
        return std::unexpected{ "ROM is too small for header ID parsing" };
    }
    if (!std::ranges::equal(rom_data.first<HEADER_ID_SIZE>(), HEADER_ID))
    {
        return std::unexpected{ "Invalid iNES header ID" };
    }
    return {};
}

} // namespace

namespace mayones::core::rom {

std::expected<RomData, std::string> load_rom(std::vector<std::uint8_t> rom_data)
{
    if (auto validation_result = validate_header_id(rom_data); !validation_result)
    {
        return std::unexpected{ std::move(validation_result).error() };
    }

    auto id_rom_result = identify_rom_format(rom_data);
    if (!id_rom_result)
    {
        return std::unexpected{ std::move(id_rom_result).error() };
    }

    switch (id_rom_result.value())
    {
        case RomFormat::INES:
            return parse_ines_rom(std::move(rom_data));
        case RomFormat::ARCHAIC_INES:
            return std::unexpected{ "Archaic iNES format is not supported" };
        case RomFormat::INES20:
            return std::unexpected{ "NES 2.0 format is not supported" };
        default:
            std::unreachable();
    }
}

} // namespace mayones::core::rom

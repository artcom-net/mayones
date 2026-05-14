#include <algorithm>
#include <array>
#include <bit>
#include <cstddef> // std::size_t
#include <cstdint> // std::uint*_t
#include <expected>
#include <span>
#include <string>
#include <utility> // std::move, std::cmp_equal, std::unreachable
#include <vector>

#include "mayones/core/cartridge.hpp"
#include "mayones/core/mapper.hpp"
#include "mayones/core/rom.hpp"
#include "mayones/core/rom_loader.hpp"

namespace {

using namespace mayones::core::rom;

using ParseRomResult = std::expected<mayones::core::Cartridge, std::string>;

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

RomFormat identify_rom_format(std::span<const std::uint8_t> rom_data)
{
    constexpr std::size_t FLAGS7_INDEX{ 7 };
    constexpr std::uint8_t INES_FORMAT_BITS{ 0x00 };
    constexpr std::uint8_t ARCHAIC_INES_FORMAT_BITS{ 0x04 };
    constexpr std::uint8_t NES20_FORMAT_BITS{ 0x08 };

    if (rom_data.size() <= FLAGS7_INDEX)
    {
        return RomFormat::UNKNOWN;
    }

    switch (rom_data[FLAGS7_INDEX] & Flags7::ROM_FORMAT_BITS)
    {
        case INES_FORMAT_BITS:
            return RomFormat::INES;
        case ARCHAIC_INES_FORMAT_BITS:
            return RomFormat::ARCHAIC_INES;
        case NES20_FORMAT_BITS:
            return RomFormat::NES20;
        default:
            return RomFormat::UNKNOWN;
    }
}

inline bool is_set_flag(std::uint8_t value, std::uint8_t flags) noexcept
{
    return (value & flags) != 0;
}

Flags6 parse_flags6(std::uint8_t flags) noexcept
{
    Mirroring mirroring =
      is_set_flag(flags, Flags6::MIRRORING) ? Mirroring::VERTICAL : Mirroring::HORIZONTAL;
    bool has_alternate_nt_layout = is_set_flag(flags, Flags6::HAS_ALTERNATE_NT_LAYOUT);
    bool has_battery = is_set_flag(flags, Flags6::HAS_BATTERY);
    bool has_trainer = is_set_flag(flags, Flags6::HAS_TRAINER);
    std::uint8_t mapper_low_bits = (flags & Flags6::MAPPER_LOW_BITS) >> 4;
    return Flags6{ .mirroring = mirroring,
                   .has_alternate_nt_layout = has_alternate_nt_layout,
                   .has_battery = has_battery,
                   .has_trainer = has_trainer,
                   .mapper_low_bits = mapper_low_bits };
}

Flags7 parse_flags7(std::uint8_t flags) noexcept
{
    ConsoleType console_type{ ConsoleType::UNKNOWN };
    if (is_set_flag(flags, Flags7::VS_UNISYSTEM))
    {
        console_type = ConsoleType::VS_UNISYSTEM;
    }
    else if (is_set_flag(flags, Flags7::PLAYCHOICE_10))
    {
        console_type = ConsoleType::PLAYCHOICE_10;
    }
    else
    {
        console_type = ConsoleType::NES;
    }
    std::uint8_t mapper_high_bits = flags & Flags7::MAPPER_HIGH_BITS;
    return Flags7{ .console_type = console_type, .mapper_high_bits = mapper_high_bits };
}

Flags9 parse_flags9(std::uint8_t flags)
{
    TVSystem tv_system = is_set_flag(flags, Flags9::TV_SYSTEM) ? TVSystem::PAL : TVSystem::NTSC;
    std::uint8_t reserved_bits = flags & Flags9::RESERVED_BITS;
    return Flags9{ .tv_system = tv_system, .reserved_bits = reserved_bits };
}

std::expected<RomInfo, std::string> parse_header(std::span<std::uint8_t> rom_data)
{
    if (rom_data.size() < HEADER_SIZE)
    {
        return std::unexpected{ "Cannot parse header: malformed rom" };
    }

    std::array<std::uint8_t, HEADER_SIZE> header_bytes{};
    std::ranges::copy(rom_data.first<HEADER_SIZE>(), header_bytes.begin());
    auto raw_header = std::bit_cast<RawHeader>(header_bytes);

    Flags6 flags6 = parse_flags6(raw_header.flags6);
    Flags7 flags7 = parse_flags7(raw_header.flags7);
    Flags9 flags9 = parse_flags9(raw_header.flags9);
    // Skip Flags10: https://www.nesdev.org/wiki/INES#Flags_10

    return RomInfo{ .header_id = raw_header.header_id,
                    .prg_rom_banks = raw_header.prg_rom_banks,
                    .chr_rom_banks = raw_header.chr_rom_banks,
                    .prg_ram_banks = raw_header.flags8,
                    .mapper_id =
                      static_cast<std::uint8_t>(flags7.mapper_high_bits | flags6.mapper_low_bits),
                    .reserved_bits = flags9.reserved_bits,
                    .padding = raw_header.padding,
                    .mirroring = flags6.mirroring,
                    .console_type = flags7.console_type,
                    .tv_system = flags9.tv_system,
                    .has_battery = flags6.has_battery,
                    .has_trainer = flags6.has_trainer,
                    .has_alternate_nt_layout = flags6.has_alternate_nt_layout };
}

std::expected<void, std::string> validate_rom(const RomInfo& rom_info)
{
    if (rom_info.prg_rom_banks == 0)
    {
        return std::unexpected{ "PRG ROM banks equals 0" };
    }
    if (rom_info.chr_rom_banks == 0)
    {
        return std::unexpected{ "CHR ROM banks equals 0" };
    }
    if (!std::ranges::equal(rom_info.header_id, HEADER_ID))
    {
        return std::unexpected{ "Invalid header id" };
    }
    if (std::ranges::any_of(rom_info.padding, [](std::uint8_t v) { return v != 0; }))
    {
        return std::unexpected{ "Header padding is not zero" };
    }
    if (rom_info.mirroring == Mirroring::UNKNOWN)
    {
        return std::unexpected{ "Unknown mirroring" };
    }
    if (rom_info.console_type == ConsoleType::UNKNOWN)
    {
        return std::unexpected{ "Unknown console type" };
    }
    if (rom_info.tv_system == TVSystem::UNKNOWN)
    {
        return std::unexpected{ "Unknown tv system" };
    }
    if (rom_info.reserved_bits != 0)
    {
        return std::unexpected{ "Reserved bits in flags9 aren't filled with zeros" };
    }
    return {};
}

ParseRomResult parse_ines_rom(std::vector<std::uint8_t> rom_data)
{
    auto parse_result = parse_header(rom_data);
    if (!parse_result)
    {
        return std::unexpected{ std::move(parse_result).error() };
    }

    const RomInfo& rom_info{ parse_result.value() };
    if (auto validation_result = validate_rom(rom_info); !validation_result)
    {
        return std::unexpected{ std::move(validation_result).error() };
    }

    const std::size_t payload_offset = HEADER_SIZE + (rom_info.has_trainer ? TRAINER_SIZE : 0);
    const std::size_t total_size = payload_offset + (PRG_BANK_SIZE * rom_info.prg_rom_banks) +
                                   (CHR_BANK_SIZE * rom_info.chr_rom_banks);

    if (total_size != rom_data.size())
    {
        return std::unexpected{ "Total size of ROM doesn't match with calculated" };
    }

    rom_data.erase(rom_data.begin(),
                   rom_data.begin() +
                     static_cast<std::vector<std::uint8_t>::difference_type>(payload_offset));

    auto create_mapper_result{ create_mapper({ .id = rom_info.mapper_id,
                                               .prg_banks = rom_info.prg_rom_banks,
                                               .rom = std::move(rom_data) }) };
    if (!create_mapper_result)
    {
        return std::unexpected{ std::move(create_mapper_result).error() };
    }

    return ParseRomResult{ std::in_place, rom_info, std::move(create_mapper_result).value() };
}

} // namespace

namespace mayones::core::rom {

std::expected<Cartridge, std::string> load_rom(std::vector<std::uint8_t> rom_data)
{
    switch (identify_rom_format(rom_data))
    {
        case RomFormat::INES:
            return parse_ines_rom(std::move(rom_data));
        case RomFormat::ARCHAIC_INES:
            return std::unexpected{ "Archaic iNES ROM's not supported" };
        case RomFormat::NES20:
            return std::unexpected{ "NES 2.0 ROM's not supported" };
        case RomFormat::UNKNOWN:
            return std::unexpected{ "Unknown ROM format" };
        default:
            std::unreachable();
    }
}

} // namespace mayones::core::rom

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mayones/core/rom.hpp"
#include "mayones/core/rom_loader.hpp"

namespace {

using namespace std::string_view_literals;
using namespace mayones::core::rom;

constexpr std::string_view HEADER_ID{ "NES\x1A"sv };

constexpr std::size_t HEADER_SIZE{ 16 };
constexpr std::size_t HEADER_ID_SIZE{ HEADER_ID.size() };
constexpr std::size_t TRAINER_SIZE{ 512 };

struct RawHeader {
    std::array<std::uint8_t, HEADER_ID_SIZE> header_id;
    std::uint8_t prg_rom_banks_lsb;
    std::uint8_t chr_rom_banks_lsb;
    std::uint8_t flags6;
    std::uint8_t flags7;
    std::uint8_t flags8;
    std::uint8_t flags9;
    std::uint8_t flags10;
    std::uint8_t flags11;
    std::uint8_t flags12;
    std::uint8_t flags13;
    std::uint8_t flags14;
    std::uint8_t flags15;
};

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
    static constexpr std::uint8_t INES_FORMAT_BITS{ 0x00 };
    static constexpr std::uint8_t ARCHAIC_INES_FORMAT_BITS{ 0x04 };
    static constexpr std::uint8_t NES20_FORMAT_BITS{ 0x08 };

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

Flags7 parse_flags7(std::uint8_t flags)
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

    RomFormat rom_format{};

    switch (flags & Flags7::ROM_FORMAT_BITS)
    {
        case Flags7::INES_FORMAT_BITS:
            rom_format = RomFormat::INES;
            break;
        case Flags7::ARCHAIC_INES_FORMAT_BITS:
            rom_format = RomFormat::ARCHAIC_INES;
            break;
        case Flags7::NES20_FORMAT_BITS:
            rom_format = RomFormat::INES20;
            break;
        default:
            std::unreachable();
    }

    return Flags7{ .rom_format = rom_format,
                   .console_type = console_type,
                   .mapper_high_bits =
                     static_cast<std::uint8_t>((flags & Flags7::MAPPER_HIGH_BITS) >> 4) };
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

std::expected<RawHeader, std::string> read_header(std::span<const std::uint8_t> rom_data)
{
    if (rom_data.size() < HEADER_SIZE)
    {
        return std::unexpected{ "ROM header is too small" };
    }

    std::array<std::uint8_t, HEADER_SIZE> header_bytes{};
    std::ranges::copy(rom_data.first<HEADER_SIZE>(), header_bytes.begin());
    return std::bit_cast<RawHeader>(header_bytes);
}

template<typename Func>
concept HeaderParser = requires(Func parser, std::span<const std::uint8_t> rom_data) {
    { parser(rom_data) } -> std::same_as<std::expected<RomInfo, std::string>>;
};

std::expected<RomData, std::string> parse(std::vector<std::uint8_t> rom_data,
                                          HeaderParser auto header_parser)
{
    auto parse_result = header_parser(rom_data);
    if (!parse_result)
    {
        return std::unexpected{ std::move(parse_result).error() };
    }

    const RomInfo rom_info{ std::move(parse_result).value() };

    const std::size_t prg_rom_start_idx = HEADER_SIZE + (rom_info.has_trainer ? TRAINER_SIZE : 0);
    if ((prg_rom_start_idx + rom_info.prg_rom_size + rom_info.chr_rom_size) != rom_data.size())
    {
        return std::unexpected{ "ROM size mismatch" };
    }

    // clang-format off
    auto prg_rom = rom_data |
                   std::views::drop(prg_rom_start_idx) |
                   std::views::take(rom_info.prg_rom_size) |
                   std::ranges::to<std::vector>();
    auto chr_rom = rom_data |
                   std::views::drop(prg_rom_start_idx + rom_info.prg_rom_size) |
                   std::views::take(rom_info.chr_rom_size) |
                   std::ranges::to<std::vector>();
    // clang-format on

    return RomData{ .prg_rom = std::move(prg_rom),
                    .chr_rom = std::move(chr_rom),
                    .rom_info = rom_info };
}

namespace ines {

struct Flags9 {
    enum BitMask : std::uint8_t {
        TV_SYSTEM = 1 << 0,
        RESERVED_BITS = 0xFE
    };

    TVSystem tv_system;
    std::uint8_t reserved_bits;
};

Flags9 parse_flags9(std::uint8_t flags)
{
    return Flags9{ .tv_system =
                     is_set_flags(flags, Flags9::TV_SYSTEM) ? TVSystem::PAL : TVSystem::NTSC,
                   .reserved_bits = static_cast<std::uint8_t>(flags & Flags9::RESERVED_BITS) };
}

std::expected<RomInfo, std::string> parse_header(std::span<const std::uint8_t> rom_data)
{
    auto parse_result = read_header(rom_data);
    if (!parse_result)
    {
        return std::unexpected{ std::move(parse_result).error() };
    }

    auto raw_header = std::move(parse_result).value();

    if (raw_header.prg_rom_banks_lsb == 0)
    {
        return std::unexpected{ "Invalid PRG ROM bank count: 0 (at least 1 required)" };
    }
    if (raw_header.chr_rom_banks_lsb == 0)
    {
        return std::unexpected{ "Invalid CHR ROM bank count: 0 (at least 1 required)" };
    }

    Flags6 flags6 = parse_flags6(raw_header.flags6);
    Flags7 flags7 = parse_flags7(raw_header.flags7);

    Flags9 flags9 = parse_flags9(raw_header.flags9);
    if (flags9.reserved_bits != 0)
    {
        return std::unexpected{ "Reserved bits in flags9 are non-zero" };
    }
    // Skip Flags10: https://www.nesdev.org/wiki/INES#Flags_10

    return RomInfo{ .prg_rom_size = raw_header.prg_rom_banks_lsb * PRG_BANK_SIZE,
                    .chr_rom_size = raw_header.chr_rom_banks_lsb * CHR_BANK_SIZE,
                    .prg_ram_size = raw_header.flags8 * PRG_RAM_BANK_SIZE,
                    .prg_nvram_size = 0,
                    .chr_ram_size = 0,
                    .chr_nvram_size = 0,
                    .mapper_id = static_cast<std::uint8_t>(flags7.mapper_high_bits << 4 |
                                                           flags6.mapper_low_bits),
                    .submapper_id = 0,
                    .rom_format = flags7.rom_format,
                    .mirroring = flags6.mirroring,
                    .console_type = flags7.console_type,
                    .tv_system = flags9.tv_system,
                    .has_battery = flags6.has_battery,
                    .has_trainer = flags6.has_trainer,
                    .has_alternate_nt_layout = flags6.has_alternate_nt_layout };
}

} // namespace ines

namespace ines20 {

struct Flags8 {
    enum BitMask : std::uint8_t {
        MAPPER_HIGH_BITS = 0x0F,
        SUBMAPPER_NUMBER = 0xF0
    };

    std::uint8_t mapper_high_bits;
    std::uint8_t submapper_number;
};

struct Flags9 {
    enum BitMask : std::uint8_t {
        PRG_ROM_BANKS_MSB = 0x0F,
        CHR_ROM_BANKS_MSB = 0xF0
    };

    std::uint8_t prg_rom_banks_msb;
    std::uint8_t chr_rom_banks_msb;
};

struct Flags10 {
    enum BitMask : std::uint8_t {
        PRG_RAM_SHIFT = 0x0F,
        PRG_NVRAM_SHIFT = 0xF0
    };

    std::uint8_t prg_ram_shift;
    std::uint8_t prg_nvram_shift;
};

struct Flags11 {
    enum BitMask : std::uint8_t {
        CHR_RAM_SHIFT = 0x0F,
        CHR_NVRAM_SHIFT = 0xF0
    };

    std::uint8_t chr_ram_shift;
    std::uint8_t chr_nvram_shift;
};

struct Flags12 {
    enum BitMask : std::uint8_t {
        TV_SYSTEM = 0x03,
    };

    TVSystem tv_system;
};

Flags8 parse_flags8(std::uint8_t flags)
{
    return { .mapper_high_bits = static_cast<uint8_t>(flags & Flags8::MAPPER_HIGH_BITS),
             .submapper_number = static_cast<uint8_t>((flags & Flags8::SUBMAPPER_NUMBER) >> 4) };
}

Flags9 parse_flags9(std::uint8_t flags)
{
    return { .prg_rom_banks_msb = static_cast<uint8_t>(flags & Flags9::PRG_ROM_BANKS_MSB),
             .chr_rom_banks_msb = static_cast<uint8_t>((flags & Flags9::CHR_ROM_BANKS_MSB) >> 4) };
}

Flags10 parse_flags10(std::uint8_t flags)
{
    return { .prg_ram_shift = static_cast<uint8_t>(flags & Flags10::PRG_RAM_SHIFT),
             .prg_nvram_shift = static_cast<uint8_t>((flags & Flags10::PRG_NVRAM_SHIFT) >> 4) };
}

Flags11 parse_flags11(std::uint8_t flags)
{
    return { .chr_ram_shift = static_cast<uint8_t>(flags & Flags11::CHR_RAM_SHIFT),
             .chr_nvram_shift = static_cast<uint8_t>((flags & Flags11::CHR_NVRAM_SHIFT) >> 4) };
}

Flags12 parse_flags12(std::uint8_t flags)
{
    switch (flags & Flags12::TV_SYSTEM)
    {
        case 0:
            return { .tv_system = TVSystem::NTSC };
        case 1:
            return { .tv_system = TVSystem::PAL };
        case 2:
            return { .tv_system = TVSystem::MULTIPLE_REGION };
        case 3:
            return { .tv_system = TVSystem::DENDY };
        default:
            std::unreachable();
    }
}

std::expected<RomInfo, std::string> parse_header(std::span<const std::uint8_t> rom_data)
{
    auto parse_result = read_header(rom_data);
    if (!parse_result)
    {
        return std::unexpected{ std::move(parse_result).error() };
    }

    auto raw_header = std::move(parse_result).value();

    Flags6 flags6 = parse_flags6(raw_header.flags6);
    Flags7 flags7 = parse_flags7(raw_header.flags7);
    Flags8 flags8 = parse_flags8(raw_header.flags8);
    Flags9 flags9 = parse_flags9(raw_header.flags9);
    Flags10 flags10 = parse_flags10(raw_header.flags10);
    Flags11 flags11 = parse_flags11(raw_header.flags11);
    Flags12 flags12 = parse_flags12(raw_header.flags12);
    // TODO: For now skip flags 13, 14, 15.

    std::uint16_t mapper_id =
      flags8.mapper_high_bits << 8 | flags7.mapper_high_bits << 4 | flags6.mapper_low_bits;

    auto calculate_rom_size = [](std::uint8_t rom_size_lsb,
                                 std::uint8_t rom_size_msb,
                                 std::size_t bank_size) -> std::size_t {
        if (rom_size_msb == 0x0F)
        {
            /*
                If the MSB nibble is $F, an exponent-multiplier notation is used:

                ++++----------- Header byte 9 D3..D0
                ||||   ++++-++++- Header byte 4
                D~BA98 7654 3210
                --------------
                1111 EEEE EEMM
                     |||| ||++- Multiplier, actual value is MM*2+1 (1,3,5,7)
                     ++++-++--- Exponent (2^E), 0-63

                The actual PRG-ROM size is 2^E *(MM*2+1) bytes.
            */

            return (1 << (rom_size_lsb >> 2)) * (((rom_size_lsb & 0x3) * 2) + 1);
        }
        return (rom_size_msb << 8 | rom_size_lsb) * bank_size;
    };

    std::size_t prg_rom_size =
      calculate_rom_size(raw_header.prg_rom_banks_lsb, flags9.prg_rom_banks_msb, PRG_BANK_SIZE);
    std::size_t chr_rom_size =
      calculate_rom_size(raw_header.chr_rom_banks_lsb, flags9.chr_rom_banks_msb, CHR_BANK_SIZE);

    auto calculate_ram_size = [](std::uint8_t shift_bits) -> std::size_t {
        return shift_bits != 0 ? 64 << shift_bits : 0;
    };

    std::size_t prg_ram_size = calculate_ram_size(flags10.prg_ram_shift);
    std::size_t prg_nvram_size = calculate_ram_size(flags10.prg_nvram_shift);

    std::size_t chr_ram_size = calculate_ram_size(flags11.chr_ram_shift);
    std::size_t chr_nvram_size = calculate_ram_size(flags11.chr_nvram_shift);

    return RomInfo{ .prg_rom_size = prg_rom_size,
                    .chr_rom_size = chr_rom_size,
                    .prg_ram_size = prg_ram_size,
                    .prg_nvram_size = prg_nvram_size,
                    .chr_ram_size = chr_ram_size,
                    .chr_nvram_size = chr_nvram_size,
                    .mapper_id = mapper_id,
                    .submapper_id = flags8.submapper_number,
                    .rom_format = flags7.rom_format,
                    .mirroring = flags6.mirroring,
                    .console_type = flags7.console_type,
                    .tv_system = flags12.tv_system,
                    .has_battery = flags6.has_battery,
                    .has_trainer = flags6.has_trainer,
                    .has_alternate_nt_layout = flags6.has_alternate_nt_layout };
}

} // namespace ines20

} // namespace

namespace mayones::core::rom {

std::expected<RomData, std::string> load_rom(std::vector<std::uint8_t> rom_data)
{
    constexpr std::size_t FLAGS7_INDEX{ 7 };

    if (auto validation_result = validate_header_id(rom_data); !validation_result)
    {
        return std::unexpected{ std::move(validation_result).error() };
    }

    if (rom_data.size() <= FLAGS7_INDEX)
    {
        return std::unexpected{ "ROM is too small to identify format" };
    }

    auto flags7 = parse_flags7(rom_data[FLAGS7_INDEX]);

    switch (flags7.rom_format)
    {
        case RomFormat::INES:
            return parse(std::move(rom_data), ines::parse_header);
        case RomFormat::INES20:
            return parse(std::move(rom_data), ines20::parse_header);
        case RomFormat::ARCHAIC_INES:
            return std::unexpected{ "Archaic iNES format is not supported" };
        default:
            std::unreachable();
    }
}

} // namespace mayones::core::rom

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "mayones/core/core.hpp"
#include "mayones/core/rom.hpp"

using namespace std::string_view_literals;

namespace {

constexpr std::size_t HEADER_SIZE{ 16 };
constexpr std::size_t HEADER_ID_SIZE{ 4 };
constexpr std::size_t PRG_BANK_SIZE{ 16384 };
constexpr std::size_t CHR_BANK_SIZE{ 8192 };
constexpr std::size_t TRAINER_SIZE{ 512 };
constexpr std::size_t PADDING_SIZE{ 5 };

struct RomHeader {
    std::array<std::uint8_t, HEADER_ID_SIZE> header_id{ 'N', 'E', 'S', 0x1A };
    std::uint8_t prg_banks{ 1 };
    std::uint8_t chr_banks{ 1 };
    std::uint8_t flags6{ 0 };
    std::uint8_t flags7{ 0 };
    std::uint8_t flags8{ 0 };
    std::uint8_t flags9{ 0 };
    std::uint8_t flags10{ 0 };
    std::array<uint8_t, PADDING_SIZE> padding{};
};

struct LoadRomParams {
    std::optional<RomHeader> header;
    std::uint8_t prg_banks;
    std::uint8_t chr_banks;
    bool has_trainer;
};

struct LoadRomFailedParams {
    LoadRomParams rom_params;
    std::string_view expected_error;
};

struct LoadRomSuccessParams {
    LoadRomParams rom_params;
    mayones::core::rom::RomInfo expected_rom_info;
};

class LoadRomFailedInputParams : public testing::TestWithParam<LoadRomFailedParams> {};
class LoadRomInputParams : public testing::TestWithParam<LoadRomSuccessParams> {};

std::array<std::uint8_t, HEADER_SIZE> dump_header(const RomHeader& params = {})
{
    return std::bit_cast<std::array<std::uint8_t, HEADER_SIZE>>(params);
}

[[maybe_unused]] void PrintTo(const LoadRomParams& params, std::ostream* stream)
{
    if (params.header)
    {
        *stream << std::format(" header={::02X},", dump_header(params.header.value()));
    }
    *stream << std::format(" prg_banks={}, chr_banks={}, has_trainer={}",
                           params.prg_banks,
                           params.chr_banks,
                           params.has_trainer);
}

[[maybe_unused]] void PrintTo(const LoadRomFailedParams& params, std::ostream* stream)
{
    *stream << " rom_params=";
    PrintTo(params.rom_params, stream);
    *stream << std::format(" expected_error={}", params.expected_error);
}

std::vector<std::uint8_t> make_rom(const LoadRomParams& rom_params)
{
    if (!rom_params.header)
    {
        return {};
    }
    const std::size_t total_size = HEADER_SIZE + (rom_params.has_trainer ? TRAINER_SIZE : 0) +
                                   (rom_params.prg_banks * PRG_BANK_SIZE) +
                                   (rom_params.chr_banks * CHR_BANK_SIZE);
    std::vector<std::uint8_t> rom_data(total_size);
    std::ranges::copy(dump_header(rom_params.header.value()), rom_data.begin());
    return rom_data;
}

} // namespace

TEST_P(LoadRomFailedInputParams, LoadRomFailed)
{
    const auto& [rom_params, expected_error] = GetParam();
    mayones::core::NesCore nes_core{};
    auto load_result = nes_core.load_rom(make_rom(rom_params));
    ASSERT_EQ(load_result.has_value(), false);
    ASSERT_EQ(load_result.error(), expected_error);
}

TEST_P(LoadRomInputParams, LoadRom)
{
    const auto& [rom_params, expected_rom_info] = GetParam();
    mayones::core::NesCore nes_core{};
    auto load_result = nes_core.load_rom(make_rom(rom_params));
    const auto& rom_info = nes_core.rom_info();
    ASSERT_EQ(load_result.has_value(), true) << load_result.error();
    ASSERT_EQ(rom_info.prg_rom_banks, expected_rom_info.prg_rom_banks);
    ASSERT_EQ(rom_info.chr_rom_banks, expected_rom_info.chr_rom_banks);
    ASSERT_EQ(rom_info.prg_ram_banks, expected_rom_info.prg_ram_banks);
    ASSERT_EQ(rom_info.mapper_id, expected_rom_info.mapper_id);

    ASSERT_EQ(rom_info.mirroring, expected_rom_info.mirroring);
    ASSERT_EQ(rom_info.console_type, expected_rom_info.console_type);
    ASSERT_EQ(rom_info.tv_system, expected_rom_info.tv_system);
    ASSERT_EQ(rom_info.has_battery, expected_rom_info.has_battery);
    ASSERT_EQ(rom_info.has_trainer, expected_rom_info.has_trainer);
    ASSERT_EQ(rom_info.has_alternate_nt_layout, expected_rom_info.has_alternate_nt_layout);
}

INSTANTIATE_TEST_SUITE_P(
  Core,
  LoadRomFailedInputParams,
  testing::Values(
    LoadRomFailedParams{
      .rom_params = { .header = {}, .prg_banks = 0, .chr_banks = 0, .has_trainer = false },
      .expected_error = "ROM is too small for header ID parsing"sv },
    LoadRomFailedParams{ .rom_params = { .header = RomHeader{ .flags7 = 0x04 },
                                         .prg_banks = 1,
                                         .chr_banks = 1,
                                         .has_trainer = false },
                         .expected_error = "Archaic iNES format is not supported"sv },
    LoadRomFailedParams{ .rom_params = { .header = RomHeader{ .flags7 = 0x08 },
                                         .prg_banks = 1,
                                         .chr_banks = 1,
                                         .has_trainer = false },
                         .expected_error = "NES 2.0 format is not supported"sv },
    LoadRomFailedParams{
      .rom_params = { .header = RomHeader{ .header_id = { 'N', 'E', 'T', 0x1A } },
                      .prg_banks = 1,
                      .chr_banks = 1,
                      .has_trainer = false },
      .expected_error = "Invalid iNES header ID"sv },
    LoadRomFailedParams{
      .rom_params = { .header = RomHeader{ .prg_banks = 0x00 },
                      .prg_banks = 1,
                      .chr_banks = 1,
                      .has_trainer = false },
      .expected_error = "Invalid PRG ROM bank count: 0 (at least 1 required)"sv },
    LoadRomFailedParams{
      .rom_params = { .header = RomHeader{ .chr_banks = 0x00 },
                      .prg_banks = 1,
                      .chr_banks = 1,
                      .has_trainer = false },
      .expected_error = "Invalid CHR ROM bank count: 0 (at least 1 required)"sv },
    LoadRomFailedParams{
      .rom_params = { .header = RomHeader{}, .prg_banks = 0, .chr_banks = 0, .has_trainer = false },
      .expected_error = "ROM size mismatch"sv },
    LoadRomFailedParams{ .rom_params = { .header = RomHeader{ .padding = { 0x01 } },
                                         .prg_banks = 1,
                                         .chr_banks = 1,
                                         .has_trainer = false },
                         .expected_error = "Header padding contains non-zero bytes"sv },
    LoadRomFailedParams{ .rom_params = { .header = RomHeader{ .flags6 = 0x10 },
                                         .prg_banks = 1,
                                         .chr_banks = 1,
                                         .has_trainer = false },
                         .expected_error = "Unsupported mapper: id=1"sv }));

INSTANTIATE_TEST_SUITE_P(
  Core,
  LoadRomInputParams,
  testing::Values(
    LoadRomSuccessParams{
      .rom_params = { .header = RomHeader{}, .prg_banks = 1, .chr_banks = 1, .has_trainer = false },
      .expected_rom_info =
        mayones::core::rom::RomInfo{ .prg_rom_banks = 1,
                                     .chr_rom_banks = 1,
                                     .prg_ram_banks = 0,
                                     .mapper_id = 0,
                                     .rom_format = mayones::core::rom::RomFormat::INES,
                                     .mirroring = mayones::core::rom::Mirroring::HORIZONTAL,
                                     .console_type = mayones::core::rom::ConsoleType::NES,
                                     .tv_system = mayones::core::rom::TVSystem::NTSC,
                                     .has_battery = false,
                                     .has_trainer = false,
                                     .has_alternate_nt_layout = false } },
    LoadRomSuccessParams{ .rom_params = { .header = RomHeader{ .flags6 = 0x04 },
                                          .prg_banks = 1,
                                          .chr_banks = 1,
                                          .has_trainer = true },
                          .expected_rom_info = mayones::core::rom::RomInfo{
                            .prg_rom_banks = 1,
                            .chr_rom_banks = 1,
                            .prg_ram_banks = 0,
                            .mapper_id = 0,
                            .rom_format = mayones::core::rom::RomFormat::INES,
                            .mirroring = mayones::core::rom::Mirroring::HORIZONTAL,
                            .console_type = mayones::core::rom::ConsoleType::NES,
                            .tv_system = mayones::core::rom::TVSystem::NTSC,
                            .has_battery = false,
                            .has_trainer = true,
                            .has_alternate_nt_layout = false } }));

#include <array>
#include <format>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "mayones/app/config.hpp"

using namespace std::string_view_literals;

namespace {

struct ReadConfigParams {
    std::vector<std::string_view> cli_args;
    bool has_value;
    mayones::app::Config expected_config;
};

[[maybe_unused]] void PrintTo(const ReadConfigParams& params, std::ostream* stream)
{
    *stream << std::format(" cli_args={}, has_value={}, expected={{ rom_path={}, debug={} }}",
                           params.cli_args |
                             std::views::join_with(" "sv) | // NOLINT(misc-include-cleaner)
                             std::ranges::to<std::string>(),
                           params.has_value,
                           params.expected_config.rom_path.string(),
                           params.expected_config.debug);
}

class ReadConfigValidInputParams : public testing::TestWithParam<ReadConfigParams> {};

} // namespace

TEST(AppTest, ReadConfigCLIEmptyArgs)
{
    auto read_result =
      mayones::app::read_config(std::span<const std::string_view>{}, "MAYONES_TEST_"sv);
    ASSERT_EQ(read_result.has_value(), false);
    ASSERT_EQ(read_result.error(), "CLI args is empty"sv);
}

TEST(AppTest, ReadConfigUnexpectedCLIOption)
{
    constexpr std::array invalid_options{ "mayones.exe"sv, "--invalid-opt"sv };
    auto read_result = mayones::app::read_config(std::span{ invalid_options }, "MAYONES_TEST_"sv);
    ASSERT_EQ(read_result.has_value(), false);
    ASSERT_EQ(read_result.error(), "Unexpected option `--invalid-opt`"sv);
}

TEST(AppTest, ReadConfigUnexpectedPositionalArgument)
{
    constexpr std::array invalid_options{ "mayones.exe"sv, "--debug"sv, "unexpected-value"sv };
    auto read_result = mayones::app::read_config(std::span{ invalid_options }, "MAYONES_TEST_"sv);
    ASSERT_EQ(read_result.has_value(), false);
    ASSERT_EQ(read_result.error(), "Unexpected positional argument `unexpected-value`"sv);
}

TEST_P(ReadConfigValidInputParams, ReadConfig)
{
    const auto& [cli_args, has_value, expected_config] = GetParam();
    auto read_result = mayones::app::read_config(std::span{ cli_args }, "MAYONES_TEST_"sv);

    ASSERT_EQ(read_result.has_value(), has_value);
    ASSERT_EQ(read_result->debug, expected_config.debug);
    ASSERT_EQ(read_result->rom_path, expected_config.rom_path);
}

INSTANTIATE_TEST_SUITE_P(
  AppTest,
  ReadConfigValidInputParams,
  testing::Values(
    ReadConfigParams{ .cli_args = { "mayones.exe" },
                      .has_value = true,
                      .expected_config = { .rom_path = {}, .debug = false } },
    ReadConfigParams{ .cli_args = { "mayones.exe", "--debug" },
                      .has_value = true,
                      .expected_config = { .rom_path = {}, .debug = true } },
    ReadConfigParams{ .cli_args = { "mayones.exe", "--rom", "rom.nes" },
                      .has_value = true,
                      .expected_config = { .rom_path = { "rom.nes" }, .debug = false } },
    ReadConfigParams{ .cli_args = { "mayones.exe", "--debug", "--rom", "rom.nes" },
                      .has_value = true,
                      .expected_config = { .rom_path = { "rom.nes" }, .debug = true } },
    ReadConfigParams{ .cli_args = { "mayones.exe", "--rom", "rom.nes", "--debug" },
                      .has_value = true,
                      .expected_config = { .rom_path = { "rom.nes" }, .debug = true } }));

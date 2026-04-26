#include <algorithm>
#include <cctype>  // for std::tolower
#include <cstdlib> // for std::getenv
#include <expected>
#include <filesystem>
#include <format>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mayones/app/config.hpp"

using namespace std::string_view_literals;

namespace {

struct ConfigOverlay {
    std::optional<std::filesystem::path> rom_path;
    std::optional<bool> debug;
};

std::expected<ConfigOverlay, std::string> parse_cli_args(std::span<const std::string_view> cli_args)
{
    if (cli_args.empty())
    {
        return std::unexpected{ "CLI args is empty" };
    }

    constexpr std::string_view ROM_OPTION{ "--rom" };
    constexpr std::string_view DEBUG_OPTION{ "--debug" };

    ConfigOverlay config{};
    std::vector<std::string_view> options_stack{};
    options_stack.reserve(2);

    for (const auto& arg : cli_args.subspan(1))
    {
        if (arg.starts_with("--"))
        {
            if (arg == ROM_OPTION)
            {
                options_stack.push_back(arg);
            }
            else if (arg == DEBUG_OPTION)
            {
                config.debug = true;
            }
            else
            {
                return std::unexpected{ std::format("Unexpected option `{}`", arg) };
            }
        }
        else
        {
            if (!options_stack.empty())
            {
                if (options_stack.back() == ROM_OPTION)
                {
                    config.rom_path = arg;
                    options_stack.pop_back();
                }
            }
            else
            {
                return std::unexpected{ std::format("Unexpected positional argument `{}`", arg) };
            }
        }
    }

    if (!options_stack.empty())
    {
        auto error_message = options_stack |
                             std::views::join_with(", "sv) | // NOLINT(misc-include-cleaner)
                             std::ranges::to<std::string>();
        return std::unexpected{ std::format("Options `{}` required values", error_message) };
    }

    return config;
}

std::expected<ConfigOverlay, std::string> parse_env_vars(std::string_view prefix)
{
    const std::string DEBUG_VAR = std::format("{}DEBUG", prefix);
    const std::string ROM_VAR = std::format("{}ROM", prefix);

    auto read_env = [](const std::string& var) -> std::string_view {
        // NOLINTNEXTLINE(clang-diagnostic-deprecated-declarations)
        const char* value_ptr = std::getenv(var.data());
        if (!value_ptr)
        {
            return std::string_view{};
        }
        return std::string_view{ value_ptr };
    };

    auto icase_comparator = [](char a, char b) -> bool { // NOLINT(readability-identifier-length)
        return std::tolower(static_cast<unsigned char>(a)) ==
               std::tolower(static_cast<unsigned char>(b));
    };

    ConfigOverlay config{};

    if (const std::string_view value = read_env(DEBUG_VAR); !value.empty())
    {
        if (std::ranges::equal(value, "true"sv, icase_comparator))
        {
            config.debug = true;
        }
        else if (std::ranges::equal(value, "false"sv, icase_comparator))
        {
            config.debug = false;
        }
        else
        {
            return std::unexpected{ std::format("Invalid value for {} var", DEBUG_VAR) };
        }
    }

    if (const std::string_view value = read_env(ROM_VAR); !value.empty())
    {
        config.rom_path = value;
    }

    return config;
}

} // namespace

namespace mayones::app {

std::expected<Config, std::string> read_config(std::span<const std::string_view> cli_args,
                                               std::string_view env_prefix)
{
    auto read_cli_config_result = parse_cli_args(cli_args);
    if (!read_cli_config_result)
    {
        return std::unexpected{ std::move(read_cli_config_result).error() };
    }

    auto read_env_config_result = parse_env_vars(env_prefix);
    if (!read_env_config_result)
    {
        return std::unexpected{ std::move(read_env_config_result).error() };
    }

    auto cli_config = std::move(read_cli_config_result).value();
    auto env_config = std::move(read_env_config_result).value();

    return Config{ .rom_path = cli_config.rom_path.value_or(env_config.rom_path.value_or({})),
                   .debug = cli_config.debug.value_or(env_config.debug.value_or(false)) };
}

}; // namespace mayones::app

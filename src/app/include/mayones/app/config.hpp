#pragma once

#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace mayones::app {

struct Config {
    std::filesystem::path rom_path;
    bool debug{ false };
};

std::expected<Config, std::string> read_config(std::span<const std::string_view> cli_args,
                                               std::string_view env_prefix);

}; // namespace mayones::app

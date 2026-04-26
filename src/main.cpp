#include <cstdio>
#include <exception>
#include <print>
#include <string_view>
#include <utility>
#include <vector>

#include "mayones/app/app.hpp"
#include "mayones/app/config.hpp"

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
int run(int argc, char* argv[])
{
    using namespace std::string_view_literals;

    auto read_config_result =
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,  misc-include-cleaner)
      mayones::app::read_config(std::vector<std::string_view>(argv, argv + argc), "MAYONES_"sv);
    if (!read_config_result)
    {
        std::println(stderr, "Configuration reading error: {}", read_config_result.error());
        return 1;
    }

    const mayones::app::MayoNES app{ std::move(read_config_result).value() };

    return 0;
}

} // namespace

int main(int argc, char* argv[])
{
    try
    {
        return run(argc, argv);
    }
    catch (const std::exception& error)
    {
        // NOLINTNEXTLINE( cppcoreguidelines-pro-type-vararg, modernize-use-std-print)
        std::fprintf(stderr, "Internal error: %s\n", error.what());
        return 1;
    }
    catch (...)
    {
        std::fputs("Unknown critical error occurred\n", stderr);
        return 1;
    }
}

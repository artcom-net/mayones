#include <cstddef>
#include <cstdio>
#include <format>
#include <print>
#include <utility>
#include <vector>

#include "mayones/app/app.hpp"
#include "mayones/app/config.hpp"
#include "mayones/app/formatters.hpp"
#include "mayones/app/version.hpp"
#include "mayones/core/core.hpp"
#include "mayones/core/rom.hpp"
#include "mayones/io/file.hpp"

namespace mayones::app {

MayoNes::MayoNes(Config config) :
    config_{ std::move(config) }
{
}

int MayoNes::run()
{
    std::println("MayoNES {}\n{}", mayones::VERSION, config_);

    auto read_result = mayones::io::read_binary_file(config_.rom_path);
    if (!read_result)
    {
        println(stderr, "Error read ROM: {}", read_result.error());
        return 1;
    }

    if (auto load_result = nes_core_.load_rom(std::move(read_result).value()); !load_result)
    {
        std::println(stderr, "Error load ROM: {}", load_result.error());
        return 1;
    }

    const core::rom::RomInfo& rom_info = nes_core_.rom_info();
    std::println("{}", rom_info);

    nes_core_.reset();

    for (std::size_t i = 0; i < 10; ++i)
    {
        if (config_.debug)
        {
            std::println("{}", nes_core_.trace_tick_frame());
        }
        else
        {
            nes_core_.tick_frame();
        }
    }

    return 0;
}

} // namespace mayones::app

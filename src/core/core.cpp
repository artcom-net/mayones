#include <cstdint>
#include <expected>
#include <string>
#include <utility>
#include <vector>

#include "mayones/core/core.hpp"
#include "mayones/core/mapper.hpp"
#include "mayones/core/rom_loader.hpp"

namespace mayones::core {

NesCore::NesCore() :
    mapper_{ rom::DummyMapper{} },
    bus_{ mapper_ },
    cpu_{ bus_ },
    rom_data_{}
{
}

std::expected<void, std::string> NesCore::load_rom(std::vector<std::uint8_t> rom_data)
{
    auto load_result = rom::load_rom(std::move(rom_data));
    if (!load_result)
    {
        return std::unexpected{ std::move(load_result).error() };
    }

    rom_data_ = std::move(load_result).value();

    auto create_result = rom::create_mapper(rom_data_);
    if (!create_result)
    {
        return std::unexpected{ std::move(create_result).error() };
    }

    mapper_ = std::move(create_result).value();

    return {};
}

const rom::RomInfo& NesCore::rom_info() const noexcept
{
    return rom_data_.rom_info;
}

void NesCore::reset()
{
    cpu_.reset();
}

void NesCore::reset(std::uint16_t pc)
{
    cpu_.reset(pc);
}

void NesCore::tick_frame()
{
    cpu_.tick();
}

Cpu::TraceEntry NesCore::trace_tick_frame()
{
    return cpu_.trace_tick();
}

} // namespace mayones::core

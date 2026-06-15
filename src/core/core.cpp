#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <utility>
#include <vector>

#include "mayones/core/core.hpp"
#include "mayones/core/mapper.hpp"
#include "mayones/core/rom_loader.hpp"

namespace mayones::core {

NesCore::NesCore(std::span<PixelColor, FRAME_BUFFER_SIZE> frame_buffer) :
    mapper_{ rom::DummyMapper{} },
    ppu_bus_{ mapper_ },
    ppu_{ ppu_bus_, frame_buffer },
    bus_{ mapper_, ppu_ },
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
    ppu_.reset();
}

void NesCore::reset(std::uint16_t pc)
{
    cpu_.reset(pc);
    ppu_.reset();
}

void NesCore::tick_frame()
{
    std::size_t cpu_tick{};
    for (std::size_t ppu_cycle = 0; ppu_cycle < PPU_CYCLES_PER_FRAME; ++ppu_cycle)
    {
        ppu_.tick();

        if (cpu_tick++ % 3 == 0)
        {
            cpu_.tick();
        }

        if (ppu_.is_nmi_pending())
        {
            cpu_.trigger_nmi();
        }
    }
}

Cpu::TraceEntry NesCore::trace_tick_frame()
{
    return cpu_.trace_tick();
}

} // namespace mayones::core

#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

#include "mayones/core/bus.hpp"
#include "mayones/core/cpu.hpp"
#include "mayones/core/mapper.hpp"
#include "mayones/core/rom.hpp"

namespace mayones::core {

class NesCore {
public:
    explicit NesCore();

    std::expected<void, std::string> load_rom(std::vector<std::uint8_t> rom_data);
    [[nodiscard]] const rom::RomInfo& rom_info() const noexcept;

    void reset();
    void reset(std::uint16_t pc);
    void tick_frame();
    Cpu::TraceEntry trace_tick_frame();

private:
    rom::Mapper mapper_;
    CpuBus bus_;
    Cpu cpu_;
    rom::RomData rom_data_;
};

} // namespace mayones::core

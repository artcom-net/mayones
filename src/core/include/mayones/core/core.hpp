#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

#include "mayones/core/bus.hpp"
#include "mayones/core/cpu.hpp"
#include "mayones/core/mapper.hpp"
#include "mayones/core/ppu.hpp"
#include "mayones/core/ppu_bus.hpp"
#include "mayones/core/rom.hpp"

namespace mayones::core {

class NesCore {
public:
    explicit NesCore(std::span<PixelColor, FRAME_BUFFER_SIZE> frame_buffer);

    std::expected<void, std::string> load_rom(std::vector<std::uint8_t> rom_data);
    [[nodiscard]] const rom::RomInfo& rom_info() const noexcept;

    void reset();
    void reset(std::uint16_t pc);
    void tick_frame();
    Cpu::TraceEntry trace_tick_frame();

private:
    static constexpr std::size_t PPU_CYCLES_PER_SCANLINE{ 341 };
    static constexpr std::size_t PPU_SCANLINE_PER_FRAME{ 262 };
    static constexpr std::size_t PPU_CYCLES_PER_FRAME{ PPU_CYCLES_PER_SCANLINE *
                                                       PPU_SCANLINE_PER_FRAME };

    rom::Mapper mapper_;
    PpuBus ppu_bus_;
    Ppu ppu_;
    CpuBus bus_;
    Cpu cpu_;
    rom::RomData rom_data_;
};

} // namespace mayones::core

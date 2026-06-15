#pragma once

#include <array>
#include <cstdint>

#include "mayones/core/const.hpp"
#include "mayones/core/mapper.hpp"

namespace mayones::core {

class PpuBus {
public:
    explicit PpuBus(rom::Mapper& mapper);

    [[nodiscard]] std::uint8_t read(std::uint16_t address) const;
    void write(std::uint16_t address, std::uint8_t data);

private:
    rom::Mapper& mapper_;
    std::array<std::uint8_t, 2 * KB> vram_{};
};

} // namespace mayones::core

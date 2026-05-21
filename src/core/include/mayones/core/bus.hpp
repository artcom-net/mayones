#pragma once

#include <array>
#include <cstdint>

#include "mayones/core/const.hpp"
#include "mayones/core/mapper.hpp"

namespace mayones::core {

class CpuBus {
public:
    explicit CpuBus(rom::Mapper& mapper);

    [[nodiscard]] std::uint8_t read(std::uint16_t address) const;
    void write(std::uint16_t address, std::uint8_t data);

private:
    static constexpr std::uint16_t RAM_MASK{ 0x07FF };

    std::array<std::uint8_t, 2 * KB> ram_;
    rom::Mapper& mapper_;
};

} // namespace mayones::core

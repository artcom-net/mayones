#pragma once

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <variant>

#include "mayones/core/rom.hpp"

namespace mayones::core::rom {

class DummyMapper {
public:
    static std::expected<DummyMapper, std::string> create(const RomData& rom_data) noexcept;
    [[nodiscard]] std::uint8_t read_prg(std::uint16_t address) const;
    void write_prg(std::uint16_t address, std::uint8_t data);
};

class NromMapper {
public:
    static std::expected<NromMapper, std::string> create(const RomData& rom_data) noexcept;
    [[nodiscard]] std::uint8_t read_prg(std::uint16_t address) const;
    void write_prg(std::uint16_t address, std::uint8_t data);

private:
    std::span<const std::uint8_t> prg_rom_;
    std::span<const std::uint8_t> chr_rom_;
    std::uint16_t address_mask_;

    explicit NromMapper(const RomData& rom_data);
};

using Mapper = std::variant<DummyMapper, NromMapper>;

std::expected<Mapper, std::string> create_mapper(const RomData& rom_data);

} // namespace mayones::core::rom

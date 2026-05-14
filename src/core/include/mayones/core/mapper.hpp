#pragma once

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace mayones::core::rom {

class DummyMapper;
class Mapper0;

using Mapper = std::variant<DummyMapper, Mapper0>;

struct CreateMapperConfig {
    std::uint8_t id;
    std::uint8_t prg_banks;
    std::vector<std::uint8_t> rom;
};

std::expected<Mapper, std::string> create_mapper(CreateMapperConfig mapper_config);

class DummyMapper {
public:
    [[nodiscard]] std::uint8_t read_prg(std::uint16_t address) const;
    [[nodiscard]] std::uint8_t read_chr(std::uint16_t address) const;

    static constexpr std::string_view NAME{ "DummyMapper" };
    static constexpr std::uint8_t ID{ 0xFF };
};

class Mapper0 {
public:
    static std::expected<Mapper0, std::string> create(std::uint8_t prg_banks,
                                                      std::vector<std::uint8_t> rom);

    [[nodiscard]] std::uint8_t read_prg(std::uint16_t address) const;
    [[nodiscard]] std::uint8_t read_chr(std::uint16_t address) const;

    static constexpr std::string_view NAME{ "NROM" };
    static constexpr std::uint8_t ID{ 0x00 };

private:
    explicit Mapper0(std::uint8_t prg_banks, std::vector<std::uint8_t> rom);

    std::vector<std::uint8_t> m_rom;
    std::span<std::uint8_t> m_prg_rom;
    std::span<std::uint8_t> m_chr_rom;
    std::uint16_t m_prg_address_mask;
};

} // namespace mayones::core::rom

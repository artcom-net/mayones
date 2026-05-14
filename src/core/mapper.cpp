#include <cstdint>
#include <expected>
#include <format>
#include <span>
#include <string>
#include <utility>

#include "mayones/core/mapper.hpp"
#include "mayones/core/rom.hpp"

namespace mayones::core::rom {

constexpr std::uint16_t MASK_16KB{ 0x3FFF };
constexpr std::uint16_t MASK_32KB{ 0x7FFF };

std::expected<Mapper, std::string> create_mapper(CreateMapperConfig mapper_config)
{
    switch (mapper_config.id)
    {
        case Mapper0::ID:
            return Mapper0::create(mapper_config.prg_banks, std::move(mapper_config.rom));
        default:
            return std::unexpected{ std::format("Unsupported mapper: id={}", mapper_config.id) };
    }
}

inline std::uint8_t DummyMapper::read_prg(std::uint16_t address) const
{
    return 0x00;
}

inline std::uint8_t DummyMapper::read_chr(std::uint16_t address) const
{
    return 0x00;
}

std::expected<Mapper0, std::string> Mapper0::create(std::uint8_t prg_banks,
                                                    std::vector<std::uint8_t> rom)
{
    if (prg_banks != 1 && prg_banks != 2)
    {
        return std::unexpected{ "Invalid number of PRG banks" };
    }

    if (rom.size() != ((prg_banks * PRG_BANK_SIZE) + CHR_BANK_SIZE))
    {
        return std::unexpected{ "ROM size mismatch" };
    }

    return Mapper0{ prg_banks, std::move(rom) };
}

Mapper0::Mapper0(std::uint8_t prg_banks, std::vector<std::uint8_t> rom) :
    m_rom{ std::move(rom) },
    m_prg_rom{ std::span{ m_rom }.subspan(0, prg_banks * PRG_BANK_SIZE) },
    m_chr_rom{ std::span{ m_rom }.subspan(prg_banks * PRG_BANK_SIZE, CHR_BANK_SIZE) },
    m_prg_address_mask{ prg_banks == 1 ? MASK_16KB : MASK_32KB }
{
}

inline std::uint8_t Mapper0::read_prg(std::uint16_t address) const
{
    return m_prg_rom[address & m_prg_address_mask];
}

inline std::uint8_t Mapper0::read_chr(std::uint16_t address) const
{
    return m_chr_rom[address];
}

} // namespace mayones::core::rom

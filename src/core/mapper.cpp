#include <cstdint>
#include <expected>
#include <format>
#include <span>
#include <string>

#include "mayones/core/mapper.hpp"
#include "mayones/core/rom.hpp"

namespace mayones::core::rom {

constexpr std::uint16_t MASK_16KB{ 0x3FFF };
constexpr std::uint16_t MASK_32KB{ 0x7FFF };

std::expected<Mapper, std::string> create_mapper(const RomData& rom_data)
{
    switch (rom_data.rom_info.mapper_id)
    {
        case 0:
            return NromMapper::create(rom_data);
        default:
            return std::unexpected{ std::format("Unsupported mapper: id={}",
                                                rom_data.rom_info.mapper_id) };
    }
}

std::uint8_t DummyMapper::read_prg(std::uint16_t address)
{
    return 0x00;
}

void DummyMapper::write_prg(std::uint16_t address, std::uint8_t data)
{
}

std::uint8_t DummyMapper::read_chr(std::uint16_t address)
{
    return 0x00;
}

void DummyMapper::write_chr(std::uint16_t address, std::uint8_t data)
{
}

std::uint16_t DummyMapper::mirror_nametable_address(std::uint16_t address)
{
    return address;
}

std::expected<NromMapper, std::string> NromMapper::create(const RomData& rom_data) noexcept
{
    if (rom_data.rom_info.prg_rom_size != 1 * PRG_BANK_SIZE &&
        rom_data.rom_info.prg_rom_size != 2 * PRG_BANK_SIZE)
    {
        return std::unexpected{ "Invalid number of PRG banks" };
    }
    if (rom_data.rom_info.chr_rom_size != CHR_BANK_SIZE)
    {
        return std::unexpected{ "Invalid number of CHR banks" };
    }
    if (rom_data.prg_rom.size() != rom_data.rom_info.prg_rom_size)
    {
        return std::unexpected{ "PRG ROM size mismatch" };
    }
    if (rom_data.chr_rom.size() != rom_data.rom_info.chr_rom_size)
    {
        return std::unexpected{ "PRG ROM size mismatch" };
    }
    return NromMapper{ rom_data };
}

NromMapper::NromMapper(const RomData& rom_data) :
    prg_rom_{ rom_data.prg_rom },
    chr_rom_{ rom_data.chr_rom },
    address_mask_{ rom_data.rom_info.prg_rom_size == (1 * PRG_BANK_SIZE) ? MASK_16KB : MASK_32KB },
    mirroring_{ rom_data.rom_info.mirroring }
{
}

std::uint8_t NromMapper::read_prg(std::uint16_t address)
{
    return prg_rom_[address & address_mask_];
}

void NromMapper::write_prg(std::uint16_t address, std::uint8_t data)
{
}

std::uint8_t NromMapper::read_chr(std::uint16_t address)
{
    return chr_rom_[address];
}

void NromMapper::write_chr(std::uint16_t address, std::uint8_t data)
{
}

std::uint16_t NromMapper::mirror_nametable_address(std::uint16_t address)
{
    // https://www.nesdev.org/wiki/Mirroring#Nametable_Mirroring
    if (mirroring_ == Mirroring::HORIZONTAL)
    {
        return (address & 0x03FF) | ((address & 0x0800) >> 1);
    }
    return address & 0x07FF;
}

} // namespace mayones::core::rom

#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <vector>
#include <utility>

#include "cartridge.h"


const uint8_t Cartridge::HEADER_SIZE = 16;

Cartridge::Cartridge(const std::string& rom_path)
{
    std::filesystem::path file_path(rom_path);
    const auto file_size = std::filesystem::file_size(file_path);
    std::ifstream stream(rom_path, std::ios::binary);
    if (!stream.is_open())
    {
        std::cout << "error open file\n";
        return;
    }
    if (file_size < HEADER_SIZE)
    {
        std::cout << "mailformed rom: " << "rom_size=" << file_size << std::endl;
        return;
    }
    Header header{};
    stream.read(reinterpret_cast<char*>(&header), HEADER_SIZE);
    if (std::strncmp(header.title, "NES\x1A", 4) != 0)
    {
        std::cout << "invalid header title: " << header.title << std::endl;
        return;
    }
    const size_t prg_rom_size = 16 * 1024 * header.prg_banks;
    const size_t chr_rom_size = 8 * 1024 * header.chr_banks;
    if (HEADER_SIZE + prg_rom_size + chr_rom_size != file_size)
    {
        std::cout << "mailformed rom: " << "rom_size=" << file_size << std::endl;
        return;
    }

    mirroring = Mirroring(header.flags6 & 1);
    has_battery_ram = (header.flags6 >> 1 & 1) == 1;
    has_trainer = (header.flags6 >> 2 & 1) == 1;
    ignore_mirroring = (header.flags6 >> 3 & 1) == 1;
    uint8_t mapper_number = header.flags6 >> 4;

    console_type = ConsoleType(header.flags7 & 3);
    mapper_number |= header.flags7 & 0xF0;

    prg_ram_banks = header.flags8;

    tv_system = TVSystem(header.flags9 & 1);

    uint8_t tv_system_id = header.flags10 & 3;
    if (tv_system_id == 1 || tv_system_id == 3)
    {
        is_dual_tv_system = true;
    }
    else
    {
        is_dual_tv_system = false;
    }
    has_prg_ram = (header.flags10 >> 4 & 1) == 1;
    has_bus_conflicts = (header.flags10 >> 5 & 1) == 1;

    std::vector<uint8_t> prg_rom;
    prg_rom.resize(prg_rom_size);
    stream.read(reinterpret_cast<char*>(prg_rom.data()), prg_rom_size);

    std::vector<uint8_t> chr_rom;
    chr_rom.resize(prg_rom_size);
    chr_rom.resize(chr_rom_size);
    stream.read(reinterpret_cast<char*>(chr_rom.data()), chr_rom_size);

    if (mapper_number != 0)
    {
        std::cout << "unsupported mapper: " << mapper_number << std::endl;
        throw;
    }
    mapper = Mapper0(std::move(prg_rom), std::move(chr_rom));
    std::cout << "";
}

uint8_t Cartridge::read(uint16_t address) const
{
    return mapper.read(address);
}

void Cartridge::write(uint16_t address, uint8_t data)
{
    mapper.write(address, data);
}

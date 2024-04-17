#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "mapper.h"


class Cartridge
{
public:
    Cartridge(const std::string& rom_path);

    Cartridge() = delete;
    Cartridge(const Cartridge& cart) = delete;
    Cartridge& operator=(const Cartridge& cart) = delete;
    Cartridge(Cartridge&& cart) = delete;
    Cartridge& operator=(Cartridge&& cart) = delete;

    uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t data);

private:
    struct Header
    {
        char title[4];
        uint8_t prg_banks;
        uint8_t chr_banks;
        uint8_t flags6;
        uint8_t flags7;
        uint8_t flags8;
        uint8_t flags9;
        uint8_t flags10;
        uint8_t padding[5];
    };

    enum class Mirroring : uint8_t
    {
        HORIZONTAL = 0,
        VERTICAL = 1
    };

    enum class ConsoleType : uint8_t
    {
        FAMILY = 0,
        VS_SYSTEM = 1,
        PLAYCHOICE10 = 2,
        EXTENDED = 3
    };

    enum class TVSystem
    {
        NTSC = 0,
        PAL = 1
    };

    Mirroring mirroring;
    bool has_battery_ram;
    bool has_trainer;
    bool ignore_mirroring;

    Mapper0 mapper;
    //uint8_t mapper_number;

    ConsoleType console_type;

    uint8_t prg_ram_banks;

    TVSystem tv_system;
    bool is_dual_tv_system;

    bool has_prg_ram;
    bool has_bus_conflicts;

    //std::vector<uint8_t> prg_rom;
    //std::vector<uint8_t> chr_rom;

    static const uint8_t HEADER_SIZE;
};
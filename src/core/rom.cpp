#include "mayones/core/rom.hpp"

namespace mayones::core::rom {

std::string_view to_string(RomFormat rom_format)
{
    switch (rom_format)
    {
        case RomFormat::INES:
            return "iNES";
        case RomFormat::ARCHAIC_INES:
            return "Archaic iNES";
        case RomFormat::INES20:
            return "iNES 2.0";
        default:
            std::unreachable();
    }
}

std::string_view to_string(Mirroring mirroring)
{
    switch (mirroring)
    {
        case Mirroring::HORIZONTAL:
            return "horizontal";
        case Mirroring::VERTICAL:
            return "verical";
        default:
            std::unreachable();
    }
}

std::string_view to_string(ConsoleType console_type)
{
    switch (console_type)
    {
        case ConsoleType::NES:
            return "NES";
        case ConsoleType::PLAYCHOICE_10:
            return "PlayChoice-10";
        case ConsoleType::VS_UNISYSTEM:
            return "VS Unisystem";
        default:
            std::unreachable();
    }
}

std::string_view to_string(TVSystem tv_system)
{
    switch (tv_system)
    {
        case TVSystem::NTSC:
            return "RP2C02 (NTSC NES)";
        case TVSystem::PAL:
            return "RP2C07 (PAL NES)";
        case TVSystem::MULTIPLE_REGION:
            return "Multiple-region";
        case TVSystem::DENDY:
            return "UA6538 (Dendy)";
        default:
            std::unreachable();
    }
}

} // namespace mayones::core::rom

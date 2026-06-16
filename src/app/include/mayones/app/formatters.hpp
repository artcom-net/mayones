#pragma once

#include <concepts>
#include <format>
#include <string_view>

#include "config.hpp"
#include "mayones/core/cpu.hpp"
#include "mayones/core/rom.hpp"

template<typename T>
    requires(std::same_as<T, mayones::core::rom::RomFormat> ||
             std::same_as<T, mayones::core::rom::Mirroring> ||
             std::same_as<T, mayones::core::rom::ConsoleType> ||
             std::same_as<T, mayones::core::rom::TVSystem>) &&
            requires(T val) {
                { mayones::core::rom::to_string(val) } -> std::convertible_to<std::string_view>;
            }
struct std::formatter<T> {
    constexpr auto parse(auto& ctx)
    {
        return ctx.begin();
    }

    auto format(const T& val, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}", mayones::core::rom::to_string(val));
    }
};

template<>
struct std::formatter<mayones::app::Config> {
    constexpr auto parse(auto& ctx)
    {
        return ctx.begin();
    }

    auto format(const mayones::app::Config& config, auto& ctx) const
    {
        return std::format_to(ctx.out(),
                              "Config: debug={} rom_path={}",
                              config.debug,
                              config.rom_path.generic_string());
    }
};

template<>
struct std::formatter<mayones::core::rom::RomInfo> {
    constexpr auto parse(auto& ctx)
    {
        return ctx.begin();
    }

    auto format(const mayones::core::rom::RomInfo& rom_info, auto& ctx) const
    {
        return std::format_to(ctx.out(),
                              "RomInfo: \n"
                              "    format={}\n"
                              "    console={}\n"
                              "    tv_system={}\n"
                              "    mapper={}\n"
                              "    submapper={}\n"
                              "    mirroring={}\n\n"
                              "    prg_size={}\n"
                              "    chr_size={}\n"
                              "    prg_ram_size={}\n"
                              "    prg_nvram_size={}\n"
                              "    chr_ram_size={}\n"
                              "    chr_nvram_size={}\n\n"
                              "    has_battery={}\n"
                              "    has_trainer={}\n"
                              "    has_alternate_nt_layout={}",
                              rom_info.rom_format,
                              rom_info.console_type,
                              rom_info.tv_system,
                              rom_info.mapper_id,
                              rom_info.submapper_id,
                              rom_info.mirroring,
                              rom_info.prg_rom_size,
                              rom_info.chr_rom_size,
                              rom_info.prg_ram_size,
                              rom_info.prg_nvram_size,
                              rom_info.chr_ram_size,
                              rom_info.chr_nvram_size,
                              rom_info.has_battery,
                              rom_info.has_trainer,
                              rom_info.has_alternate_nt_layout);
    }
};

template<>
struct std::formatter<mayones::core::Cpu::TraceEntry> {
    constexpr auto parse(auto& ctx)
    {
        return ctx.begin();
    }

    auto format(const mayones::core::Cpu::TraceEntry& trace, auto& ctx) const
    {
        auto out_it = std::format_to(ctx.out(), "{:04X}  {:02X}", trace.pc, trace.opcode);

        if (std::holds_alternative<std::uint8_t>(trace.operand))
        {
            out_it = std::format_to(out_it, " {:02X}    ", std::get<std::uint8_t>(trace.operand));
        }
        else if (std::holds_alternative<std::uint16_t>(trace.operand))
        {
            out_it = std::format_to(out_it, " {:04X}  ", std::get<std::uint16_t>(trace.operand));
        }
        else
        {
            out_it = std::format_to(out_it, "{}", "       ");
        }

        return std::format_to(out_it,
                              "{}        A:{:02X} X:{:02X} Y:{:02X} P:{:02X} SP:{:02X} CYC:{}",
                              trace.mnemonic,
                              trace.a,
                              trace.x,
                              trace.y,
                              trace.p,
                              trace.sp,
                              trace.cycles);
    }
};

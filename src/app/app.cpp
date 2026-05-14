#include <filesystem>
#include <fstream>
#include <ios>
#include <print>
#include <string>
#include <utility>

#include "mayones/app/app.hpp"
#include "mayones/app/config.hpp"
#include "mayones/app/version.hpp"
#include "mayones/core/core.hpp"
#include "mayones/core/rom.hpp"

namespace {

using ReadFileResult = std::expected<std::vector<std::uint8_t>, std::string>;

ReadFileResult read_file(const std::filesystem::path& filepath)
{
    if (!std::filesystem::exists(filepath))
    {
        return std::unexpected{ "file doesn't exists: " + filepath.string() };
    }

    std::ifstream stream{ filepath, std::ios::binary };
    if (!stream.is_open())
    {
        return std::unexpected{ "open file error" };
    }

    auto file_size = std::filesystem::file_size(filepath);
    std::vector<std::uint8_t> buffer(file_size);
    stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size));

    if (!std::cmp_equal(stream.gcount(), file_size))
    {
        return std::unexpected{ "read file error" };
    }

    return buffer;
}

} // namespace

namespace mayones::app {

MayoNes::MayoNes(Config config) :
    m_config{ std::move(config) }
{
}

int MayoNes::run()
{
    std::println("MayoNES {}\nConfig:\n  rom_path={}\n  debug={}",
                 mayones::VERSION,
                 m_config.rom_path.generic_string(),
                 m_config.debug);

    auto read_result = read_file(m_config.rom_path);
    if (!read_result)
    {
        println("Error read ROM: {}", read_result.error());
        return 1;
    }

    if (auto load_result = m_nes_core.load_rom(std::move(read_result).value()); !load_result)
    {
        std::println("Error load ROM: {}", load_result.error());
        return 1;
    }

    const core::rom::RomInfo& rom_info = m_nes_core.rom_info();
    std::println("ROM:\n  name={}\n  header_id={::02x}\n  prg_banks={}\n  chr_rom_banks={}\n  "
                 "prg_ram_banks={}\n  mapper_id={:02x}",
                 m_config.rom_path.filename().string(),
                 rom_info.header_id,
                 rom_info.prg_rom_banks,
                 rom_info.chr_rom_banks,
                 rom_info.prg_ram_banks,
                 rom_info.mapper_id);

    return 0;
}

} // namespace mayones::app

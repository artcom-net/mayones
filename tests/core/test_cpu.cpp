#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "mayones/core/core.hpp"
#include "mayones/core/cpu.hpp"
#include "mayones/io/file.hpp"

using namespace std::string_view_literals;

namespace mayones::core {

static std::ostream& operator<<(std::ostream& os, const mayones::core::TraceEntry& trace)
{
    os << std::format("pc={:04X} oc={:02X}", trace.pc, trace.opcode);

    if (std::holds_alternative<std::uint8_t>(trace.operand))
    {
        os << std::format(" op={:02X}", std::get<std::uint8_t>(trace.operand));
    }
    else if (std::holds_alternative<std::uint16_t>(trace.operand))
    {
        os << std::format(" op={:04X}", std::get<std::uint16_t>(trace.operand));
    }
    else
    {
        os << "        ";
    }

    os << std::format(" mm={} a={:02X} x={:02X} y={:02X} p={:02X} sp={:02X} cyc={}",
                      trace.mnemonic,
                      trace.a,
                      trace.x,
                      trace.y,
                      trace.p,
                      trace.sp,
                      trace.cycles);

    return os;
}

} // namespace mayones::core

namespace {

using TraceBuffer = std::vector<mayones::core::TraceEntry>;

template<typename T>
T parse_number(std::string_view str_value, int base)
{
    T value{};
    auto [_, errc] =
      std::from_chars(str_value.data(), str_value.data() + str_value.size(), value, base);
    if (errc != std::errc())
    {
        throw;
    }
    return value;
}

template<typename T>
T parse_register_data(std::string_view reg_data, int base)
{
    return parse_number<T>(reg_data.substr(reg_data.find(':') + 1), base);
}

mayones::core::TraceEntry parse_trace_entry(const std::string& line)
{
    constexpr std::size_t MIN_TOKENS_COUNT{ 10 };

    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream{ line };

    while (stream >> token)
    {
        tokens.emplace_back(std::move(token));
    }

    if (tokens.size() < MIN_TOKENS_COUNT)
    {
        throw std::runtime_error{ "Not enough tokens in trace line" };
    }

    std::size_t token_idx{ 0 };

    auto pc = parse_number<std::uint16_t>(tokens[token_idx++], 16);
    auto opcode = parse_number<std::uint8_t>(tokens[token_idx++], 16);

    std::vector<std::uint8_t> operand_bytes;

    for (; tokens[token_idx].size() == 2; ++token_idx)
    {
        operand_bytes.push_back(parse_number<std::uint8_t>(tokens[token_idx], 16));
    }

    std::variant<std::monostate, std::uint16_t, std::uint8_t> operand;
    switch (operand_bytes.size())
    {
        case 0:
            break;
        case 1:
            operand = static_cast<std::uint8_t>(operand_bytes[0]);
            break;
        case 2:
            operand = static_cast<std::uint16_t>(operand_bytes[1] << 8 | operand_bytes[0]);
            break;
        default:
            throw std::runtime_error("Invalid instruction operands count");
    }

    std::string mnemonic;
    if (tokens[token_idx].starts_with('*'))
    {
        mnemonic = tokens[token_idx++].substr(1);
    }
    else
    {
        mnemonic = tokens[token_idx++];
    }

    for (; !tokens[token_idx].starts_with("A:"); ++token_idx)
    {
    }
    auto a = parse_register_data<std::uint8_t>(tokens[token_idx++], 16);
    auto x = parse_register_data<std::uint8_t>(tokens[token_idx++], 16);
    auto y = parse_register_data<std::uint8_t>(tokens[token_idx++], 16);
    auto p = parse_register_data<std::uint8_t>(tokens[token_idx++], 16);
    auto sp = parse_register_data<std::uint8_t>(tokens[token_idx++], 16);

    for (; !tokens[token_idx].starts_with("CYC:"); ++token_idx)
    {
    }
    auto cycles = parse_register_data<std::size_t>(tokens[token_idx], 10);

    return { .opcode = opcode,
             .mnemonic = mnemonic,
             .operand = operand,
             .a = a,
             .x = x,
             .y = y,
             .p = p,
             .sp = sp,
             .pc = pc,
             .cycles = cycles };
}

TraceBuffer parse_nestest_trace(std::vector<std::string> trace_lines)
{
    TraceBuffer trace;
    trace.reserve(trace_lines.size());

    for (const auto& line : trace_lines)
    {
        trace.emplace_back(parse_trace_entry(line));
    }

    return trace;
}

TraceBuffer run_nestest(std::vector<std::uint8_t> rom_data, std::size_t nestest_trace_size)
{
    constexpr std::uint16_t NESTEST_PC{ 0xC000 };

    mayones::core::NesCore nes_core{};
    auto load_rom_result = nes_core.load_rom(std::move(rom_data));
    if (!load_rom_result)
    {
        throw std::runtime_error{ "Error loading rom" };
    }

    TraceBuffer trace;
    trace.reserve(nestest_trace_size);

    nes_core.reset(NESTEST_PC);
    for (std::size_t i = 0; i < nestest_trace_size; ++i)
    {
        trace.emplace_back(nes_core.trace_tick_frame());
    }

    return trace;
}

} // namespace

TEST(Core, CpuNesTest)
{
    const char* nestest_rom_raw_path = nullptr;
    const char* nestest_trace_raw_path = nullptr;

    if (nestest_rom_raw_path = std::getenv("MAYONES_NESTEST_ROM"); !nestest_rom_raw_path)
    {
        FAIL() << "MAYONES_NESTEST_ROM environment variable is not set";
    }
    if (nestest_trace_raw_path = std::getenv("MAYONES_NESTEST_TRACE"); !nestest_trace_raw_path)
    {
        FAIL() << "MAYONES_NESTEST_TRACE environment variable is not set";
    }

    const std::filesystem::path nestest_rom_path{ nestest_rom_raw_path };
    const std::filesystem::path nestest_trace_path{ nestest_trace_raw_path };

    auto read_rom_result = mayones::io::read_binary_file(nestest_rom_path);
    if (!read_rom_result)
    {
        FAIL() << "Error reading nestest ROM: " + std::move(read_rom_result).error();
    }

    auto read_trace_result = mayones::io::read_text_file(nestest_trace_path);
    if (!read_trace_result)
    {
        FAIL() << "Error reading nestest trace: " + std::move(read_trace_result).error();
    }

    TraceBuffer nestest_trace = parse_nestest_trace(std::move(read_trace_result).value());
    TraceBuffer cpu_trace = run_nestest(std::move(read_rom_result).value(), nestest_trace.size());

    ASSERT_EQ(nestest_trace.size(), cpu_trace.size());
    for (const auto& [nestest_entry, cpu_entry] : std::views::zip(nestest_trace, cpu_trace))
    {
        ASSERT_EQ(nestest_entry, cpu_entry);
    }
}

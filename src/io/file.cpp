
#include <expected>
#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <string>
#include <utility>
#include <vector>

#include "mayones/io/file.hpp"

namespace {

using OpenFileResult = std::expected<std::ifstream, std::string>;

OpenFileResult open_file(const std::filesystem::path& filepath, std::ios::openmode flags)
{
    if (!std::filesystem::exists(filepath))
    {
        return std::unexpected{ "file doesn't exists: " + filepath.string() };
    }

    std::ifstream stream{ filepath, flags };
    if (!stream.is_open())
    {
        return std::unexpected{ "open file error" };
    }

    return stream;
}

} // namespace

namespace mayones::io {

ReadTextResult read_text_stream(std::istream& stream)
{
    std::string line;
    std::vector<std::string> buffer;
    while (std::getline(stream, line))
    {
        buffer.emplace_back(std::move(line));
    }
    return buffer;
}

ReadBytesResult read_binary_stream(std::istream& stream)
{
    stream.seekg(0, std::ios::end);
    auto stream_size = stream.tellg();
    if (stream_size <= 0)
    {
        return std::unexpected{ "Invalid stream size or empty" };
    }

    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> buffer(stream_size);
    stream.read(reinterpret_cast<char*>(buffer.data()), stream_size);
    if (stream.gcount() != stream_size)
    {
        return std::unexpected{ "Read stream error" };
    }

    return buffer;
}

ReadTextResult read_text_file(const std::filesystem::path& filepath)
{
    auto open_result = open_file(filepath, std::ios::in);
    if (!open_result)
    {
        return std::unexpected{ "Error open file: " + std::move(open_result).error() };
    }

    std::ifstream stream{ std::move(open_result).value() };
    return read_text_stream(stream);
}

ReadBytesResult read_binary_file(const std::filesystem::path& filepath)
{
    auto open_result = open_file(filepath, std::ios::in | std::ios::binary);
    if (!open_result)
    {
        return std::unexpected{ "Error open file: " + std::move(open_result).error() };
    }

    std::ifstream stream{ std::move(open_result).value() };
    return read_binary_stream(stream);
}

} // namespace mayones::io

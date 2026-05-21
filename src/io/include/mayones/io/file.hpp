#pragma once

#include <expected>
#include <filesystem>
#include <istream>
#include <string>
#include <vector>

namespace mayones::io {

using ReadTextResult = std::expected<std::vector<std::string>, std::string>;
using ReadBytesResult = std::expected<std::vector<std::uint8_t>, std::string>;

ReadTextResult read_text_stream(std::istream& stream);
ReadBytesResult read_binary_stream(std::istream& stream);

ReadTextResult read_text_file(const std::filesystem::path& filepath);
ReadBytesResult read_binary_file(const std::filesystem::path& filepath);

} // namespace mayones::io

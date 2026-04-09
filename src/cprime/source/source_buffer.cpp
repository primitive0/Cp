#include "source_buffer.hpp"

#include <array>
#include <format>
#include <fstream>
#include <istream>
#include <sstream>
#include <stdexcept>

namespace cprime::source {

namespace {

auto skip_utf8_bom(std::istream& input) -> void
{
    constexpr std::string_view kUtf8Bom = "\xEF\xBB\xBF";

    std::array<char, 3> bytes{};
    if (!input.read(bytes.data(), bytes.size())) {
        input.clear();
        input.seekg(0, std::ios::beg);
        return;
    }
    if (std::string_view{bytes.data(), bytes.size()} != kUtf8Bom) {
        input.clear();
        input.seekg(0, std::ios::beg);
    }
}

auto next_normalized_char(std::istream& input, char& ch) -> bool
{
    if (!input.get(ch)) {
        return false;
    }
    if (ch == '\r') {
        if (input.get(ch)) {
            if (ch != '\n') {
                input.unget();
            }
        } else {
            input.clear();
        }
        ch = '\n';
    }
    return true;
}

auto read_and_normalize_text(std::istream& input) -> std::string
{
    std::string result{};

    skip_utf8_bom(input);

    char ch{};
    bool at_line_beginning = true;
    while (next_normalized_char(input, ch)) {
        at_line_beginning = ch == '\n';
        result += ch;
    }
    if (input.bad()) {
        return std::string{};
    }
    if (!result.empty() && !at_line_beginning) {
        result += '\n';
    }

    return result;
}

} // namespace

auto SourceBuffer::from_file(const std::filesystem::path& path)
    -> std::unique_ptr<SourceBuffer>
{
    std::ifstream input{path, std::ios::binary};
    if (input.fail()) {
        throw std::runtime_error{
            std::format("failed to open file {}", path.native())};
    }

    std::string content = read_and_normalize_text(input);
    if (input.bad()) {
        throw std::runtime_error{
            std::format("failed to read file {}", path.native())};
    }

    auto source_buffer = std::make_unique<SourceBuffer>(ConstructionToken{});
    source_buffer->path_ = path;
    source_buffer->content_ = std::move(content);
    return source_buffer;
}

auto SourceBuffer::from_text(std::string_view text)
    -> std::unique_ptr<SourceBuffer>
{
    std::istringstream stream{std::string{text}};
    auto source_buffer = std::make_unique<SourceBuffer>(ConstructionToken{});
    source_buffer->content_ = read_and_normalize_text(stream);
    return source_buffer;
}

auto SourceBuffer::path() const -> const std::filesystem::path&
{
    return path_;
}

auto SourceBuffer::set_path(const std::filesystem::path& path) -> void
{
    path_ = path;
}

auto SourceBuffer::content() const -> std::string_view
{
    return content_;
}

auto SourceBuffer::begin() const -> const char*
{
    return content_.data();
}

auto SourceBuffer::end() const -> const char*
{
    return content_.data() + content_.size();
}

} // namespace cprime::source

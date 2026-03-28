#include "source_buffer.hpp"

#include <format>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace cprime::source {

auto SourceBuffer::from_file(const std::filesystem::path& path)
    -> std::unique_ptr<SourceBuffer>
{
    // Используем std::ios::binary, потому что SourceBuffer хранит исходный
    // текст как есть.
    std::ifstream input{path, std::ios::binary};
    if (input.fail()) {
        throw std::runtime_error{
            std::format("failed to open file {}", path.native())};
    }

    std::string content{std::istreambuf_iterator{input}, {}};
    if (input.fail()) {
        throw std::runtime_error{
            std::format("failed to read file {}", path.native())};
    }

    auto source_buffer = std::make_unique<SourceBuffer>(ConstructionToken{});
    source_buffer->path_ = path;
    source_buffer->content_ = std::move(content);
    return source_buffer;
}

auto SourceBuffer::from_content(std::string content)
    -> std::unique_ptr<SourceBuffer>
{
    auto source_buffer = std::make_unique<SourceBuffer>(ConstructionToken{});
    source_buffer->content_ = std::move(content);
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

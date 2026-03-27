#ifndef CPRIME_SOURCE_SOURCEBUFFER_H_
#define CPRIME_SOURCE_SOURCEBUFFER_H_

#include <filesystem>
#include <cprime/support/prelude.hpp>

namespace cprime::source {

class SourceBuffer final
{
private:
    struct ConstructionToken
    {
    };

public:
    explicit SourceBuffer(ConstructionToken) {}

    static auto from_file(const std::filesystem::path& path)
        -> std::unique_ptr<SourceBuffer>;
    static auto from_content(std::string content)
        -> std::unique_ptr<SourceBuffer>;

    auto path() const -> const std::filesystem::path&;
    auto set_path(const std::filesystem::path& path) -> void;

    auto content() const -> std::string_view;

    auto begin() const -> const char*;
    auto end() const -> const char*;

    auto cbegin() const -> const char*
    {
        return begin();
    }

    auto cend() const -> const char*
    {
        return end();
    }

private:
    SourceBuffer(const SourceBuffer&) = delete;
    SourceBuffer& operator=(const SourceBuffer&) = delete;

    SourceBuffer(SourceBuffer&&) = delete;
    SourceBuffer& operator=(SourceBuffer&&) = delete;

private:
    std::filesystem::path path_{};
    std::string content_{};
};

struct SourceLocation
{
    i32 line;
    i32 column;
};

class SourceSpan final
{
public:
    explicit SourceSpan(
        const SourceBuffer& buffer,
        SourceLocation start,
        std::string_view content)
        : buffer_{&buffer}
        , start_{start}
        , content_{content}

    {
    }

    auto buffer() const -> const SourceBuffer&
    {
        return *buffer_;
    }

    auto start() const -> SourceLocation
    {
        return start_;
    }

    auto content() const -> std::string_view
    {
        return content_;
    }

    auto begin() const -> const char*
    {
        return content_.begin();
    }

    auto end() const -> const char*
    {
        return content_.end();
    }

    auto cbegin() const -> const char*
    {
        return begin();
    }

    auto cend() const -> const char*
    {
        return end();
    }

private:
    const SourceBuffer* buffer_;
    SourceLocation start_;
    std::string_view content_;
};

} // namespace cprime::source

#endif // CPRIME_SOURCE_SOURCEBUFFER_H_

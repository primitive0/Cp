#ifndef CPRIME_SOURCE_SOURCEBUFFER_H_
#define CPRIME_SOURCE_SOURCEBUFFER_H_

#include <filesystem>
#include <cprime/support/prelude.hpp>
#include <cprime/support/contract.hpp>

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
    static auto from_text(std::string_view text)
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

struct LineColumn
{
    i32 line;
    i32 column;

    auto operator==(const LineColumn&) const -> bool = default;
};

class SourceSpan final
{
public:
    explicit SourceSpan(
        const SourceBuffer& buffer,
        LineColumn location,
        const char* begin,
        const char* end)
        : buffer_{&buffer}
        , location_{location}
        , ptr_{begin}
    {
        CPRIME_DEBUG_ASSERT(begin <= end);
        length_ = static_cast<size_t>(end - begin);
    }

    auto buffer() const -> const SourceBuffer&
    {
        return *buffer_;
    }

    auto location() const -> LineColumn
    {
        return location_;
    }

    auto size() const -> size_t
    {
        return length_;
    }

    auto begin() const -> const char*
    {
        return ptr_;
    }

    auto end() const -> const char*
    {
        return ptr_ + length_;
    }

    auto cbegin() const -> const char*
    {
        return begin();
    }

    auto cend() const -> const char*
    {
        return end();
    }

    auto content() const -> std::string_view
    {
        return std::string_view{ptr_, length_};
    }

private:
    const SourceBuffer* buffer_;
    LineColumn location_;
    const char* ptr_;
    size_t length_;
};

} // namespace cprime::source

#endif // CPRIME_SOURCE_SOURCEBUFFER_H_

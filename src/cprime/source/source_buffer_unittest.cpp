#include "source_buffer.hpp"

#include <stdexcept>
#include <cprime/support/temp_file.hpp>
#include <catch2/catch_test_macros.hpp>

namespace cprime::source {
namespace {

using cprime::support::TempFile;

TEST_CASE("SourceBuffer is created from file", "[cprime][source]")
{
    constexpr std::string_view kSourceText = "fn I32 main() { return 0; }\n";

    TempFile source_file = TempFile::with_content(kSourceText);

    std::unique_ptr<SourceBuffer> buffer =
        SourceBuffer::from_file(source_file.path());

    REQUIRE(buffer->path() == source_file.path());
    REQUIRE(buffer->content() == kSourceText);
    REQUIRE(buffer->cbegin() == buffer->begin());
    REQUIRE(buffer->cend() == buffer->end());
    REQUIRE(std::string_view{buffer->begin(), buffer->end()} == kSourceText);
}

TEST_CASE("SourceBuffer is created from content", "[cprime][source]")
{
    constexpr std::string_view kSourceText = "fn Unit foo() { println(); }\n";

    std::unique_ptr<SourceBuffer> buffer =
        SourceBuffer::from_content(std::string{kSourceText});

    REQUIRE(buffer->path().empty());
    REQUIRE(buffer->content() == kSourceText);
    REQUIRE(buffer->cbegin() == buffer->begin());
    REQUIRE(buffer->cend() == buffer->end());
    REQUIRE(std::string_view{buffer->begin(), buffer->end()} == kSourceText);
}

TEST_CASE("Can set SourceBuffer path", "[cprime][source]")
{
    constexpr std::string_view kSourcePath = "path/to/source.txt";
    constexpr std::string_view kSourceText =
        "fn I32 the_answer() { return 42; }\n";

    std::unique_ptr<SourceBuffer> buffer =
        SourceBuffer::from_content(std::string{kSourceText});

    REQUIRE(buffer->path().empty());

    buffer->set_path(kSourcePath);

    REQUIRE(buffer->path() == kSourcePath);
}

TEST_CASE("SourceBuffer can't open non-existent file", "[cprime][source]")
{
    REQUIRE_THROWS_AS(
        SourceBuffer::from_file("/this/does/not/exist"),
        std::runtime_error);
}

TEST_CASE("SourceSpan is constructed", "[cprime][source]")
{
    constexpr std::string_view kSourceText =
        "fn U8 fizz() { return '3'; }\n";

    std::unique_ptr<SourceBuffer> buffer =
        SourceBuffer::from_content(std::string{kSourceText});

    // ...U8 |fizz|()...
    SourceSpan span{
        *buffer,
        LineColumn{1, 7},
        buffer->content().begin() + 6,
        buffer->content().begin() + 10};

    REQUIRE(&span.buffer() == &*buffer);
    REQUIRE(span.location().line == 1);
    REQUIRE(span.location().column == 7);
    REQUIRE(span.content() == "fizz");
    REQUIRE(span.cbegin() == span.begin());
    REQUIRE(span.cend() == span.end());
    REQUIRE(std::string_view{span.begin(), span.end()} == "fizz");
}

} // namespace
} // namespace cprime::source

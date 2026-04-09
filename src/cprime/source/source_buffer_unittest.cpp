#include "source_buffer.hpp"

#include <stdexcept>
#include <string_view>
#include <cprime/support/temp_file.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

namespace cprime::source {
namespace {

using cprime::support::TempFile;

TEST_CASE("SourceBuffer is created", "[cprime][source]")
{
    struct TC
    {
        std::string_view input;
        std::string_view expected;
    };

    auto [input, expected] = GENERATE(
        TC{"", ""},
        TC{"fn I32 main() {}\n", "fn I32 main() {}\n"},
        TC{"\n", "\n"},
        TC{"\n\n\n", "\n\n\n"},
        TC{"\r\n", "\n"},
        TC{"\r\n\r\n\r\n", "\n\n\n"},
        TC{"\r", "\n"},
        TC{"\r\r\r", "\n\n\n"},
        TC{"\n\n\r\n\r\r", "\n\n\n\n\n"},
        TC{"Sample text\r", "Sample text\n"},
        TC{"Trailing line ending", "Trailing line ending\n"},
        TC{"\xEF\xBB\xBFHello, BOM!\n", "Hello, BOM!\n"},
        TC{"\xEF\xBB\xBF", ""},
        TC{"\xEF\xBB\xBE", "\xEF\xBB\xBE\n"},
        TC{"\xEF\xBC", "\xEF\xBC\n"});

    SECTION("From file")
    {
        TempFile source_file = TempFile::with_content(input);

        std::unique_ptr<SourceBuffer> buffer =
            SourceBuffer::from_file(source_file.path());

        REQUIRE(buffer->path() == source_file.path());
        REQUIRE(buffer->content() == expected);
        REQUIRE(buffer->cbegin() == buffer->begin());
        REQUIRE(buffer->cend() == buffer->end());
        REQUIRE(std::string_view{buffer->begin(), buffer->end()} == expected);
    }

    SECTION("From source text")
    {
        std::unique_ptr<SourceBuffer> buffer = SourceBuffer::from_text(input);

        REQUIRE(buffer->path().empty());
        REQUIRE(buffer->content() == expected);
        REQUIRE(buffer->cbegin() == buffer->begin());
        REQUIRE(buffer->cend() == buffer->end());
        REQUIRE(std::string_view{buffer->begin(), buffer->end()} == expected);
    }
}

TEST_CASE("Can set SourceBuffer path", "[cprime][source]")
{
    constexpr std::string_view kSourcePath = "path/to/source.txt";
    constexpr std::string_view kSourceText =
        "fn I32 the_answer() { return 42; }\n";

    std::unique_ptr<SourceBuffer> buffer =
        SourceBuffer::from_text(kSourceText);

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
        SourceBuffer::from_text(kSourceText);

    SECTION("Empty")
    {
        SourceSpan span{
            *buffer,
            LineColumn{1, 1},
            buffer->begin(),
            buffer->begin()};

        REQUIRE(&span.buffer() == &*buffer);
        REQUIRE(span.location().line == 1);
        REQUIRE(span.location().column == 1);
        REQUIRE(span.size() == 0);
        REQUIRE(span.cbegin() == span.begin());
        REQUIRE(span.cend() == span.end());
        REQUIRE(span.begin() == span.end());
        REQUIRE(span.content().empty());
    }

    SECTION("With some content")
    {
        // ...U8 |fizz|()...
        SourceSpan span{
            *buffer,
            LineColumn{1, 7},
            buffer->begin() + 6,
            buffer->begin() + 10};

        REQUIRE(&span.buffer() == &*buffer);
        REQUIRE(span.location().line == 1);
        REQUIRE(span.location().column == 7);
        REQUIRE(span.cbegin() == span.begin());
        REQUIRE(span.cend() == span.end());
        REQUIRE(std::string_view{span.begin(), span.end()} == "fizz");
        REQUIRE(span.content() == "fizz");
    }
}

} // namespace
} // namespace cprime::source

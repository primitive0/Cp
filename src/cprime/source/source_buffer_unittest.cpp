#include "source_buffer.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cprime/support/temp_file.hpp>

using namespace cprime::source;

using cprime::support::TempFile;

TEST_CASE("SourceBuffer is created from file", "[cprime][source]")
{
    constexpr std::string_view kSourceText = "fn I32 main() { return 0; }\n";

    TempFile source_file = TempFile::with_content(kSourceText);

    std::unique_ptr<SourceBuffer> buffer =
        SourceBuffer::from_file(source_file.path());

    CHECK(buffer->path() == source_file.path());
    CHECK(buffer->content() == kSourceText);
    CHECK(buffer->cbegin() == buffer->begin());
    CHECK(buffer->cend() == buffer->end());
    CHECK(std::string_view{buffer->begin(), buffer->end()} == kSourceText);
}

TEST_CASE("SourceBuffer is created from content", "[cprime][source]")
{
    constexpr std::string_view kSourceText = "fn Unit foo() { println(); }\n";

    std::unique_ptr<SourceBuffer> buffer =
        SourceBuffer::from_content(std::string{kSourceText});

    CHECK(buffer->path().empty());
    CHECK(buffer->content() == kSourceText);
    CHECK(buffer->cbegin() == buffer->begin());
    CHECK(buffer->cend() == buffer->end());
    CHECK(std::string_view{buffer->begin(), buffer->end()} == kSourceText);
}

TEST_CASE("Can set SourceBuffer path", "[cprime][source]")
{
    constexpr std::string_view kSourcePath = "path/to/source.txt";
    constexpr std::string_view kSourceText =
        "fn I32 the_answer() { return 42; }\n";

    std::unique_ptr<SourceBuffer> buffer =
        SourceBuffer::from_content(std::string{kSourceText});

    CHECK(buffer->path().empty());

    buffer->set_path(kSourcePath);

    CHECK(buffer->path() == kSourcePath);
}

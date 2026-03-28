#include "diagnostic_recorder.hpp"

#include <cprime/support/prelude.hpp>
#include <cprime/source/source_buffer.hpp>
#include <catch2/catch_test_macros.hpp>

namespace cprime::diagnostics {
namespace {

using cprime::source::LineColumn;
using cprime::source::SourceBuffer;
using cprime::source::SourceSpan;

TEST_CASE("DiagnosticRecorder is constructed empty", "[cprime][diagnostics]")
{
    DiagnosticRecorder recorder{};

    REQUIRE(recorder.count() == 0);
    REQUIRE(recorder.error_count() == 0);
    REQUIRE(recorder.warning_count() == 0);
    REQUIRE(!recorder.has_any());
    REQUIRE(!recorder.has_errors());
    REQUIRE(!recorder.has_warnings());
    REQUIRE(recorder.begin() == recorder.end());
    REQUIRE(recorder.cbegin() == recorder.cend());
}

TEST_CASE("DiagnosticRecorder records diagnostics", "[cprime][diagnostics]")
{
    std::unique_ptr<SourceBuffer> source_buffer =
        SourceBuffer::from_content("some text\nfoobar\n");

    DiagnosticRecorder recorder{};
    {
        SourceSpan span{
            *source_buffer,
            LineColumn{1, 1},
            source_buffer->begin(),
            source_buffer->begin() + 9};
        recorder.emit_error("Error message!", span);
    }
    {
        SourceSpan span{
            *source_buffer,
            LineColumn{2, 1},
            source_buffer->begin() + 10,
            source_buffer->begin() + 16};
        recorder.emit_warning("Warning message!", span);
    }

    REQUIRE(recorder.count() == 2);
    REQUIRE(recorder.error_count() == 1);
    REQUIRE(recorder.warning_count() == 1);
    REQUIRE(recorder.has_any());
    REQUIRE(recorder.has_errors());
    REQUIRE(recorder.has_warnings());

    std::span<const Diagnostic> diagnostics{recorder.begin(), recorder.end()};

    REQUIRE(diagnostics.size() == 2);

    REQUIRE(diagnostics[0].severity == Severity::Error);
    REQUIRE(&diagnostics[0].span.buffer() == &*source_buffer);
    REQUIRE(diagnostics[0].span.location() == LineColumn{1, 1});
    REQUIRE(diagnostics[0].span.content() == "some text");
    REQUIRE(diagnostics[0].message == "Error message!");

    REQUIRE(diagnostics[1].severity == Severity::Warning);
    REQUIRE(&diagnostics[1].span.buffer() == &*source_buffer);
    REQUIRE(diagnostics[1].span.location() == LineColumn{2, 1});
    REQUIRE(diagnostics[1].span.content() == "foobar");
    REQUIRE(diagnostics[1].message == "Warning message!");
}

TEST_CASE("DiagnosticRecorder distinguishes errors and warnings", "[cprime][diagnostics]")
{
    std::unique_ptr<SourceBuffer> source_buffer =
        SourceBuffer::from_content("another text");
    SourceSpan source_span{
        *source_buffer,
        LineColumn{1, 1},
        source_buffer->begin(),
        source_buffer->end()};

    DiagnosticRecorder recorder{};
    recorder.emit_warning("This is warning description.", source_span);

    REQUIRE(recorder.count() == 1);
    REQUIRE(recorder.error_count() == 0);
    REQUIRE(recorder.warning_count() == 1);
    REQUIRE(recorder.has_any());
    REQUIRE(!recorder.has_errors());
    REQUIRE(recorder.has_warnings());
    REQUIRE(recorder.begin() != recorder.end());
    REQUIRE(recorder.cbegin() != recorder.cend());

    recorder.emit_error("This is error description.", source_span);

    REQUIRE(recorder.count() == 2);
    REQUIRE(recorder.error_count() == 1);
    REQUIRE(recorder.warning_count() == 1);
    REQUIRE(recorder.has_any());
    REQUIRE(recorder.has_errors());
    REQUIRE(recorder.has_warnings());
    REQUIRE(recorder.begin() != recorder.end());
    REQUIRE(recorder.cbegin() != recorder.cend());
}

TEST_CASE("Can reset DiagnosticRecorder", "[cprime][diagnostics]")
{
    std::unique_ptr<SourceBuffer> source_buffer =
        SourceBuffer::from_content("yet another text");
    SourceSpan source_span{
        *source_buffer,
        LineColumn{1, 1},
        source_buffer->begin(),
        source_buffer->end()};

    DiagnosticRecorder recorder{};
    recorder.emit_warning("1warning", source_span);
    recorder.emit_error("2error", source_span);
    recorder.emit_error("3error", source_span);

    REQUIRE(recorder.count() == 3);
    REQUIRE(recorder.error_count() == 2);
    REQUIRE(recorder.warning_count() == 1);
    REQUIRE(recorder.has_any());
    REQUIRE(recorder.has_errors());
    REQUIRE(recorder.has_warnings());
    REQUIRE(recorder.begin() != recorder.end());
    REQUIRE(recorder.cbegin() != recorder.cend());

    recorder.reset();

    REQUIRE(recorder.count() == 0);
    REQUIRE(recorder.error_count() == 0);
    REQUIRE(recorder.warning_count() == 0);
    REQUIRE(!recorder.has_any());
    REQUIRE(!recorder.has_errors());
    REQUIRE(!recorder.has_warnings());
    REQUIRE(recorder.begin() == recorder.end());
    REQUIRE(recorder.cbegin() == recorder.cend());
}

} // namespace
} // namespace cprime::diagnostics

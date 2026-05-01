#ifndef CPRIME_LEX_LEX_H_
#define CPRIME_LEX_LEX_H_

#include <optional>
#include <cprime/support/prelude.hpp>
#include <cprime/diagnostics/diagnostic_sink.hpp>
#include <cprime/lex/token.hpp>
#include <cprime/source/source_buffer.hpp>

namespace cprime::lex {

class Lexer final
{
public:
    Lexer(const Lexer&) = delete;
    Lexer& operator=(const Lexer&) = delete;

    Lexer(Lexer&&) = default;
    Lexer& operator=(Lexer&&) = default;

    explicit Lexer(
        const source::SourceBuffer& source_buffer,
        diagnostics::IDiagnosticSink& diagnostic_sink);

    auto next() -> Token;

private:
    auto parse_keyword_or_identifier() -> Token;
    auto parse_integer_literal() -> Token;
    auto parse_string_literal() -> Token;
    auto parse_escape_sequence() -> std::optional<std::string_view>;

    auto on_error() -> Token;

    auto capture_and_emit(TokenKind kind) -> Token;
    auto skip_whitespaces_and_comments() -> void;
    auto try_parse_comment() -> bool;

    // TODO: refactor this function
    auto diagnose_invalid_utf8(u8 byte, source::SourceSpan span) -> void;

    [[nodiscard]] auto cursor_span() const -> source::SourceSpan;
    auto advance() -> void;
    [[nodiscard]] auto capture() -> source::SourceSpan;
    auto decode_next() -> void;

private:
    static constexpr char32_t kNoChar = 0xFFFFFFFF;
    static constexpr char32_t kInvalidChar = 0xFFFFFFFE;

    const source::SourceBuffer* source_buffer_;
    source::LineColumn anchor_location_;
    source::LineColumn cursor_location_;
    const char* anchor_;
    const char* cursor_;
    const char* cursor_end_;
    const char* end_;
    char32_t ch_;
    diagnostics::IDiagnosticSink* diagnostic_sink_;
};

} // namespace cprime::lex

#endif // CPRIME_LEX_LEX_H_

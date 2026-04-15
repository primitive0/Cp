#ifndef CPRIME_LEX_LEX_H_
#define CPRIME_LEX_LEX_H_

#include <optional>
#include <cprime/support/prelude.hpp>
#include <cprime/diagnostics/diagnostic_sink.hpp>
#include <cprime/lex/token.hpp>
#include <cprime/source/source_buffer.hpp>

namespace cprime::lex {

class TextScanner final
{
public:
    static constexpr char32_t kNoChar = 0xFFFFFFFF;
    static constexpr char32_t kInvalidChar = 0xFFFFFFFE;

    explicit TextScanner(const source::SourceBuffer& source_buffer);

    [[nodiscard]] auto peek() const -> char32_t;
    [[nodiscard]] auto peek_raw() const -> std::string_view;
    [[nodiscard]] auto peek_span() const -> source::SourceSpan;
    [[nodiscard]] auto peek_invalid_byte() const -> u8;

    auto advance() -> void;

    // auto match(std::string_view) -> bool;

    [[nodiscard]] auto capture() -> source::SourceSpan;

private:
    auto decode_next() -> void;

private:
    const source::SourceBuffer* source_buffer_;
    source::LineColumn anchor_location_;
    source::LineColumn cursor_location_;
    const char* anchor_;
    const char* cursor_;
    const char* cursor_end_;
    const char* end_;
    char32_t cursor_char_;
};

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
    auto skip_spaces() -> void;

    auto diagnose_invalid_utf8(u8 byte, source::SourceSpan span) -> void;

private:
    TextScanner scanner_;
    diagnostics::IDiagnosticSink* diagnostic_sink_;
};

} // namespace cprime::lex

#endif // CPRIME_LEX_LEX_H_

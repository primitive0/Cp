#include "lex.hpp"

#include <cassert>
#include <charconv>
#include <format>
#include <system_error>
#include <utf8.h>
#include <cprime/source/source_buffer.hpp>
#include <cprime/support/text.hpp>

namespace cprime::lex {

using source::LineColumn;
using source::SourceBuffer;
using source::SourceSpan;

namespace {

auto get_keyword_token_kind(std::string_view lexeme)
    -> std::optional<TokenKind>
{
    if (lexeme == "fn") {
        return TokenKind::Fn;
    } else if (lexeme == "return") {
        return TokenKind::Return;
    } else {
        return std::nullopt;
    }
}

} // namespace

TextScanner::TextScanner(const SourceBuffer& source_buffer)
    : source_buffer_{&source_buffer}
    , anchor_location_{1, 1}
    , cursor_location_{1, 1}
    , anchor_{source_buffer.begin()}
    , cursor_{source_buffer.begin()}
    , cursor_end_{source_buffer.begin()}
    , end_{source_buffer.end()}
    , cursor_char_{kNoChar}
{
    decode_next();
}

auto TextScanner::peek() const -> char32_t
{
    return cursor_char_;
}

auto TextScanner::peek_raw() const -> std::string_view
{
    return std::string_view{cursor_, cursor_end_};
}

auto TextScanner::peek_span() const -> SourceSpan
{
    return SourceSpan{
        *source_buffer_,
        cursor_location_,
        cursor_,
        cursor_end_};
}

auto TextScanner::peek_invalid_byte() const -> u8
{
    assert(cursor_char_ == kInvalidChar &&
           "Current cursor char must be invalid.");
    return static_cast<u8>(*cursor_);
}

auto TextScanner::advance() -> void
{
    if (cursor_char_ == kNoChar) {
        return;
    }

    if (cursor_char_ == '\n') {
        ++cursor_location_.line;
        cursor_location_.column = 1;
    } else {
        ++cursor_location_.column;
    }

    cursor_ = cursor_end_;
    decode_next();
}

auto TextScanner::capture() -> SourceSpan
{
    SourceSpan span{
        *source_buffer_,
        anchor_location_,
        anchor_,
        cursor_};

    anchor_location_ = cursor_location_;
    anchor_ = cursor_;

    return span;
}

auto TextScanner::decode_next() -> void
{
    if (cursor_end_ == end_) {
        cursor_char_ = kNoChar;
        return;
    }
    try {
        cursor_char_ = utf8::next(cursor_end_, end_);
    } catch (const utf8::exception&) {
        cursor_char_ = kInvalidChar;
        ++cursor_end_;
    }
}

Lexer::Lexer(
    const source::SourceBuffer& source_buffer,
    diagnostics::IDiagnosticSink& diagnostic_sink)
    : scanner_{source_buffer}
    , diagnostic_sink_{&diagnostic_sink}
{
}

auto Lexer::next() -> Token
{
    skip_whitespaces_and_comments();

    char32_t ch = scanner_.peek();
    if (ch == TextScanner::kNoChar) {
        // TODO: is this correct implementation?
        return capture_and_emit(TokenKind::Eof);
    }
    if (support::text::utf8::is_ascii_alphabetic(ch) || ch == U'_') {
        return parse_keyword_or_identifier();
    }
    if (support::text::utf8::is_ascii_digit(ch)) {
        return parse_integer_literal();
    }
    if (ch == U'"') {
        return parse_string_literal();
    }

    // TODO: handle single-char tokens correctly
    switch (ch) {
    case U';':
        scanner_.advance();
        return capture_and_emit(TokenKind::Semicolon);
    case U',':
        scanner_.advance();
        return capture_and_emit(TokenKind::Comma);
    case U'(':
        scanner_.advance();
        return capture_and_emit(TokenKind::ParenOpen);
    case U')':
        scanner_.advance();
        return capture_and_emit(TokenKind::ParenClose);
    case U'{':
        scanner_.advance();
        return capture_and_emit(TokenKind::BraceOpen);
    case U'}':
        scanner_.advance();
        return capture_and_emit(TokenKind::BraceClose);
    case U'\n':
        scanner_.advance();
        return capture_and_emit(TokenKind::NewLine);
    }

    return on_error();
}

auto Lexer::parse_keyword_or_identifier() -> Token
{
    scanner_.advance();
    char32_t ch = scanner_.peek();
    while (support::text::utf8::is_ascii_alphanumeric(ch) || ch == U'_') {
        scanner_.advance();
        ch = scanner_.peek();
    }

    SourceSpan span = scanner_.capture();
    auto kind = get_keyword_token_kind(span.content())
                    .value_or(TokenKind::Identifier);
    return Token{kind, span};
}

auto Lexer::parse_integer_literal() -> Token
{
    bool first_char_is_zero = scanner_.peek() == '0';
    scanner_.advance();
    bool has_leading_zeros = first_char_is_zero &&
                             support::text::utf8::is_ascii_digit(
                                 scanner_.peek());
    while (support::text::utf8::is_ascii_digit(scanner_.peek())) {
        scanner_.advance();
    }

    SourceSpan span = scanner_.capture();

    if (has_leading_zeros) {
        diagnostic_sink_->emit_error(
            "leading zeros are not allowed in integer literals",
            span);
        return Token{TokenKind::IntegerLiteral, span};
    }

    i64 value{};
    auto [it, error_code] = std::from_chars(span.begin(), span.end(), value);
    if (error_code != std::errc{}) {
        diagnostic_sink_->emit_error(
            "integer literal is too large",
            span);
        return Token{TokenKind::IntegerLiteral, span};
    }
    // Лексема включает в себя только цифры, поэтому должна быть полностью
    // разобрана std::from_chars().
    assert(it == span.end());

    return Token{TokenKind::IntegerLiteral, span, value};
}

auto Lexer::parse_string_literal() -> Token
{
    scanner_.advance();

    std::string value{};
    bool has_errors = false;
    while (true) {
        char32_t ch = scanner_.peek();
        assert(ch != TextScanner::kNoChar);

        switch (ch) {
        case U'"': {
            scanner_.advance();
            SourceSpan span = scanner_.capture();
            if (has_errors) {
                return Token{TokenKind::StringLiteral, span};
            } else {
                return Token{TokenKind::StringLiteral, span, std::move(value)};
            }
        }

        case U'\n': {
            SourceSpan span = scanner_.capture();
            diagnostic_sink_->emit_error("unclosed string literal", span);
            return Token{TokenKind::StringLiteral, span};
        }

        case U'\\': {
            if (auto decoded = parse_escape_sequence(); decoded) {
                value.append(*decoded);
            } else {
                has_errors = true;
            }
            break;
        }

        default: {
            value.append(scanner_.peek_raw());
            scanner_.advance();
            break;
        }
        }
    }
}

// TODO: update spec to include new sequences
// TODO: add \" to spec
auto Lexer::parse_escape_sequence() -> std::optional<std::string_view>
{
    using namespace std::literals::string_view_literals;

    scanner_.advance();

    char32_t ch = scanner_.peek();
    if (ch == U'\n') {
        return std::nullopt;
    }
    SourceSpan escape_span = scanner_.peek_span();
    scanner_.advance();
    switch (ch) {
    case U'n':
        return "\n";
    case U'r':
        return "\r";
    case U't':
        return "\t";
    case U'0':
        return "\0"sv;
    case U'"':
        return "\"";
    case U'\\':
        return "\\";
    default:
        // TODO: better error message
        // need message exactly at escape sequence position
        diagnostic_sink_->emit_error("unknown escape sequence", escape_span);
        return std::nullopt;
    }
}

auto Lexer::on_error() -> Token
{
    if (scanner_.peek() == TextScanner::kInvalidChar) {
        diagnose_invalid_utf8(
            scanner_.peek_invalid_byte(),
            scanner_.peek_span());
    } else {
        // TODO: better character formatting
        diagnostic_sink_->emit_error(
            std::format("unexpected character '{}'", scanner_.peek_raw()),
            scanner_.peek_span());
    }
    scanner_.advance();
    return capture_and_emit(TokenKind::Error);
}

auto Lexer::capture_and_emit(TokenKind kind) -> Token
{
    return Token{kind, scanner_.capture()};
}

auto Lexer::skip_whitespaces_and_comments() -> void
{
    while (true) {
        switch (scanner_.peek()) {
        case U' ':
            scanner_.advance();
            break;
        case U'/':
            if (!try_parse_comment()) {
                std::ignore = scanner_.capture();
                return;
            }
            break;
        default:
            std::ignore = scanner_.capture();
            return;
        }
    }
}

auto Lexer::try_parse_comment() -> bool
{
    // FIXME: this function does not support operator "/"

    scanner_.advance();
    assert(scanner_.peek() == U'*');
    scanner_.advance();
    while (true) {
        switch (scanner_.peek()) {
        case TextScanner::kInvalidChar:
            // FIXME: diagnostic span
            diagnose_invalid_utf8(
                scanner_.peek_invalid_byte(),
                scanner_.capture());
            scanner_.advance();
            break;

        case U'*':
            scanner_.advance();
            if (scanner_.peek() == '/') {
                scanner_.advance();
                return true;
            }
            break;

        case TextScanner::kNoChar:
            // FIXME: diagnostic span
            diagnostic_sink_->emit_error(
                "unclosed comment",
                scanner_.capture());
            return true;

        default:
            scanner_.advance();
            break;
        }
    }
}

auto Lexer::diagnose_invalid_utf8(u8 byte, SourceSpan span) -> void
{
    diagnostic_sink_->emit_error(
        std::format("invalid UTF-8 byte 0x{:02X}", byte),
        span);
}

} // namespace cprime::lex

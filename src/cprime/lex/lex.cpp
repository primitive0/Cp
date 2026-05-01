#include "lex.hpp"

#include <cassert>
#include <charconv>
#include <format>
#include <system_error>
#include <utf8.h>
#include <cprime/source/source_buffer.hpp>
#include <cprime/support/text.hpp>

// TODO: проверить, что advance() первого символа X происходит внутри
// всех parse_X()

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

Lexer::Lexer(
    const source::SourceBuffer& source_buffer,
    diagnostics::IDiagnosticSink& diagnostic_sink)
    : source_buffer_{&source_buffer}
    , anchor_location_{1, 1}
    , cursor_location_{1, 1}
    , anchor_{source_buffer.begin()}
    , cursor_{source_buffer.begin()}
    , cursor_end_{source_buffer.begin()}
    , end_{source_buffer.end()}
    , ch_{kNoChar}
    , diagnostic_sink_{&diagnostic_sink}
{
    decode_next();
}

auto Lexer::next() -> Token
{
    skip_whitespaces_and_comments();

    if (ch_ == kNoChar) {
        // TODO: is this correct implementation?
        return capture_and_emit(TokenKind::Eof);
    }
    if (support::text::utf8::is_ascii_alphabetic(ch_) || ch_ == U'_') {
        return parse_keyword_or_identifier();
    }
    if (support::text::utf8::is_ascii_digit(ch_)) {
        return parse_integer_literal();
    }
    if (ch_ == U'"') {
        return parse_string_literal();
    }

    // TODO: handle single-char tokens correctly
    switch (ch_) {
    case U';':
        advance();
        return capture_and_emit(TokenKind::Semicolon);
    case U',':
        advance();
        return capture_and_emit(TokenKind::Comma);
    case U'(':
        advance();
        return capture_and_emit(TokenKind::ParenOpen);
    case U')':
        advance();
        return capture_and_emit(TokenKind::ParenClose);
    case U'{':
        advance();
        return capture_and_emit(TokenKind::BraceOpen);
    case U'}':
        advance();
        return capture_and_emit(TokenKind::BraceClose);
    case U'\n':
        advance();
        return capture_and_emit(TokenKind::NewLine);
    }

    return on_error();
}

auto Lexer::parse_keyword_or_identifier() -> Token
{
    do {
        advance();
    } while (support::text::utf8::is_ascii_alphanumeric(ch_) || ch_ == U'_');

    SourceSpan span = capture();
    auto kind = get_keyword_token_kind(span.content())
                    .value_or(TokenKind::Identifier);
    return Token{kind, span};
}

auto Lexer::parse_integer_literal() -> Token
{
    bool first_char_is_zero = ch_ == '0';
    advance();
    bool has_leading_zeros = first_char_is_zero &&
                             support::text::utf8::is_ascii_digit(ch_);
    while (support::text::utf8::is_ascii_digit(ch_)) {
        advance();
    }

    SourceSpan span = capture();

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
    advance();

    std::string value{};
    bool has_errors = false;
    while (true) {
        assert(ch_ != kNoChar);

        switch (ch_) {
        case U'"': {
            advance();
            SourceSpan span = capture();
            if (has_errors) {
                return Token{TokenKind::StringLiteral, span};
            } else {
                return Token{TokenKind::StringLiteral, span, std::move(value)};
            }
        }

        case U'\n': {
            SourceSpan span = capture();
            diagnostic_sink_->emit_error("unclosed string literal", span);
            return Token{TokenKind::StringLiteral, span};
        }

        case U'\\': {
            if (auto decoded = parse_escape_sequence()) {
                value.append(*decoded);
            } else {
                has_errors = true;
            }
            break;
        }

        default: {
            value.append(cursor_, cursor_end_);
            advance();
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

    const char* escape_sequence_start = cursor_;
    LineColumn escape_sequence_location = cursor_location_;
    advance();

    // Не диагностируем внезапный конец строкового литерала здесь, а
    // полагаемся на parse_string_literal(). Некорректные символы также
    // диагностируются в parse_string_literal().
    assert(ch_ != kNoChar);
    if (ch_ == U'\n' || ch_ == kInvalidChar) {
        return std::nullopt;
    }
    char32_t designator = ch_;
    advance();

    switch (designator) {
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
        SourceSpan span{
            *source_buffer_,
            escape_sequence_location,
            escape_sequence_start,
            cursor_};
        diagnostic_sink_->emit_error(
            std::format("unknown escape sequence '{}'", span.content()),
            span);
        return std::nullopt;
    }
}

auto Lexer::on_error() -> Token
{
    assert(ch_ != U'\n' && ch_ != kNoChar);

    if (ch_ == kInvalidChar) {
        diagnose_invalid_utf8(static_cast<u8>(*cursor_), cursor_span());
        advance();
        return capture_and_emit(TokenKind::Error);
    }

    // TODO: better character formatting
    SourceSpan span = cursor_span();
    diagnostic_sink_->emit_error(
        std::format("unexpected character '{}'", span.content()),
        span);
    advance();
    return capture_and_emit(TokenKind::Error);
}

auto Lexer::capture_and_emit(TokenKind kind) -> Token
{
    return Token{kind, capture()};
}

auto Lexer::skip_whitespaces_and_comments() -> void
{
    while (true) {
        switch (ch_) {
        case U' ':
            advance();
            break;
        case U'/':
            if (!try_parse_comment()) {
                std::ignore = capture();
                return;
            }
            break;
        default:
            std::ignore = capture();
            return;
        }
    }
}

auto Lexer::try_parse_comment() -> bool
{
    // FIXME: this function does not support operator "/"

    advance();
    assert(ch_ == U'*');
    advance();
    while (true) {
        switch (ch_) {
        case kInvalidChar:
            diagnose_invalid_utf8(static_cast<u8>(*cursor_), cursor_span());
            advance();
            break;

        case U'*':
            advance();
            if (ch_ == '/') {
                advance();
                return true;
            }
            break;

        case kNoChar:
            diagnostic_sink_->emit_error(
                "unclosed comment",
                cursor_span());
            return true;

        default:
            advance();
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

auto Lexer::cursor_span() const -> SourceSpan
{
    return SourceSpan{
        *source_buffer_,
        cursor_location_,
        cursor_,
        cursor_end_};
}

auto Lexer::advance() -> void
{
    if (ch_ == kNoChar) {
        return;
    }

    if (ch_ == '\n') {
        ++cursor_location_.line;
        cursor_location_.column = 1;
    } else {
        ++cursor_location_.column;
    }

    cursor_ = cursor_end_;
    decode_next();
}

auto Lexer::capture() -> SourceSpan
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

auto Lexer::decode_next() -> void
{
    if (cursor_end_ == end_) {
        ch_ = kNoChar;
        return;
    }
    try {
        ch_ = utf8::next(cursor_end_, end_);
    } catch (const utf8::exception&) {
        ch_ = kInvalidChar;
        ++cursor_end_;
    }
}

} // namespace cprime::lex

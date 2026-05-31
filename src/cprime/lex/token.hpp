#ifndef CPRIME_LEX_TOKEN_H_
#define CPRIME_LEX_TOKEN_H_

#include <variant>
#include <cprime/support/prelude.hpp>
#include <cprime/source/source_buffer.hpp>
#include <cprime/support/contract.hpp>

namespace cprime::lex {

enum class TokenKind {
    // Ключевые слова
    Fn,
    Return,

    Identifier,

    // Операторы
    Plus,
    Minus,
    Star,
    Slash,
    Percent,

    // Пунктуация
    ParenOpen,
    ParenClose,
    BraceOpen,
    BraceClose,
    Semicolon,
    Comma,

    // Литералы
    StringLiteral,
    IntegerLiteral,

    NewLine,
    Eof,
    Error,
};

using TokenValue = std::variant<std::monostate, std::string, i64>;

class Token final
{
public:
    explicit Token(TokenKind kind, source::SourceSpan span)
        : kind_{kind}
        , span_{span}
        , computed_value_{std::monostate{}}
    {
    }

    explicit Token(
        TokenKind kind,
        source::SourceSpan span,
        std::string computed_value)
        : kind_{kind}
        , span_{span}
        , computed_value_{std::move(computed_value)}
    {
        CPRIME_DEBUG_ASSERT(kind_ == TokenKind::StringLiteral);
    }

    explicit Token(
        TokenKind kind,
        source::SourceSpan span,
        i64 computed_value)
        : kind_{kind}
        , span_{span}
        , computed_value_{computed_value}
    {
        CPRIME_DEBUG_ASSERT(kind_ == TokenKind::IntegerLiteral);
    }

    auto kind() const -> TokenKind
    {
        return kind_;
    }

    auto lexeme() const -> std::string_view
    {
        return span_.content();
    }

    auto span() const -> const source::SourceSpan&
    {
        return span_;
    }

    auto is_valid() const -> bool
    {
        switch (kind_) {
        case TokenKind::Error:
            return false;
        case TokenKind::StringLiteral:
            return std::holds_alternative<std::string>(computed_value_);
        case TokenKind::IntegerLiteral:
            return std::holds_alternative<i64>(computed_value_);
        default:
            return true;
        }
    }

    auto string_value() const -> std::string_view
    {
        CPRIME_DEBUG_ASSERT(
            kind_ == TokenKind::StringLiteral,
            "Token must be a valid string literal.");
        return std::get<std::string>(computed_value_);
    }

    auto integer_value() const -> i64
    {
        CPRIME_DEBUG_ASSERT(
            kind_ == TokenKind::IntegerLiteral,
            "Token must be a valid integer literal.");
        return std::get<i64>(computed_value_);
    }

    auto computed_value() const -> const TokenValue&
    {
        return computed_value_;
    }

private:
    TokenKind kind_;
    source::SourceSpan span_;
    TokenValue computed_value_;
};

// TODO: Name is too generic. Rename.
// TODO: return string_view or accept Token instead of TokenKind?
inline auto describe(TokenKind kind) -> std::string
{
    using enum TokenKind;

    switch (kind) {
    case Fn:
        return "'fn'";
    case Return:
        return "'return'";

    case Identifier:
        return "identifier";

    case Plus:
        return "'+'";
    case Minus:
        return "'-'";
    case Star:
        return "'*'";
    case Slash:
        return "'/'";
    case Percent:
        return "'%'";

    case ParenOpen:
        return "'('";
    case ParenClose:
        return "')'";
    case BraceOpen:
        return "'{'";
    case BraceClose:
        return "'}'";
    case Semicolon:
        return "';'";
    case Comma:
        return "','";

    case StringLiteral:
        return "string literal";
    case IntegerLiteral:
        return "integer literal";

    case NewLine:
        return "newline";
    case Eof:
        return "end of file";
    case Error:
        return "invalid token";
    }

    return "<UNKNOWN TOKEN>";
}

} // namespace cprime::lex

#endif // CPRIME_LEX_TOKEN_H_

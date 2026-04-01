#ifndef CPRIME_LEX_TOKEN_H_
#define CPRIME_LEX_TOKEN_H_

#include <cassert>
#include <variant>
#include <cprime/support/prelude.hpp>
#include <cprime/source/source_buffer.hpp>

namespace cprime::lex {

enum class TokenKind {
    // Ключевые слова
    Fn,
    Return,

    Identifier,

    // Пунктуация
    ParenOpen,
    ParenClose,
    BraceOpen,
    BraceClose,
    Semicolon,

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
        assert(kind_ == TokenKind::StringLiteral);
    }

    explicit Token(
        TokenKind kind,
        source::SourceSpan span,
        i64 computed_value)
        : kind_{kind}
        , span_{span}
        , computed_value_{computed_value}
    {
        assert(kind_ == TokenKind::IntegerLiteral);
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
        assert(kind_ == TokenKind::StringLiteral &&
               "Token must be a valid string literal.");
        return std::get<std::string>(computed_value_);
    }

    auto integer_value() const -> i64
    {
        assert(kind_ == TokenKind::IntegerLiteral &&
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

} // namespace cprime::lex

#endif // CPRIME_LEX_TOKEN_H_

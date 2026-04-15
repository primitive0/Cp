#include "parse.hpp"

#include <format>
#include <cprime/ast/ast.hpp>
#include <cprime/diagnostics/diagnostic_sink.hpp>
#include <cprime/lex/lex.hpp>
#include <cprime/lex/token.hpp>
#include <cprime/source/source_buffer.hpp>

namespace cprime::parse {

using diagnostics::IDiagnosticSink;
using lex::Lexer;
using lex::Token;
using lex::TokenKind;
using source::SourceBuffer;

namespace {

class TokenStream final
{
public:
    explicit TokenStream(Lexer lexer)
        : lexer_{std::move(lexer)}
        , token_{lexer_.next()}
    {
    }

    // TODO: handle invalid tokens
    [[nodiscard]]
    auto peek() const -> const Token&
    {
        return token_;
    }

    [[nodiscard]]
    auto skip_newlines_and_peek() -> const Token&
    {
        skip_newlines();
        return peek();
    }

    auto skip_newlines() -> void
    {
        while (peek().kind() == TokenKind::NewLine) {
            advance();
        }
    }

    auto advance() -> void
    {
        token_ = lexer_.next();
    }

private:
    Lexer lexer_;
    Token token_;
};

class ParseAbort final
{
};

class Parser final
{
public:
    explicit Parser(
        const SourceBuffer& source_buffer,
        IDiagnosticSink& diagnostic_sink)
        : stream_{Lexer{source_buffer, diagnostic_sink}}
        , diagnostic_sink_{&diagnostic_sink}
    {
    }

    auto parse() -> ast::AstContext
    {
        return ast::AstContext{parse_translation_unit()};
    }

private:
    auto parse_translation_unit() -> std::unique_ptr<ast::TranslationUnit>
    {
        auto tu = std::make_unique<ast::TranslationUnit>();
        while (stream_.skip_newlines_and_peek().kind() != TokenKind::Eof) {
            const auto& token = stream_.peek();
            switch (token.kind()) {
            case TokenKind::Fn: {
                stream_.advance();
                try {
                    tu->functions.push_back(parse_function_decl());
                } catch (const ParseAbort&) {
                    skip_to(TokenKind::Fn);
                }
                break;
            }
            default: {
                diagnose_unexpected_token(TokenKind::Fn, token);
                skip_to(TokenKind::Fn);
                break;
            }
            }
        }
        return tu;
    }

    auto parse_function_decl() -> std::unique_ptr<ast::Function>
    {
        auto function = std::make_unique<ast::Function>();
        parse_type();
        function->name = expect(TokenKind::Identifier).lexeme();
        expect(TokenKind::ParenOpen);
        expect(TokenKind::ParenClose);
        function->stmts = parse_block();
        return function;
    }

    auto parse_block() -> std::vector<std::unique_ptr<ast::Stmt>>
    {
        std::vector<std::unique_ptr<ast::Stmt>> stmts{};
        // TODO: move to parse_stmt_list or smth else
        expect(TokenKind::BraceOpen);
        while (
            stream_.skip_newlines_and_peek().kind() != TokenKind::BraceClose)
        //
        {
            stmts.push_back(parse_stmt());
        }
        stream_.advance();
        return stmts;
    }

    auto parse_stmt() -> std::unique_ptr<ast::Stmt>
    {
        Token token = stream_.skip_newlines_and_peek();
        switch (token.kind()) {
        case TokenKind::Return:
            stream_.advance();
            return parse_return_stmt();
        case TokenKind::Identifier:
            stream_.advance();
            return parse_call_stmt(token);
        case TokenKind::Semicolon:
            stream_.advance();
            return std::make_unique<ast::EmptyStmt>();
        default:
            diagnostic_sink_->emit_error(
                std::format(
                    "unexpected {} at start of statement",
                    lex::describe(token.kind())),
                token.span());
            throw ParseAbort{};
        }
    }

    auto parse_return_stmt() -> std::unique_ptr<ast::ReturnStmt>
    {
        auto return_stmt = std::make_unique<ast::ReturnStmt>();
        return_stmt->value = parse_expr();
        expect(TokenKind::Semicolon);
        return return_stmt;
    }

    auto parse_call_stmt(Token name) -> std::unique_ptr<ast::Stmt>
    {
        if (name.lexeme() == "print") {
            return parse_print_stmt();
        } else if (name.lexeme() == "println") {
            return parse_println_stmt();
        } else {
            diagnostic_sink_->emit_error(
                std::format("unknown function '{}'", name.lexeme()),
                name.span());
            throw ParseAbort{};
        }
    }

    auto parse_print_stmt() -> std::unique_ptr<ast::PrintStmt>
    {
        auto call = std::make_unique<ast::PrintStmt>();
        expect(TokenKind::ParenOpen);
        call->args = parse_expr_list();
        expect(TokenKind::ParenClose);
        expect(TokenKind::Semicolon);
        return call;
    }

    auto parse_println_stmt() -> std::unique_ptr<ast::PrintlnStmt>
    {
        auto call = std::make_unique<ast::PrintlnStmt>();
        expect(TokenKind::ParenOpen);
        call->args = parse_expr_list();
        expect(TokenKind::ParenClose);
        expect(TokenKind::Semicolon);
        return call;
    }

    auto parse_expr_list() -> std::vector<std::unique_ptr<ast::Expr>>
    {
        std::vector<std::unique_ptr<ast::Expr>> exprs{};
        exprs.push_back(parse_expr());
        while (true) {
            switch (stream_.peek().kind()) {
            case TokenKind::Comma:
                stream_.advance();
                exprs.push_back(parse_expr());
                break;
            default:
                return exprs;
            }
        }
    }

    auto parse_expr() -> std::unique_ptr<ast::Expr>
    {
        const auto& token = stream_.peek();
        if (!token.is_valid()) {
            throw ParseAbort{};
        }
        switch (token.kind()) {
        case TokenKind::StringLiteral: {
            auto string_expr = std::make_unique<ast::StringExpr>();
            string_expr->value = token.string_value();
            stream_.advance();
            return string_expr;
        }
        case TokenKind::IntegerLiteral: {
            auto integer_expr = std::make_unique<ast::IntegerExpr>();
            integer_expr->value = token.integer_value();
            stream_.advance();
            return integer_expr;
        }
        default: {
            diagnostic_sink_->emit_error(
                std::format("expected expression, but found {}",
                    lex::describe(token.kind())),
                token.span());
            throw ParseAbort{};
        }
        }
    }

    // parse I32 type
    auto parse_type() -> void
    {
        Token identifier = expect(TokenKind::Identifier);
        if (identifier.lexeme() == "I32") {
            return;
        } else {
            diagnostic_sink_->emit_error(
                std::format(
                    "unknown type name '{}'",
                    identifier.lexeme()),
                identifier.span());
            throw ParseAbort{};
        }
    }

    auto expect(TokenKind expected) -> Token
    {
        Token token = stream_.skip_newlines_and_peek();
        // Прерываем разбор, игнорируя ошибку. Ошибки, связанные с некорректными
        // токенами диагностируются лексером.
        if (!token.is_valid()) {
            throw ParseAbort{};
        }
        if (token.kind() != expected) {
            diagnose_unexpected_token(expected, token);
            throw ParseAbort{};
        }
        stream_.advance();
        return token;
    }

    // TODO: better naming
    auto skip_to(TokenKind target) -> void
    {
        TokenKind current = stream_.peek().kind();
        while (current != target && current != TokenKind::Eof) {
            stream_.advance();
            current = stream_.peek().kind();
        }
    }

    auto diagnose_unexpected_token(
        TokenKind expected,
        const Token& actual)
        -> void
    {
        diagnostic_sink_->emit_error(
            std::format(
                "expected {}, but found {}",
                lex::describe(expected),
                lex::describe(actual.kind())),
            actual.span());
    }

private:
    TokenStream stream_;
    IDiagnosticSink* diagnostic_sink_;
}; // namespace

} // namespace

auto parse(
    const SourceBuffer& source_buffer,
    IDiagnosticSink& diagnostic_sink)
    -> ast::AstContext
{
    return Parser{source_buffer, diagnostic_sink}.parse();
}

} // namespace cprime::parse

#include "parse.hpp"

#include <algorithm>
#include <format>
#include <limits>
#include <tuple>
#include <boost/intrusive/list.hpp>
#include <cprime/ast/ast.hpp>
#include <cprime/diagnostics/diagnostic_sink.hpp>
#include <cprime/lex/lex.hpp>
#include <cprime/lex/token.hpp>
#include <cprime/source/source_buffer.hpp>
#include <cprime/support/contract.hpp>

namespace cprime::parse {

namespace {

using diagnostics::IDiagnosticSink;
using lex::Lexer;
using lex::Token;
using lex::TokenKind;
using source::LineColumn;
using source::SourceBuffer;
using source::SourceSpan;

struct SourceMarker
{
    LineColumn location;
    const char* cbegin;
};

class ParsePanic final
{
};

class Parser final
{
public:
    explicit Parser(
        ast::AstContext& ast_context,
        const SourceBuffer& source_buffer,
        IDiagnosticSink& diagnostic_sink)
        : ast_context_{&ast_context}
        , source_buffer_{&source_buffer}
        , diagnostic_sink_{&diagnostic_sink}
        , lexer_{source_buffer, diagnostic_sink}
        , token_{lexer_.next()}
        , capture_span_end_{nullptr}
        , errors_found_{false}
    {
        // TODO: remove this dirty hack when NewLine token gets removed
        if (token_.kind() == TokenKind::NewLine) {
            advance();
        }
    }

    auto parse() -> bool
    {
        ast_context_->set_translation_unit(parse_translation_unit());
        return !errors_found_;
    }

private:
    auto parse_translation_unit() -> ast::TranslationUnit*
    {
        boost::intrusive::list<ast::FunctionDecl> functions{};
        while (token_.kind() != TokenKind::Eof) {
            try {
                functions.push_back(*parse_function_decl());
            } catch (const ParsePanic&) {
                skip_until({TokenKind::Fn});
            }
        }

        SourceSpan span{
            *source_buffer_,
            LineColumn{1, 1},
            source_buffer_->cbegin(),
            source_buffer_->cend()};
        return ast_context_->make<ast::TranslationUnit>(
            span,
            std::move(functions));
    }

    auto parse_function_decl() -> ast::FunctionDecl*
    {
        SourceMarker marker = start_capturing();

        match(TokenKind::Fn);
        parse_type();
        std::string_view name = match(TokenKind::Identifier).lexeme();
        match(TokenKind::ParenOpen);
        match(TokenKind::ParenClose);
        ast::Block* body = parse_block();

        return ast_context_->make<ast::FunctionDecl>(
            capture_span(marker),
            name,
            body);
    }

    auto parse_block() -> ast::Block*
    {
        SourceMarker marker = start_capturing();

        match(TokenKind::BraceOpen);

        boost::intrusive::list<ast::Stmt> stmts{};
        while (token_.kind() != TokenKind::BraceClose) {
            try {
                stmts.push_back(*parse_stmt());
            } catch (const ParsePanic&) {
                if (token_.kind() == TokenKind::Eof ||
                    token_.kind() == TokenKind::Fn)
                //
                {
                    diagnostic_sink_->emit_error(
                        "expected '}' to close block",
                        token_.span());
                    panic();
                }

                skip_until({
                    TokenKind::BraceClose,
                    TokenKind::Semicolon,
                    TokenKind::Fn,
                });
                if (token_.kind() == TokenKind::Semicolon) {
                    advance();
                }
            }
        }
        advance();

        return ast_context_->make<ast::Block>(
            capture_span(marker),
            std::move(stmts));
    }

    auto parse_stmt() -> ast::Stmt*
    {
        switch (token_.kind()) {
        case TokenKind::Semicolon:
            return parse_empty_stmt();
        case TokenKind::Return:
            return parse_return_stmt();
        default:
            return parse_expr_stmt();
        }
    }

    auto parse_empty_stmt() -> ast::EmptyStmt*
    {
        SourceMarker marker = start_capturing();

        match(TokenKind::Semicolon);
        return ast_context_->make<ast::EmptyStmt>(capture_span(marker));
    }

    auto parse_return_stmt() -> ast::ReturnStmt*
    {
        SourceMarker marker = start_capturing();

        match(TokenKind::Return);
        ast::Expr* value = parse_expr();
        match(TokenKind::Semicolon);

        return ast_context_->make<ast::ReturnStmt>(
            capture_span(marker),
            value);
    }

    auto parse_expr_stmt() -> ast::ExprStmt*
    {
        SourceMarker marker = start_capturing();

        ast::Expr* expr = parse_expr();
        match(TokenKind::Semicolon);

        return ast_context_->make<ast::ExprStmt>(
            capture_span(marker),
            expr);
    }

    auto parse_expr_list(TokenKind terminator) -> ast::ExprList*
    {
        SourceMarker marker = start_capturing();

        if (token_.kind() == terminator) {
            return ast_context_->make<ast::ExprList>(
                capture_span(marker),
                boost::intrusive::list<ast::Expr>{});
        }

        boost::intrusive::list<ast::Expr> exprs{};
        while (true) {
            // TODO: Реализовать error recovery при разборе выражений.
            //
            // Допустим, мы решили реализовать наивный вариант error recovery:
            //   try {
            //       exprs.push_back(*parse_expr());
            //   } catch (const ParsePanic&) {
            //       skip_until({TokenKind::Comma, terminator});
            //   }
            //
            // Для синхронизации используются токены ',' и некоторая закрывающая
            // скобка.
            //
            // Рассмотрим следующий пример:
            //   fn Unit foo() {
            //       println(1 +
            //   }
            //   );
            //
            // При разборе вызова println() парсер выйдет за границы функции
            // foo(), что сделает дальнейший синтаксический анализ невозможным.

            exprs.push_back(*parse_expr());
            if (token_.kind() == terminator) {
                return ast_context_->make<ast::ExprList>(
                    capture_span(marker),
                    std::move(exprs));
            }
            match(TokenKind::Comma);
        }
    }

    auto parse_expr() -> ast::Expr*
    {
        return parse_expr_bp(0);
    }

    auto parse_expr_bp(i32 min_bp) -> ast::Expr*
    {
        // Реализуем разбор бинарных выражений, используя Pratt parsing, также
        // известный как precedence climbing (хотя Википедия почему-то считает
        // иначе, см. https://www.oilshell.org/blog/2016/11/01.html).
        //
        // bp --- это binding power (сила связи). Для поддержки правой
        // ассоциативности каждому бинарному оператору сопоставляется два вида
        // силы связи: левая (l_bp) и правая (r_bp).
        //
        // Преимущества перед реализацией в стиле рекурсивного спуска таковы:
        // 1. Проще добавлять новые операторы.
        // 2. Меньше рекурсивных вызовов функций.
        //
        // Подробнее об алгоритме можно узнать в этой статье:
        // https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html

        auto get_binary_op_bp = [](TokenKind kind)
            -> std::tuple<ast::BinaryOp, i32, i32> {
            switch (kind) {
            case TokenKind::Plus:
                return {ast::BinaryOp::Add, 1, 2};
            case TokenKind::Minus:
                return {ast::BinaryOp::Sub, 1, 2};
            case TokenKind::Star:
                return {ast::BinaryOp::Mul, 3, 4};
            case TokenKind::Slash:
                return {ast::BinaryOp::Div, 3, 4};
            case TokenKind::Percent:
                return {ast::BinaryOp::Modulo, 3, 4};
            default:
                // Токен не является бинарным оператором. Возвращаем l_bp = -1,
                // чтобы завершить разбор бинарного выражения.
                return {ast::BinaryOp{}, -1, 0};
            }
        };

        SourceMarker marker = start_capturing();

        ast::Expr* lhs = parse_prefix_expr();
        while (true) {
            auto [op, l_bp, r_bp] = get_binary_op_bp(token_.kind());

            // Программист должен решать, какую ассоциативность имеют операторы
            // и как связываются операнды с операторами. Равенство l_bp и min_bp
            // не допустимо.
            CPRIME_DEBUG_ASSERT(
                l_bp != min_bp,
                "Parsing expression is ambiguous.");

            if (l_bp < min_bp) {
                break;
            }

            advance();
            ast::Expr* rhs = parse_expr_bp(r_bp);

            lhs = ast_context_->make<ast::BinaryExpr>(
                capture_span(marker),
                op,
                lhs,
                rhs);
        }
        return lhs;
    }

    auto parse_prefix_expr() -> ast::Expr*
    {
        SourceMarker marker = start_capturing();

        switch (token_.kind()) {
        case TokenKind::Minus: {
            advance();
            ast::Expr* operand = parse_prefix_expr();

            return ast_context_->make<ast::UnaryExpr>(
                capture_span(marker),
                ast::UnaryOp::Minus,
                operand);
        }

        default: {
            return parse_primary_expr();
        }
        }
    }

    auto parse_primary_expr() -> ast::Expr*
    {
        SourceMarker marker = start_capturing();

        if (token_.kind() == TokenKind::Identifier) {
            std::string_view name = token_.lexeme();
            advance();

            if (token_.kind() != TokenKind::ParenOpen) {
                return ast_context_->make<ast::NameExpr>(
                    capture_span(marker),
                    name);
            }

            // TODO: исправить грамматику, чтобы соответствовала комментарию.
            //
            // Вызов функции в C' является первичным выражением, поэтому
            // обрабатываем его здесь.
            advance();
            ast::ExprList* args = parse_expr_list(TokenKind::ParenClose);
            match(TokenKind::ParenClose);

            return ast_context_->make<ast::CallExpr>(
                capture_span(marker),
                name,
                args);
        }

        // Токены строковых и целочисленных литералов могут быть невалидными.
        // Это нужно учесть.
        if (!token_.is_valid()) {
            // Ошибка уже диагностирована лексером, поэтому здесь диагностика
            // не нужна.
            panic();
        }
        switch (token_.kind()) {
        case TokenKind::StringLiteral: {
            std::pmr::string value =
                ast_context_->make_string(token_.string_value());
            advance();
            return ast_context_->make<ast::StringExpr>(
                capture_span(marker),
                std::move(value));
        }

        case TokenKind::IntegerLiteral: {
            i64 value = token_.integer_value();
            advance();

            // TODO: Сделать нормальную диагностику.
            constexpr i32 kI32Min = std::numeric_limits<i32>::min();
            constexpr i32 kI32Max = std::numeric_limits<i32>::max();
            CPRIME_DEBUG_ASSERT(kI32Min <= value && value <= kI32Max);
            i32 value_i32 = static_cast<i32>(value);

            return ast_context_->make<ast::IntegerExpr>(
                capture_span(marker),
                value_i32);
        }

        default: {
            diagnostic_sink_->emit_error(
                std::format(
                    "unexpected {} at start of expression",
                    lex::describe(token_.kind())),
                token_.span());
            panic();
        }
        }
    }

    auto parse_type() -> void
    {
        Token identifier = match(TokenKind::Identifier);
        if (identifier.lexeme() != "I32") {
            diagnostic_sink_->emit_error(
                std::format(
                    "unknown type name '{}'",
                    identifier.lexeme()),
                identifier.span());
            panic();
        }
    }

    auto skip_until(std::initializer_list<TokenKind> terminators) -> void
    {
        while (
            !std::ranges::contains(terminators, token_.kind()) &&
            token_.kind() != TokenKind::Eof)
        //
        {
            advance();
        }
    }

    auto match(TokenKind kind) -> Token
    {
        if (token_.kind() != kind) {
            diagnose_unexpected_token(kind, token_);
            panic();
        }
        Token matched = token_;
        advance();
        return matched;
    }

    auto advance() -> void
    {
        capture_span_end_ = token_.span().cend();
        token_ = lexer_.next();

        // TODO: Нужно удалить токен NewLine из лексера и тогда это станет не
        // нужным.
        while (token_.kind() == TokenKind::NewLine) {
            token_ = lexer_.next();
        }
    }

    [[noreturn]]
    auto panic() -> void
    {
        errors_found_ = true;
        throw ParsePanic{};
    }

    auto start_capturing() -> SourceMarker
    {
        // Если узел имеет пустой диапазон, то может случиться так, что
        // start.cbegin > capture_span_end. Например:
        //
        //   foo(  )
        //       ^^
        //       ExprList пуст. start.cbegin на ')', capture_span_end на '('.
        capture_span_end_ = token_.span().cbegin();

        return SourceMarker{
            token_.span().location(),
            token_.span().cbegin()};
    }

    auto capture_span(SourceMarker start) -> SourceSpan
    {
        CPRIME_DEBUG_ASSERT(
            capture_span_end_ != nullptr,
            "Node span is not being captured.");
        return SourceSpan{
            *source_buffer_,
            start.location,
            start.cbegin,
            capture_span_end_};
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
    ast::AstContext* ast_context_;
    const SourceBuffer* source_buffer_;
    IDiagnosticSink* diagnostic_sink_;

    Lexer lexer_;

    Token token_;
    const char* capture_span_end_;
    bool errors_found_;
};

} // namespace

auto parse_cprime_source(
    ast::AstContext& ast_context,
    diagnostics::IDiagnosticSink& diagnostic_sink,
    const source::SourceBuffer& source_buffer)
    -> bool
{
    return Parser{ast_context, source_buffer, diagnostic_sink}.parse();
}

} // namespace cprime::parse

#ifndef CPRIME_AST_EXPR_H_
#define CPRIME_AST_EXPR_H_

#include "node.hpp"
#include <boost/intrusive/list.hpp>
#include <cprime/support/prelude.hpp>
#include <cprime/source/source_buffer.hpp>
#include <cprime/support/contract.hpp>

namespace cprime::ast {

class Expr
    : public Node
    , public boost::intrusive::list_base_hook<>
{
protected:
    explicit Expr(NodeKind kind, source::SourceSpan span)
        : Node{kind, span}
    {
    }

    ~Expr() = default;
};

class ExprList final : public Node
{
private:
    using InnerListType = boost::intrusive::list<Expr>;

public:
    explicit ExprList(source::SourceSpan span, InnerListType exprs)
        : Node{NodeKind::ExprList, span}
        , exprs_{std::move(exprs)}
    {
    }

    auto begin() -> InnerListType::iterator
    {
        return exprs_.begin();
    }

    auto end() -> InnerListType::iterator
    {
        return exprs_.end();
    }

private:
    InnerListType exprs_;
};

class StringExpr final : public Expr
{
public:
    explicit StringExpr(
        source::SourceSpan span,
        std::pmr::string value)
        : Expr{NodeKind::StringExpr, span}
        , value_{std::move(value)}
    {
    }

    auto value() -> const std::pmr::string&
    {
        return value_;
    }

private:
    std::pmr::string value_;
};

class IntegerExpr final : public Expr
{
public:
    explicit IntegerExpr(source::SourceSpan span, i64 value)
        : Expr{NodeKind::IntegerExpr, span}
        , value_{value}
    {
    }

    auto value() -> i64
    {
        return value_;
    }

private:
    i64 value_;
};

template<typename ReturnType = void>
class ExprVisitor
{
protected:
    explicit ExprVisitor() = default;

    template<typename Self>
    auto accept_expr(this Self&& self, Expr* expr) -> ReturnType
    {
        CPRIME_DEBUG_ASSERT(expr != nullptr);

        auto call_visit = [&self](auto concrete_expr) -> ReturnType {
            return std::forward<Self>(self).visit(concrete_expr);
        };

        switch (expr->kind()) {
        case NodeKind::StringExpr:
            return call_visit(static_cast<StringExpr*>(expr));
        case NodeKind::IntegerExpr:
            return call_visit(static_cast<IntegerExpr*>(expr));
        default:
            CPRIME_UNREACHABLE("Unknown expression kind.");
        }
    }
};

} // namespace cprime::ast

#endif // CPRIME_AST_EXPR_H_

#ifndef CPRIME_AST_EXPR_H_
#define CPRIME_AST_EXPR_H_

#include "node.hpp"
#include <magic_enum/magic_enum.hpp>
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
    explicit IntegerExpr(source::SourceSpan span, i32 value)
        : Expr{NodeKind::IntegerExpr, span}
        , value_{value}
    {
    }

    auto value() -> i32
    {
        return value_;
    }

private:
    i32 value_;
};

class NameExpr final : public Expr
{
public:
    explicit NameExpr(source::SourceSpan span, std::string_view name)
        : Expr{NodeKind::NameExpr, span}
        , name_{name}
    {
    }

    auto name() -> std::string_view
    {
        return name_;
    }

private:
    std::string_view name_;
};

enum class CalleeKind {
    BuiltinPrint,
    BuiltinPrintln,
    Function,
};

class CallExpr final : public Expr
{
public:
    explicit CallExpr(
        source::SourceSpan span,
        std::string_view callee,
        ExprList* args)
        : Expr{NodeKind::CallExpr, span}
        , callee_{callee}
        , args_{args}
        , callee_kind_{get_callee_kind(callee)}
    {
    }

    auto callee() -> std::string_view
    {
        return callee_;
    }

    auto args() -> ExprList*
    {
        return args_;
    }

    auto callee_kind() -> CalleeKind
    {
        return callee_kind_;
    }

private:
    static auto get_callee_kind(std::string_view callee) -> CalleeKind
    {
        if (callee == "print") {
            return CalleeKind::BuiltinPrint;
        } else if (callee == "println") {
            return CalleeKind::BuiltinPrintln;
        } else {
            return CalleeKind::Function;
        }
    }

private:
    std::string_view callee_;
    ExprList* args_;

    CalleeKind callee_kind_;
};

enum class UnaryOp {
    Minus,
    Increment,
    Decrement,
};

class UnaryExpr final : public Expr
{
public:
    explicit UnaryExpr(source::SourceSpan span, UnaryOp op, Expr* operand)
        : Expr{NodeKind::UnaryExpr, span}
        , op_{op}
        , operand_{operand}
    {
    }

    auto op() -> UnaryOp
    {
        return op_;
    }

    auto operand() -> Expr*
    {
        return operand_;
    }

private:
    UnaryOp op_;
    Expr* operand_;
};

enum class BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Modulo,
};

class BinaryExpr final : public Expr
{
public:
    explicit BinaryExpr(
        source::SourceSpan span,
        BinaryOp op,
        Expr* left, Expr* right)
        : Expr{NodeKind::BinaryExpr, span}
        , op_{op}
        , left_{left}
        , right_{right}
    {
    }

    auto op() -> BinaryOp
    {
        return op_;
    }

    auto left() -> Expr*
    {
        return left_;
    }

    auto right() -> Expr*
    {
        return right_;
    }

private:
    BinaryOp op_;
    Expr* left_;
    Expr* right_;
};

template<typename ReturnType = void>
class ExprVisitor
{
protected:
    explicit ExprVisitor() = default;

    template<typename Self>
    [[nodiscard]]
    auto dispatch_expr(this Self&& self, Expr* expr) -> ReturnType
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
        case NodeKind::NameExpr:
            return call_visit(static_cast<NameExpr*>(expr));
        case NodeKind::CallExpr:
            return call_visit(static_cast<CallExpr*>(expr));
        case NodeKind::UnaryExpr:
            return call_visit(static_cast<UnaryExpr*>(expr));
        case NodeKind::BinaryExpr:
            return call_visit(static_cast<BinaryExpr*>(expr));
        default:
            CPRIME_UNREACHABLE(
                "Unknown expression kind NodeKind::{}.",
                magic_enum::enum_name(expr->kind()));
        }
    }
};

} // namespace cprime::ast

#endif // CPRIME_AST_EXPR_H_

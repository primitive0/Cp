#ifndef CPRIME_AST_STMT_H_
#define CPRIME_AST_STMT_H_

#include "expr.hpp"
#include "node.hpp"
#include <magic_enum/magic_enum.hpp>
#include <boost/intrusive/list.hpp>
#include <cprime/support/prelude.hpp>
#include <cprime/source/source_buffer.hpp>
#include <cprime/support/contract.hpp>

namespace cprime::ast {

class Stmt
    : public Node
    , public boost::intrusive::list_base_hook<>
{
protected:
    explicit Stmt(NodeKind kind, source::SourceSpan span)
        : Node{kind, span}
    {
    }

    ~Stmt() = default;
};

class Block final : public Node
{
private:
    using InnerListType = boost::intrusive::list<Stmt>;

public:
    explicit Block(source::SourceSpan span, InnerListType stmts)
        : Node{NodeKind::Block, span}
        , stmts_{std::move(stmts)}
    {
    }

    auto begin() -> InnerListType::iterator
    {
        return stmts_.begin();
    }

    auto end() -> InnerListType::iterator
    {
        return stmts_.end();
    }

private:
    InnerListType stmts_;
};

class EmptyStmt final : public Stmt
{
public:
    explicit EmptyStmt(source::SourceSpan span)
        : Stmt{NodeKind::EmptyStmt, span}
    {
    }
};

class ReturnStmt final : public Stmt
{
public:
    explicit ReturnStmt(source::SourceSpan span, Expr* value)
        : Stmt{NodeKind::ReturnStmt, span}
        , value_{value}
    {
    }

    auto value() -> Expr*
    {
        return value_;
    }

private:
    Expr* value_;
};

class ExprStmt final : public Stmt
{
public:
    explicit ExprStmt(source::SourceSpan span, Expr* expr)
        : Stmt{NodeKind::ExprStmt, span}
        , expr_{expr}
    {
    }

    auto expr() -> Expr*
    {
        return expr_;
    }

private:
    Expr* expr_;
};

template<typename ReturnType = void>
class StmtVisitor
{
protected:
    explicit StmtVisitor() = default;

    template<typename Self>
    [[nodiscard]]
    auto dispatch_stmt(this Self&& self, Stmt* stmt) -> ReturnType
    {
        CPRIME_DEBUG_ASSERT(stmt != nullptr);

        auto call_visit = [&self](auto concrete_stmt) -> ReturnType {
            return std::forward<Self>(self).visit(concrete_stmt);
        };

        switch (stmt->kind()) {
        case NodeKind::EmptyStmt:
            return call_visit(static_cast<EmptyStmt*>(stmt));
        case NodeKind::ReturnStmt:
            return call_visit(static_cast<ReturnStmt*>(stmt));
        case NodeKind::ExprStmt:
            return call_visit(static_cast<ExprStmt*>(stmt));
        default:
            CPRIME_UNREACHABLE(
                "Unknown statement kind NodeKind::{}.",
                magic_enum::enum_name(stmt->kind()));
        }
    }
};

} // namespace cprime::ast

#endif // CPRIME_AST_STMT_H_

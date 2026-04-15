#ifndef CPRIME_AST_AST_H_
#define CPRIME_AST_AST_H_

#include "visitor.hpp"

#include <cprime/support/prelude.hpp>

namespace cprime::ast {

struct Node
{
public:
    virtual ~Node() = default;

    Node(const Node&) = delete;
    auto operator=(const Node&) -> Node& = delete;

    virtual auto accept(Visitor& v) -> void = 0;
    virtual auto accept_children(Visitor& v) -> void {}

protected:
    Node(Node&&) = default;
    auto operator=(Node&&) -> Node& = default;

    explicit Node() = default;
};

template<typename Base, typename Derived>
struct ConcreteNode : public Base
{
public:
    auto accept(Visitor& v) -> void override
    {
        v.visit(static_cast<Derived&>(*this));
    }

protected:
    ConcreteNode(ConcreteNode&&) = default;
    auto operator=(ConcreteNode&&) -> ConcreteNode& = default;

    explicit ConcreteNode() = default;
};

struct Expr : public Node
{
protected:
    Expr(Expr&&) = default;
    auto operator=(Expr&&) -> Expr& = default;

    explicit Expr() = default;
};

struct VariableExpr final : public ConcreteNode<Expr, VariableExpr>
{
    std::string_view name{};
};

struct LiteralExpr : public Expr
{
protected:
    LiteralExpr(LiteralExpr&&) = default;
    auto operator=(LiteralExpr&&) -> LiteralExpr& = default;

    explicit LiteralExpr() = default;
};

struct StringExpr final : public ConcreteNode<LiteralExpr, StringExpr>
{
    std::string value{};
};

struct IntegerExpr final : public ConcreteNode<LiteralExpr, IntegerExpr>
{
    i64 value{};
};

struct Stmt : public Node
{
protected:
    Stmt(Stmt&&) = default;
    auto operator=(Stmt&&) -> Stmt& = default;

    explicit Stmt() = default;
};

struct ReturnStmt final : public ConcreteNode<Stmt, ReturnStmt>
{
    std::unique_ptr<Expr> value{};
};

struct PrintStmt final : public ConcreteNode<Stmt, PrintStmt>
{
    std::vector<std::unique_ptr<Expr>> args{};
};

struct PrintlnStmt final : public ConcreteNode<Stmt, PrintlnStmt>
{
    std::vector<std::unique_ptr<Expr>> args{};
};

struct EmptyStmt final : public ConcreteNode<Stmt, EmptyStmt>
{
};

struct Function final : public ConcreteNode<Node, Function>
{
    std::string_view name{};
    std::vector<std::unique_ptr<Stmt>> stmts{};
};

struct TranslationUnit final : public ConcreteNode<Node, TranslationUnit>
{
    std::vector<std::unique_ptr<Function>> functions{};
};

// TODO: implement class AstContext
struct AstContext
{
    std::unique_ptr<TranslationUnit> translation_unit{};
};
// class AstContext final
// {
// public:
// private:
// };

} // namespace cprime::ast

#endif // CPRIME_AST_AST_H_

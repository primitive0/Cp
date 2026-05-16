#ifndef CPRIME_AST_NODE_H_
#define CPRIME_AST_NODE_H_

#include <cprime/support/prelude.hpp>
#include <cprime/source/source_buffer.hpp>

namespace cprime::ast {

enum class NodeKind {
    TranslationUnit,
    FunctionDecl,
    Block,
    EmptyStmt,
    ReturnStmt,
    CallStmt,
    ExprList,
    StringExpr,
    IntegerExpr,
};

class Node
{
public:
    // Подклассы Node создаются на арене, предоставленной AstContext.
    // Стандартные операторы new/delete лучше запретить, чтобы избежать ошибок
    // при работе с узлами AST.

    static auto operator new(std::size_t) -> void* = delete;
    static auto operator delete(void*) -> void = delete;

    static auto operator new[](std::size_t) -> void* = delete;
    static auto operator delete[](void*) -> void = delete;

public:
    Node(const Node&) = delete;
    auto operator=(const Node&) -> Node& = delete;

    Node(Node&&) = delete;
    auto operator=(Node&&) -> Node& = delete;

    auto kind() -> NodeKind
    {
        return kind_;
    }

    auto span() -> source::SourceSpan
    {
        return span_;
    }

protected:
    explicit Node(NodeKind kind, source::SourceSpan span)
        : kind_{kind}
        , span_{span}
    {
    }

    ~Node() = default;

private:
    NodeKind kind_;
    source::SourceSpan span_;
};

} // namespace cprime::ast

#endif // CPRIME_AST_NODE_H_

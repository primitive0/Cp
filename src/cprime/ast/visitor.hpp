#ifndef CPRIME_AST_VISITOR_H_
#define CPRIME_AST_VISITOR_H_

#include <cprime/support/contract.hpp>

namespace cprime::ast {

class Visitor
{
public:
    virtual ~Visitor() = default;

    // clang-format off
    virtual auto visit(struct TranslationUnit&) -> void { CPRIME_UNREACHABLE("Not implemented."); }
    virtual auto visit(struct Function&)        -> void { CPRIME_UNREACHABLE("Not implemented."); }

    virtual auto visit(struct ReturnStmt&)      -> void { CPRIME_UNREACHABLE("Not implemented."); }
    virtual auto visit(struct PrintStmt&)       -> void { CPRIME_UNREACHABLE("Not implemented."); }
    virtual auto visit(struct PrintlnStmt&)     -> void { CPRIME_UNREACHABLE("Not implemented."); }
    virtual auto visit(struct EmptyStmt&)       -> void { CPRIME_UNREACHABLE("Not implemented."); }

    virtual auto visit(struct VariableExpr&)    -> void { CPRIME_UNREACHABLE("Not implemented."); }
    virtual auto visit(struct StringExpr&)      -> void { CPRIME_UNREACHABLE("Not implemented."); }
    virtual auto visit(struct IntegerExpr&)     -> void { CPRIME_UNREACHABLE("Not implemented."); }
    // clang-format on

protected:
    Visitor(const Visitor&) = default;
    auto operator=(const Visitor&) -> Visitor& = default;

    Visitor(Visitor&&) = default;
    auto operator=(Visitor&&) -> Visitor& = default;

    explicit Visitor() = default;
};

} // namespace cprime::ast

#endif // CPRIME_AST_VISITOR_H_

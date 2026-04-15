#ifndef CPRIME_AST_VISITOR_H_
#define CPRIME_AST_VISITOR_H_

#include <cassert>

namespace cprime::ast {

class Visitor
{
public:
    virtual ~Visitor() = default;

    // clang-format off
    virtual auto visit(struct TranslationUnit&) -> void { assert(false && "Not implemented."); }
    virtual auto visit(struct Function&)        -> void { assert(false && "Not implemented."); }

    virtual auto visit(struct ReturnStmt&)      -> void { assert(false && "Not implemented."); }
    virtual auto visit(struct PrintStmt&)       -> void { assert(false && "Not implemented."); }
    virtual auto visit(struct PrintlnStmt&)     -> void { assert(false && "Not implemented."); }
    virtual auto visit(struct EmptyStmt&)       -> void { assert(false && "Not implemented."); }

    virtual auto visit(struct VariableExpr&)    -> void { assert(false && "Not implemented."); }
    virtual auto visit(struct StringExpr&)      -> void { assert(false && "Not implemented."); }
    virtual auto visit(struct IntegerExpr&)     -> void { assert(false && "Not implemented."); }
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

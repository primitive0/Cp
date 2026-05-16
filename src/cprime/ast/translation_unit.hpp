#ifndef CPRIME_AST_TRANSLATIONUNIT_H_
#define CPRIME_AST_TRANSLATIONUNIT_H_

#include "decl.hpp"
#include "node.hpp"
#include <boost/intrusive/list.hpp>
#include <cprime/support/prelude.hpp>
#include <cprime/source/source_buffer.hpp>

namespace cprime::ast {

class TranslationUnit final : public Node
{
private:
    using FunctionListType = boost::intrusive::list<FunctionDecl>;

public:
    explicit TranslationUnit(
        source::SourceSpan span,
        FunctionListType functions)
        : Node{NodeKind::TranslationUnit, span}
        , functions_{std::move(functions)}
    {
    }

    auto begin() -> FunctionListType::iterator
    {
        return functions_.begin();
    }

    auto end() -> FunctionListType::iterator
    {
        return functions_.end();
    }

private:
    FunctionListType functions_;
};

} // namespace cprime::ast

#endif // CPRIME_AST_TRANSLATIONUNIT_H_

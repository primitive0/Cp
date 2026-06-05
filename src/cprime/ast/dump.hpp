#ifndef CPRIME_AST_DUMP_H_
#define CPRIME_AST_DUMP_H_

#include <ostream>
#include <cprime/support/prelude.hpp>

namespace cprime::ast {

class TranslationUnit;

// Записывает в поток `os` строковое представление синтаксической части AST.
// Семантические атрибуты не включаются в результат.
auto dump_syntax_ast(
    std::ostream& os,
    TranslationUnit* translation_unit) -> void;

} // namespace cprime::ast

#endif // CPRIME_AST_DUMP_H_

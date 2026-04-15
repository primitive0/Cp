#ifndef CPRIME_PARSE_PARSE_H_
#define CPRIME_PARSE_PARSE_H_

#include <cprime/support/prelude.hpp>
#include <cprime/ast/ast.hpp>
#include <cprime/diagnostics/diagnostic_sink.hpp>

namespace cprime::parse {

auto parse(
    const source::SourceBuffer& source_buffer,
    diagnostics::IDiagnosticSink& diagnostic_sink)
    -> ast::AstContext;

} // namespace cprime::parse

#endif // CPRIME_PARSE_PARSE_H_

#ifndef CPRIME_PARSE_PARSE_H_
#define CPRIME_PARSE_PARSE_H_

#include <cprime/support/prelude.hpp>
#include <cprime/ast/ast_context.hpp>
#include <cprime/diagnostics/diagnostic_sink.hpp>
#include <cprime/source/source_buffer.hpp>

namespace cprime::parse {

[[nodiscard]]
auto parse_cprime_source(
    ast::AstContext& ast_context,
    diagnostics::IDiagnosticSink& diagnostic_sink,
    const source::SourceBuffer& source_buffer)
    -> bool;

} // namespace cprime::parse

#endif // CPRIME_PARSE_PARSE_H_

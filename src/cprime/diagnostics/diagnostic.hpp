#ifndef CPRIME_DIAGNOSTICS_DIAGNOSTIC_H_
#define CPRIME_DIAGNOSTICS_DIAGNOSTIC_H_

#include <cprime/support/prelude.hpp>
#include <cprime/source/source_buffer.hpp>

namespace cprime::diagnostics {

enum class Severity {
    Warning,
    Error,
};

struct Diagnostic
{
    Severity severity;
    std::string message;
    source::SourceSpan span;
};

} // namespace cprime::diagnostics

#endif // CPRIME_DIAGNOSTICS_DIAGNOSTIC_H_

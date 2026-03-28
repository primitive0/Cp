#ifndef CPRIME_DIAGNOSTICS_DIAGNOSTICSINK_H_
#define CPRIME_DIAGNOSTICS_DIAGNOSTICSINK_H_

#include <cprime/support/prelude.hpp>
#include <cprime/diagnostics/diagnostic.hpp>
#include <cprime/source/source_buffer.hpp>

namespace cprime::diagnostics {

class IDiagnosticSink
{
public:
    virtual ~IDiagnosticSink() = default;

    virtual auto emit(
        Severity severity,
        std::string message,
        source::SourceSpan span) -> void = 0;

    auto emit_error(std::string message, source::SourceSpan span) -> void
    {
        emit(Severity::Error, std::move(message), span);
    }

    auto emit_warning(std::string message, source::SourceSpan span) -> void
    {
        emit(Severity::Warning, std::move(message), span);
    }
};

} // namespace cprime::diagnostics

#endif // CPRIME_DIAGNOSTICS_DIAGNOSTICSINK_H_

#ifndef CPRIME_DIAGNOSTICS_DIAGNOSTICRECORDER_H_
#define CPRIME_DIAGNOSTICS_DIAGNOSTICRECORDER_H_

#include <cprime/support/prelude.hpp>
#include <cprime/diagnostics/diagnostic.hpp>
#include <cprime/diagnostics/diagnostic_sink.hpp>

namespace cprime::diagnostics {

class DiagnosticRecorder final : public IDiagnosticSink
{
public:
    explicit DiagnosticRecorder();

    auto emit(
        Severity severity,
        std::string message,
        source::SourceSpan span) -> void override;

    auto count() const -> size_t;
    auto error_count() const -> size_t;
    auto warning_count() const -> size_t;
    auto has_any() const -> bool;
    auto has_errors() const -> bool;
    auto has_warnings() const -> bool;

    auto begin() const -> const Diagnostic*;
    auto end() const -> const Diagnostic*;

    auto cbegin() const -> const Diagnostic*
    {
        return begin();
    }

    auto cend() const -> const Diagnostic*
    {
        return end();
    }

    auto reset() -> void;

private:
    std::vector<Diagnostic> diagnostics_;
    size_t error_count_;
    size_t warning_count_;
};

} // namespace cprime::diagnostics

#endif // CPRIME_DIAGNOSTICS_DIAGNOSTICRECORDER_H_

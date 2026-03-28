#include "diagnostic_recorder.hpp"

#include <cprime/support/prelude.hpp>
#include <cprime/diagnostics/diagnostic.hpp>

namespace cprime::diagnostics {

DiagnosticRecorder::DiagnosticRecorder()
    : diagnostics_{}
    , error_count_{0}
    , warning_count_{0}
{
}

auto DiagnosticRecorder::emit(
    Severity severity,
    std::string message,
    source::SourceSpan span) -> void
{
    diagnostics_.push_back(
        Diagnostic{severity, std::move(message), span});

    switch (severity) {
    case Severity::Warning:
        ++warning_count_;
        break;
    case Severity::Error:
        ++error_count_;
        break;
    }
}

auto DiagnosticRecorder::count() const -> size_t
{
    return diagnostics_.size();
}

auto DiagnosticRecorder::error_count() const -> size_t
{
    return error_count_;
}

auto DiagnosticRecorder::warning_count() const -> size_t
{
    return warning_count_;
}

auto DiagnosticRecorder::has_any() const -> bool
{
    return count() > 0;
}

auto DiagnosticRecorder::has_errors() const -> bool
{
    return error_count() > 0;
}

auto DiagnosticRecorder::has_warnings() const -> bool
{
    return warning_count() > 0;
}

auto DiagnosticRecorder::begin() const -> const Diagnostic*
{
    return diagnostics_.data();
}

auto DiagnosticRecorder::end() const -> const Diagnostic*
{
    return diagnostics_.data() + diagnostics_.size();
}

auto DiagnosticRecorder::reset() -> void
{
    diagnostics_.clear();
    error_count_ = 0;
    warning_count_ = 0;
}

} // namespace cprime::diagnostics

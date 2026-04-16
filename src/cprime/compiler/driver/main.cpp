#include <filesystem>
#include <print>

#include <cprime/compiler/codegen_llvm/codegen.hpp>
#include <cprime/diagnostics/diagnostic_recorder.hpp>
#include <cprime/parse/parse.hpp>
#include <cprime/source/source_buffer.hpp>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::println(stderr, "usage: cpc <file>");
        return 1;
    }

    auto source_buffer = cprime::source::SourceBuffer::from_file(argv[1]);

    cprime::diagnostics::DiagnosticRecorder diagnostics;
    auto ast_context = cprime::parse::parse(*source_buffer, diagnostics);

    for (const auto& diag : diagnostics) {
        auto loc = diag.span.location();
        std::println(stderr, "{}:{}:{}: {}: {}",
            diag.span.buffer().path().string(),
            loc.line,
            loc.column,
            diag.severity == cprime::diagnostics::Severity::Error
                ? "error"
                : "warning",
            diag.message);
    }

    if (diagnostics.has_errors()) {
        return 1;
    }

    const auto src_path = std::filesystem::path{argv[1]};
    const auto exe_path = src_path.parent_path() / src_path.stem();
    cprime::compiler::codegen_llvm::generate_executable(ast_context, exe_path);

    return 0;
}

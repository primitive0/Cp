#include <filesystem>
#include <print>
#include <cprime/ast/ast.hpp>
#include <cprime/compiler/codegen_llvm/codegen.hpp>
#include <cprime/diagnostics/diagnostic_recorder.hpp>
#include <cprime/parse/parse.hpp>
#include <cprime/source/source_buffer.hpp>

// TODO: refactor
auto main(int argc, char** argv) -> int
{
    if (argc < 2) {
        std::println(stderr, "usage: cpc <file>");
        return 1;
    }

    auto source_buffer = cprime::source::SourceBuffer::from_file(argv[1]);
    cprime::ast::AstContext ast_context{};
    cprime::diagnostics::DiagnosticRecorder diagnostics{};
    bool parsing_success = cprime::parse::parse_cprime_source(
        ast_context, diagnostics, *source_buffer);
    if (!parsing_success) {
        std::println(stderr, "parsing finished with errors");
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
        return 1;
    }

    auto src_path = std::filesystem::path{argv[1]};
    std::filesystem::path exe_path{};
    if (argc >= 3) {
        exe_path = std::filesystem::path{argv[2]};
    } else {
        exe_path = std::filesystem::current_path() / src_path.filename();
        exe_path += ".out";
    }
    cprime::codegen_llvm::initialize_llvm_targets();
    cprime::codegen_llvm::generate_executable(ast_context, exe_path);

    return 0;
}

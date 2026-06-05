#include <ostream>
#include <sstream>
#include <cprime/support/prelude.hpp>
#include <cprime/ast/ast.hpp>
#include <cprime/ast/dump.hpp>
#include <cprime/diagnostics/diagnostic.hpp>
#include <cprime/diagnostics/diagnostic_recorder.hpp>
#include <cprime/parse/parse.hpp>
#include <cprime/source/source_buffer.hpp>
#include <cprime/testing/file_test.hpp>

namespace cprime::parse {

using ast::AstContext;
using diagnostics::Diagnostic;
using diagnostics::DiagnosticRecorder;
using diagnostics::Severity;
using source::LineColumn;
using source::SourceBuffer;

// TODO: покрыть тестами случаи с токенами Error. Пока что в FileTest
// отсутствует возможность вставлять произвольные байты в текст.

class ParseFileTest final : public testing::FileTest
{
public:
    explicit ParseFileTest(std::string_view exe_name)
        : FileTest{"parse", "src/cprime/parse/testdata/", exe_name}
    {
    }

    auto process(std::string_view input) -> std::string override
    {
        auto source_buffer = SourceBuffer::from_text(input);

        AstContext ast_context{};
        DiagnosticRecorder diagnostics{};
        bool success = parse::parse_cprime_source(
            ast_context, diagnostics, *source_buffer);

        std::ostringstream oss{};
        if (success) {
            oss << "SUCCESS\n\n";
            ast::dump_syntax_ast(oss, ast_context.translation_unit());
        } else {
            oss << "ERROR\n\n";
            dump_diagnostics(oss, diagnostics);
        }
        return oss.str();
    }

private:
    static auto dump_diagnostics(
        std::ostream& os,
        const DiagnosticRecorder& diagnostics) -> void
    {
        for (const Diagnostic& diagnostic : diagnostics) {
            switch (diagnostic.severity) {
            case Severity::Warning:
                os << "Warning ";
                break;
            case Severity::Error:
                os << "Error ";
                break;
            }
            LineColumn location = diagnostic.span.location();
            os << "<" << location.line << ":" << location.column << "> ";
            os << "\"" << diagnostic.message << "\"\n";
        }
    }
};

} // namespace cprime::parse

auto main(int argc, char** argv) -> int
{
    return cprime::parse::ParseFileTest{"cprime_parse_file_test"}
        .run(argc, argv);
}

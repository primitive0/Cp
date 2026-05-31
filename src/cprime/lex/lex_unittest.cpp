#include "lex.hpp"

#include <iomanip>
#include <magic_enum/magic_enum.hpp>
#include <ostream>
#include <type_traits>
#include <variant>
#include <cprime/support/prelude.hpp>
#include <cprime/diagnostics/diagnostic_recorder.hpp>
#include <cprime/diagnostics/diagnostic_sink.hpp>
#include <cprime/lex/token.hpp>
#include <cprime/source/source_buffer.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

namespace cprime::lex {
namespace {

// TODO: use diagnostics recorder
using Catch::Matchers::RangeEquals;
using source::LineColumn;
using source::SourceBuffer;

using namespace std::literals::string_literals;

struct ExpectedToken
{
    TokenKind kind;
    std::string_view lexeme;
    LineColumn location;
    TokenValue computed_value;

    explicit(false) ExpectedToken(
        TokenKind kind,
        std::string_view lexeme,
        LineColumn location)
        : ExpectedToken{kind, lexeme, location, std::monostate{}}
    {
    }

    explicit(false) ExpectedToken(
        TokenKind kind,
        std::string_view lexeme,
        LineColumn location,
        TokenValue computed_value)
        : kind{kind}
        , lexeme{lexeme}
        , location{location}
        , computed_value{std::move(computed_value)}
    {
    }

    auto operator==(const ExpectedToken&) const -> bool = default;
};

// TODO: change
auto operator<<(std::ostream& out, const ExpectedToken& token) -> std::ostream&
{
    out << "ExpectedToken{"
        << "kind=" << magic_enum::enum_name(token.kind)
        << ", lexeme=" << std::quoted(std::string{token.lexeme})
        << ", location=" << token.location.line << ':' << token.location.column;

    std::visit(
        [&](const auto& value) {
            using T = std::decay_t<decltype(value)>;

            if constexpr (!std::is_same_v<T, std::monostate>) {
                out << ", computed_value=" << value;
            }
        },
        token.computed_value);

    out << '}';

    return out;
}

class LexHelper final
{
public:
    explicit LexHelper() = default;

    auto run(
        std::string_view source_code,
        diagnostics::IDiagnosticSink& diagnostic_sink)
        -> std::vector<ExpectedToken>
    {
        source_buffer_ = SourceBuffer::from_text(source_code);

        Lexer lexer{*source_buffer_, diagnostic_sink};

        std::vector<ExpectedToken> tokens{};
        while (true) {
            Token token = lexer.next();
            REQUIRE(token.lexeme() == token.span().content());

            tokens.push_back(ExpectedToken{
                token.kind(),
                token.lexeme(),
                token.span().location(),
                token.computed_value()});

            if (token.kind() == TokenKind::Eof) {
                return tokens;
            }
        }
    }

private:
    std::unique_ptr<SourceBuffer> source_buffer_{};
};

TEST_CASE("Can lex empty source buffer", "[cprime][lex]")
{
    diagnostics::DiagnosticRecorder diagnostic_recorder{};
    LexHelper lex_helper{};
    REQUIRE_THAT(
        lex_helper.run("", diagnostic_recorder),
        RangeEquals(
            {
                ExpectedToken{TokenKind::Eof, "", LineColumn{1, 1}},
            }));
    REQUIRE(!diagnostic_recorder.has_errors());
}

TEST_CASE("Can lex NewLine token", "[cprime][lex]")
{
    struct TC
    {
        std::string_view input;
        std::vector<ExpectedToken> expected;
    };

    auto test_case = GENERATE(
        TC{
            "\n",
            {
                {TokenKind::NewLine, "\n", LineColumn{1, 1}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "\n\n\n\n\n",
            {
                {TokenKind::NewLine, "\n", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{2, 1}},
                {TokenKind::NewLine, "\n", LineColumn{3, 1}},
                {TokenKind::NewLine, "\n", LineColumn{4, 1}},
                {TokenKind::NewLine, "\n", LineColumn{5, 1}},
                {TokenKind::Eof, "", LineColumn{6, 1}},
            },
        },
        TC{
            "      \n",
            {
                {TokenKind::NewLine, "\n", LineColumn{1, 7}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        // TODO: more test cases
        // TODO: move somewhere
        TC{
            "fn I32 main() {\n"
            "    return 0;\n"
            "}\n",
            {
                {TokenKind::Fn, "fn", LineColumn{1, 1}},
                {TokenKind::Identifier, "I32", LineColumn{1, 4}},
                {TokenKind::Identifier, "main", LineColumn{1, 8}},
                {TokenKind::ParenOpen, "(", LineColumn{1, 12}},
                {TokenKind::ParenClose, ")", LineColumn{1, 13}},
                {TokenKind::BraceOpen, "{", LineColumn{1, 15}},
                {TokenKind::NewLine, "\n", LineColumn{1, 16}},
                {TokenKind::Return, "return", LineColumn{2, 5}},
                {TokenKind::IntegerLiteral, "0", LineColumn{2, 12}, i64{0}},
                {TokenKind::Semicolon, ";", LineColumn{2, 13}},
                {TokenKind::NewLine, "\n", LineColumn{2, 14}},
                {TokenKind::BraceClose, "}", LineColumn{3, 1}},
                {TokenKind::NewLine, "\n", LineColumn{3, 2}},
                {TokenKind::Eof, "", LineColumn{4, 1}},
            },
        });

    // TODO: add capture

    diagnostics::DiagnosticRecorder diagnostic_recorder{};
    LexHelper lex_helper{};
    REQUIRE_THAT(
        lex_helper.run(test_case.input, diagnostic_recorder),
        RangeEquals(test_case.expected));
    REQUIRE(!diagnostic_recorder.has_errors());
}

TEST_CASE("Can lex keywords", "[cprime][lex]")
{
    struct TC
    {
        std::string_view input;
        std::vector<ExpectedToken> expected;
    };

    auto test_case = GENERATE(
        TC{
            "fn\n",
            {
                {TokenKind::Fn, "fn", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 3}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "return\n",
            {
                {TokenKind::Return, "return", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 7}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "fn return\n",
            {
                {TokenKind::Fn, "fn", LineColumn{1, 1}},
                {TokenKind::Return, "return", LineColumn{1, 4}},
                {TokenKind::NewLine, "\n", LineColumn{1, 10}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        });

    diagnostics::DiagnosticRecorder diagnostic_recorder{};
    LexHelper lex_helper{};
    REQUIRE_THAT(
        lex_helper.run(test_case.input, diagnostic_recorder),
        RangeEquals(test_case.expected));
    REQUIRE(!diagnostic_recorder.has_errors());
}

TEST_CASE("Can lex identifiers", "[cprime][lex]")
{
    struct TC
    {
        std::string_view input;
        std::vector<ExpectedToken> expected;
    };

    auto test_case = GENERATE(
        TC{
            "a\n",
            {
                {TokenKind::Identifier, "a", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "abcdefghijklmnopqrstuvwxyz_0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ\n",
            {
                {
                    TokenKind::Identifier,
                    "abcdefghijklmnopqrstuvwxyz_0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ",
                    LineColumn{1, 1},
                },
                {TokenKind::NewLine, "\n", LineColumn{1, 65}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "___Foo\n",
            {
                {TokenKind::Identifier, "___Foo", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 7}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "return_it\n",
            {
                {TokenKind::Identifier, "return_it", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 10}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        });

    diagnostics::DiagnosticRecorder diagnostic_recorder{};
    LexHelper lex_helper{};
    REQUIRE_THAT(
        lex_helper.run(test_case.input, diagnostic_recorder),
        RangeEquals(test_case.expected));
    REQUIRE(!diagnostic_recorder.has_errors());
}

TEST_CASE("Can lex operators", "[cprime][lex]")
{
    struct TC
    {
        std::string_view input;
        std::vector<ExpectedToken> expected;
    };

    auto test_case = GENERATE(
        TC{
            "+\n",
            {
                {TokenKind::Plus, "+", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "-\n",
            {
                {TokenKind::Minus, "-", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "*\n",
            {
                {TokenKind::Star, "*", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "/\n",
            {
                {TokenKind::Slash, "/", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "%\n",
            {
                {TokenKind::Percent, "%", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "+-*/%\n",
            {
                {TokenKind::Plus, "+", LineColumn{1, 1}},
                {TokenKind::Minus, "-", LineColumn{1, 2}},
                {TokenKind::Star, "*", LineColumn{1, 3}},
                {TokenKind::Slash, "/", LineColumn{1, 4}},
                {TokenKind::Percent, "%", LineColumn{1, 5}},
                {TokenKind::NewLine, "\n", LineColumn{1, 6}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        });

    diagnostics::DiagnosticRecorder diagnostic_recorder{};
    LexHelper lex_helper{};
    REQUIRE_THAT(
        lex_helper.run(test_case.input, diagnostic_recorder),
        RangeEquals(test_case.expected));
    REQUIRE(!diagnostic_recorder.has_errors());
}

TEST_CASE("Can lex punctuation", "[cprime][lex]")
{
    struct TC
    {
        std::string_view input;
        std::vector<ExpectedToken> expected;
    };

    auto test_case = GENERATE(
        TC{
            "{\n",
            {
                {TokenKind::BraceOpen, "{", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "}\n",
            {
                {TokenKind::BraceClose, "}", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "(\n",
            {
                {TokenKind::ParenOpen, "(", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            ")\n",
            {
                {TokenKind::ParenClose, ")", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            ";\n",
            {
                {TokenKind::Semicolon, ";", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            ",\n",
            {
                {TokenKind::Comma, ",", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "{}();,\n",
            {
                {TokenKind::BraceOpen, "{", LineColumn{1, 1}},
                {TokenKind::BraceClose, "}", LineColumn{1, 2}},
                {TokenKind::ParenOpen, "(", LineColumn{1, 3}},
                {TokenKind::ParenClose, ")", LineColumn{1, 4}},
                {TokenKind::Semicolon, ";", LineColumn{1, 5}},
                {TokenKind::Comma, ",", LineColumn{1, 6}},
                {TokenKind::NewLine, "\n", LineColumn{1, 7}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        });

    diagnostics::DiagnosticRecorder diagnostic_recorder{};
    LexHelper lex_helper{};
    REQUIRE_THAT(
        lex_helper.run(test_case.input, diagnostic_recorder),
        RangeEquals(test_case.expected));
    REQUIRE(!diagnostic_recorder.has_errors());
}

TEST_CASE("Can lex string literals", "[cprime][lex]")
{
    struct TC
    {
        std::string_view input;
        std::vector<ExpectedToken> expected;
    };

    auto test_case = GENERATE(
        TC{
            R"("")",
            {
                {TokenKind::StringLiteral, R"("")", LineColumn{1, 1}, ""},
                {TokenKind::NewLine, "\n", LineColumn{1, 3}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            R"("X")",
            {
                {TokenKind::StringLiteral, R"("X")", LineColumn{1, 1}, "X"},
                {TokenKind::NewLine, "\n", LineColumn{1, 4}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            R"("foobar")",
            {
                {
                    TokenKind::StringLiteral,
                    R"("foobar")",
                    LineColumn{1, 1},
                    "foobar",
                },
                {TokenKind::NewLine, "\n", LineColumn{1, 9}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            R"("\n")",
            {
                {TokenKind::StringLiteral, R"("\n")", LineColumn{1, 1}, "\n"},
                {TokenKind::NewLine, "\n", LineColumn{1, 5}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            R"("\"")",
            {
                {TokenKind::StringLiteral, R"("\"")", LineColumn{1, 1}, "\""},
                {TokenKind::NewLine, "\n", LineColumn{1, 5}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            R"("\\")",
            {
                {TokenKind::StringLiteral, R"("\\")", LineColumn{1, 1}, "\\"},
                {TokenKind::NewLine, "\n", LineColumn{1, 5}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            R"("\0\r\n\t\\\"")",
            {
                {
                    TokenKind::StringLiteral,
                    R"("\0\r\n\t\\\"")",
                    LineColumn{1, 1},
                    "\0\r\n\t\\\""s,
                },
                {TokenKind::NewLine, "\n", LineColumn{1, 15}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            R"("Hello! Привет! こんにちは! 🙂🙃")",
            {
                {
                    TokenKind::StringLiteral,
                    R"("Hello! Привет! こんにちは! 🙂🙃")",
                    LineColumn{1, 1},
                    "Hello! Привет! こんにちは! 🙂🙃",
                },
                {TokenKind::NewLine, "\n", LineColumn{1, 27}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        });

    diagnostics::DiagnosticRecorder diagnostic_recorder{};
    LexHelper lex_helper{};
    REQUIRE_THAT(
        lex_helper.run(test_case.input, diagnostic_recorder),
        RangeEquals(test_case.expected));
    REQUIRE(!diagnostic_recorder.has_errors());

    // unclosed empty string literal with eof "
    // unclosed string literal "Я помню чудное мгновенье...
    // unclosed with \n literal "fizzybuzzy
    //
    // unclosed with "\"
    //
    // unclosed with unclosed escape sequence "   \


    // Do not forget to check for errors
}

TEST_CASE("Can lex invalid string literals", "[cprime][lex]")
{
    struct TC
    {
        std::string_view input;
        std::vector<ExpectedToken> expected;
    };

    auto test_case = GENERATE(
        TC{
            R"(")",
            {
                {TokenKind::StringLiteral, R"(")", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            R"("Я помню чудное мгновенье...)",
            {
                {
                    TokenKind::StringLiteral,
                    R"("Я помню чудное мгновенье...)",
                    LineColumn{1, 1},
                },
                {TokenKind::NewLine, "\n", LineColumn{1, 29}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            R"("\")",
            {
                {TokenKind::StringLiteral, R"("\")", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 4}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            R"("\\)",
            {
                {TokenKind::StringLiteral, R"("\\)", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 4}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            R"("\)",
            {
                {TokenKind::StringLiteral, R"("\)", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 3}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "\"\x80\"",
            {
                {TokenKind::StringLiteral, "\"\x80\"", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 4}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        });

    diagnostics::DiagnosticRecorder diagnostic_recorder{};
    LexHelper lex_helper{};
    REQUIRE_THAT(
        lex_helper.run(test_case.input, diagnostic_recorder),
        RangeEquals(test_case.expected));
    REQUIRE(diagnostic_recorder.has_errors());
}

TEST_CASE("Can lex integer literals", "[cprime][lex]")
{
    struct TC
    {
        std::string_view input;
        std::vector<ExpectedToken> expected;
    };

    auto test_case = GENERATE(
        TC{
            "0\n",
            {
                {TokenKind::IntegerLiteral, "0", LineColumn{1, 1}, i64{0}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "1\n",
            {
                {TokenKind::IntegerLiteral, "1", LineColumn{1, 1}, i64{1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "2\n",
            {
                {TokenKind::IntegerLiteral, "2", LineColumn{1, 1}, i64{2}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "3\n",
            {
                {TokenKind::IntegerLiteral, "3", LineColumn{1, 1}, i64{3}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "4\n",
            {
                {TokenKind::IntegerLiteral, "4", LineColumn{1, 1}, i64{4}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "5\n",
            {
                {TokenKind::IntegerLiteral, "5", LineColumn{1, 1}, i64{5}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "6\n",
            {
                {TokenKind::IntegerLiteral, "6", LineColumn{1, 1}, i64{6}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "7\n",
            {
                {TokenKind::IntegerLiteral, "7", LineColumn{1, 1}, i64{7}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "8\n",
            {
                {TokenKind::IntegerLiteral, "8", LineColumn{1, 1}, i64{8}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "9\n",
            {
                {TokenKind::IntegerLiteral, "9", LineColumn{1, 1}, i64{9}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "1234567890\n",
            {
                {TokenKind::IntegerLiteral, "1234567890", LineColumn{1, 1}, i64{1234567890}},
                {TokenKind::NewLine, "\n", LineColumn{1, 11}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "42\n",
            {
                {TokenKind::IntegerLiteral, "42", LineColumn{1, 1}, i64{42}},
                {TokenKind::NewLine, "\n", LineColumn{1, 3}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "9223372036854775807\n",
            {
                {
                    TokenKind::IntegerLiteral,
                    "9223372036854775807",
                    LineColumn{1, 1},
                    i64{9223372036854775807ll},
                },
                {TokenKind::NewLine, "\n", LineColumn{1, 20}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        });

    diagnostics::DiagnosticRecorder diagnostic_recorder{};
    LexHelper lex_helper{};
    REQUIRE_THAT(
        lex_helper.run(test_case.input, diagnostic_recorder),
        RangeEquals(test_case.expected));
    REQUIRE(!diagnostic_recorder.has_errors());
}

TEST_CASE("Can lex invalid integer literals", "[cprime][lex]")
{
    struct TC
    {
        std::string_view input;
        std::vector<ExpectedToken> expected;
    };

    auto test_case = GENERATE(
        TC{
            "01\n",
            {
                {TokenKind::IntegerLiteral, "01", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 3}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "00\n",
            {
                {TokenKind::IntegerLiteral, "00", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 3}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "9223372036854775808\n",
            {
                {
                    TokenKind::IntegerLiteral,
                    "9223372036854775808",
                    LineColumn{1, 1},
                },
                {TokenKind::NewLine, "\n", LineColumn{1, 20}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "111111111111111111111111111111111111111111\n",
            {
                {
                    TokenKind::IntegerLiteral,
                    "111111111111111111111111111111111111111111",
                    LineColumn{1, 1},
                },
                {TokenKind::NewLine, "\n", LineColumn{1, 43}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        });

    diagnostics::DiagnosticRecorder diagnostic_recorder{};
    LexHelper lex_helper{};
    REQUIRE_THAT(
        lex_helper.run(test_case.input, diagnostic_recorder),
        RangeEquals(test_case.expected));
    REQUIRE(diagnostic_recorder.has_errors());
}

TEST_CASE("Can lex errors", "[cprime][lex]")
{
    struct TC
    {
        std::string_view input;
        std::vector<ExpectedToken> expected;
    };

    auto test_case = GENERATE(
        TC{
            "$\n",
            {
                {TokenKind::Error, "$", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            "\xFF\n",
            {
                {TokenKind::Error, "\xFF", LineColumn{1, 1}},
                {TokenKind::NewLine, "\n", LineColumn{1, 2}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            // Invalid out-of-range code point
            "\xF7\xBF\xBF\xBF\n",
            {
                {TokenKind::Error, "\xF7", LineColumn{1, 1}},
                {TokenKind::Error, "\xBF", LineColumn{1, 2}},
                {TokenKind::Error, "\xBF", LineColumn{1, 3}},
                {TokenKind::Error, "\xBF", LineColumn{1, 4}},
                {TokenKind::NewLine, "\n", LineColumn{1, 5}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        },
        TC{
            // Overlong encoding for '@'
            "\xF0\x80\x81\x80\n",
            {
                {TokenKind::Error, "\xF0", LineColumn{1, 1}},
                {TokenKind::Error, "\x80", LineColumn{1, 2}},
                {TokenKind::Error, "\x81", LineColumn{1, 3}},
                {TokenKind::Error, "\x80", LineColumn{1, 4}},
                {TokenKind::NewLine, "\n", LineColumn{1, 5}},
                {TokenKind::Eof, "", LineColumn{2, 1}},
            },
        });

    diagnostics::DiagnosticRecorder diagnostic_recorder{};
    LexHelper lex_helper{};
    REQUIRE_THAT(
        lex_helper.run(test_case.input, diagnostic_recorder),
        RangeEquals(test_case.expected));
    REQUIRE(diagnostic_recorder.has_errors());
}

} // namespace
} // namespace cprime::lex

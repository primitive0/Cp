#include "token.hpp"

#include <cprime/support/prelude.hpp>
#include <cprime/source/source_buffer.hpp>
#include <catch2/catch_test_macros.hpp>

namespace cprime::lex {
namespace {

// TODO: uncomment

// using source::LineColumn;
// using source::SourceBuffer;
// using source::SourceSpan;

// auto make_token(TokenKind kind, std::string_view lexeme)
//     -> std::pair<std::unique_ptr<SourceBuffer>, Token>
// {
//     std::unique_ptr<SourceBuffer> buffer =
//         SourceBuffer::from_content(std::string{lexeme});
//     SourceSpan span{*buffer, LineColumn{1, 1}, buffer->content()};
//     Token token{kind, span};
//     return std::make_pair(std::move(buffer), token);
// }

// template<typename T>
// auto make_token(TokenKind kind, std::string_view lexeme, T value)
//     -> std::pair<std::unique_ptr<SourceBuffer>, Token>
// {
//     std::unique_ptr<SourceBuffer> buffer =
//         SourceBuffer::from_content(std::string{lexeme});
//     SourceSpan span{*buffer, LineColumn{1, 1}, buffer->content()};
//     Token token{kind, span, std::move(value)};
//     return std::make_pair(std::move(buffer), token);
// }

// TEST_CASE("Token is created from TokenKind and SourceSpan", "[cprime][lex]")
// {
//     auto [source_buffer, token] = make_token(TokenKind::Return, "return");

//     CHECK(token.kind() == TokenKind::Return);
//     CHECK(token.lexeme() == "return");
//     CHECK(&token.span().buffer() == &*source_buffer);
//     CHECK(token.span().location() == LineColumn{1, 1});
//     CHECK(token.span().content() == "return");
//     CHECK(token.is_valid());
// }

// TEST_CASE("Token is created from TokenKind and SourceSpan with string value", "[cprime][lex]")
// {
//     auto [source_buffer, token] = make_token(
//         TokenKind::StringLiteral, R"("foo")", "foo");

//     CHECK(token.kind() == TokenKind::StringLiteral);
//     CHECK(token.lexeme() == R"("foo")");
//     CHECK(&token.span().buffer() == &*source_buffer);
//     CHECK(token.span().location() == LineColumn{1, 1});
//     CHECK(token.span().content() == R"("foo")");
//     CHECK(token.is_valid());
//     CHECK(token.string_value() == "foo");
// }

// TEST_CASE("Token is created from TokenKind and SourceSpan with integer value", "[cprime][lex]")
// {
//     auto [source_buffer, token] = make_token(
//         TokenKind::IntegerLiteral, "1234", i64{1234});

//     CHECK(token.kind() == TokenKind::IntegerLiteral);
//     CHECK(token.lexeme() == "1234");
//     CHECK(&token.span().buffer() == &*source_buffer);
//     CHECK(token.span().location() == LineColumn{1, 1});
//     CHECK(token.span().content() == "1234");
//     CHECK(token.is_valid());
//     CHECK(token.integer_value() == 1234);
// }

// TEST_CASE("String literal is invalid if it has no value", "[cprime][lex]")
// {
//     auto [source_buffer, token] = make_token(TokenKind::StringLiteral, "\"bar");

//     CHECK(token.kind() == TokenKind::StringLiteral);
//     CHECK(token.lexeme() == "\"bar");
//     CHECK(&token.span().buffer() == &*source_buffer);
//     CHECK(token.span().location() == LineColumn{1, 1});
//     CHECK(token.span().content() == "\"bar");
//     CHECK(!token.is_valid());
// }

// TEST_CASE("Integer literal is invalid if it has no value", "[cprime][lex]")
// {
//     auto [source_buffer, token] = make_token(TokenKind::IntegerLiteral, "001");

//     CHECK(token.kind() == TokenKind::IntegerLiteral);
//     CHECK(token.lexeme() == "001");
//     CHECK(&token.span().buffer() == &*source_buffer);
//     CHECK(token.span().location() == LineColumn{1, 1});
//     CHECK(token.span().content() == "001");
//     CHECK(!token.is_valid());
// }

} // namespace
} // namespace cprime::lex

#ifndef CPRIME_SUPPORT_TEXT_H_
#define CPRIME_SUPPORT_TEXT_H_

#include <cprime/support/prelude.hpp>
#include <cprime/support/contract.hpp>

namespace cprime::support::text {
namespace utf8 {

inline constexpr char32_t kBom = U'\xFEFF';

inline auto is_ascii_lowercase(char32_t ch) -> bool
{
    return U'a' <= ch && ch <= U'z';
}

inline auto is_ascii_uppercase(char32_t ch) -> bool
{
    return U'A' <= ch && ch <= U'Z';
}

inline auto is_ascii_alphabetic(char32_t ch) -> bool
{
    return is_ascii_lowercase(ch) || is_ascii_uppercase(ch);
}

inline auto is_ascii_digit(char32_t ch) -> bool
{
    return U'0' <= ch && ch <= U'9';
}

inline auto is_ascii_alphanumeric(char32_t ch) -> bool
{
    return is_ascii_alphabetic(ch) || is_ascii_digit(ch);
}

inline auto sequence_length(char32_t ch) -> i32
{
    if (ch < 0x80) {
        return 1;
    } else if (ch < 0x800) {
        return 2;
    } else if (ch < 0x10000) {
        return 3;
    } else if (ch < 0x110000) {
        return 4;
    } else {
        CPRIME_UNREACHABLE("'ch' must be Unicode code point.");
    }
}

} // namespace utf8
} // namespace cprime::support::text

#endif // CPRIME_SUPPORT_TEXT_H_

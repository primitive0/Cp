#ifndef CPRIME_SUPPORT_CONTRACT_H_
#define CPRIME_SUPPORT_CONTRACT_H_

#include <cstdio>
#include <cstdlib>
#include <format>
#include <print>
#include <source_location>
#include <utility>

namespace cprime::support::detail {

template<typename... Args>
[[noreturn]] inline auto fail_assertion(
    std::source_location location,
    std::format_string<Args...> fmt = "",
    Args&&... args)
    -> void
{
    std::print(
        stderr,
        "Assertion failed at {}:{}:{} inside '{}'",
        location.file_name(),
        location.line(),
        location.column(),
        location.function_name());
    if (fmt.get().empty()) {
        std::print(stderr, ".\n");
    } else {
        std::print(
            stderr,
            ":\n  {}\n",
            std::format(fmt, std::forward<Args>(args)...));
    }

    std::abort();
}

} // namespace cprime::support::detail

#define CPRIME_ASSERT(condition, ...)                    \
    ((condition)                                         \
            ? (void)0                                    \
            : ::cprime::support::detail::fail_assertion( \
                  std::source_location::current() __VA_OPT__(, ) __VA_ARGS__))

#ifndef NDEBUG
#define CPRIME_DEBUG_ASSERT(condition, ...) \
    CPRIME_ASSERT(condition __VA_OPT__(, ) __VA_ARGS__)
#else
// Даже в Release сборке мы хотим убедиться, что выражение condition можно
// использовать как bool.
#define CPRIME_DEBUG_ASSERT(condition, ...) \
    ((void)(sizeof((condition) ? 0 : 0)))
#endif

#define CPRIME_UNREACHABLE(...) \
    CPRIME_ASSERT(false __VA_OPT__(, ) __VA_ARGS__)

#endif // CPRIME_SUPPORT_CONTRACT_H_

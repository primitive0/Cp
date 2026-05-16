#ifndef CPRIME_AST_ASTCONTEXT_H_
#define CPRIME_AST_ASTCONTEXT_H_

#include <memory_resource>
#include <cprime/support/prelude.hpp>
#include <cprime/support/contract.hpp>

namespace cprime::ast {

class TranslationUnit;

class AstContext final
{
public:
    AstContext(const AstContext&) = delete;
    auto operator=(const AstContext&) -> AstContext& = delete;

    AstContext(AstContext&&) = delete;
    auto operator=(AstContext&&) -> AstContext& = delete;

    explicit AstContext()
        : arena_{kArenaInitialSize}
        , translation_unit_{nullptr}
    {
    }

    template<typename T, typename... Args>
    auto make(Args&&... args) -> T*
    {
        void* storage = arena_.allocate(sizeof(T), alignof(T));
        // В случае возникновения исключения освобождать память нет
        // необходимости, так как аллокации происходят на арене. Обходимся без
        // блока try-catch.
        return std::construct_at(
            static_cast<T*>(storage),
            std::forward<Args>(args)...);
    }

    // TODO: Make lexer do allocations on AstContext arena
    // На текущий момент это просто хак.
    auto make_string(std::string_view str) -> std::pmr::string
    {
        return std::pmr::string{str, &arena_};
    }

    auto translation_unit() -> TranslationUnit*
    {
        CPRIME_ASSERT(
            translation_unit_ != nullptr,
            "Translation unit is not set for this AstContext.");
        return translation_unit_;
    }

    auto set_translation_unit(TranslationUnit* translation_unit) -> void
    {
        CPRIME_ASSERT(
            translation_unit_ == nullptr,
            "Translation unit is already set for this AstContext.");
        CPRIME_ASSERT(translation_unit != nullptr);
        translation_unit_ = translation_unit;
    }

private:
    static constexpr size_t kArenaInitialSize = 64 * 1024; // 64 KiB

    std::pmr::monotonic_buffer_resource arena_;
    TranslationUnit* translation_unit_;

    // TODO: function lookup table
};

} // namespace cprime::ast

#endif // CPRIME_AST_ASTCONTEXT_H_

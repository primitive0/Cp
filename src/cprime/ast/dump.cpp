#include "dump.hpp"

#include "decl.hpp"
#include "expr.hpp"
#include "stmt.hpp"
#include "translation_unit.hpp"
#include <iterator>
#include <ostream>
#include <type_traits>
#include <unordered_set>
#include <cprime/support/prelude.hpp>
#include <cprime/source/source_buffer.hpp>
#include <cprime/support/contract.hpp>

namespace cprime::ast {

namespace {

class SyntaxAstDumper final
    : private StmtVisitor<>
    , private ExprVisitor<>
{
    friend class StmtVisitor<>;
    friend class ExprVisitor<>;

public:
    SyntaxAstDumper(const SyntaxAstDumper&) = delete;
    auto operator=(const SyntaxAstDumper&) -> SyntaxAstDumper& = delete;

    SyntaxAstDumper(SyntaxAstDumper&&) = delete;
    auto operator=(SyntaxAstDumper&&) -> SyntaxAstDumper& = delete;

    explicit SyntaxAstDumper(std::ostream& os)
        : os_{os}
        , depth_{0}
        , active_tree_guides_{}
    {
    }

    auto dump(TranslationUnit* translation_unit) -> void
    {
        visit(translation_unit);
    }

private:
    class ChildLevelGuard final
    {
    public:
        ChildLevelGuard(const ChildLevelGuard&) = delete;
        auto operator=(const ChildLevelGuard&) -> ChildLevelGuard& = delete;

        ChildLevelGuard(ChildLevelGuard&&) = delete;
        auto operator=(ChildLevelGuard&&) -> ChildLevelGuard& = delete;

        explicit ChildLevelGuard(SyntaxAstDumper& dumper)
            : dumper_{&dumper}
        {
            CPRIME_DEBUG_ASSERT(
                dumper_->depth_ >= 0 &&
                dumper_->active_tree_guides_.size() <= dumper_->depth_);

            ++dumper_->depth_;
            dumper_->active_tree_guides_.insert(dumper_->depth_);
        }

        ~ChildLevelGuard()
        {
            dumper_->active_tree_guides_.erase(dumper_->depth_);
            --dumper_->depth_;

            CPRIME_DEBUG_ASSERT(
                dumper_->depth_ >= 0 &&
                dumper_->active_tree_guides_.size() <= dumper_->depth_);
        }

    private:
        SyntaxAstDumper* dumper_;
    };

    auto visit(TranslationUnit* translation_unit) -> void
    {
        node("TranslationUnit", translation_unit->span());
        node_end();

        ChildLevelGuard guard{*this};
        dump_children(
            translation_unit->begin(),
            translation_unit->end(),
            /*is_last=*/true);
    }

    auto visit(FunctionDecl* function_decl) -> void
    {
        node("FunctionDecl", function_decl->span());
        node_attr(function_decl->name());
        node_end();

        ChildLevelGuard guard{*this};
        dump_child(function_decl->body(), /*is_last=*/true);
    }

    auto visit(Block* block) -> void
    {
        node("Block", block->span());
        node_end();

        ChildLevelGuard guard{*this};
        dump_children(block->begin(), block->end(), /*is_last=*/true);
    }

    auto visit(EmptyStmt* empty_stmt) -> void
    {
        node("EmptyStmt", empty_stmt->span());
        node_end();
    }

    auto visit(ReturnStmt* return_stmt) -> void
    {
        node("ReturnStmt", return_stmt->span());
        node_end();

        ChildLevelGuard guard{*this};
        dump_child(return_stmt->value(), /*is_last=*/true);
    }

    auto visit(ExprStmt* expr_stmt) -> void
    {
        node("ExprStmt", expr_stmt->span());
        node_end();

        ChildLevelGuard guard{*this};
        dump_child(expr_stmt->expr(), /*is_last=*/true);
    }

    auto visit(ExprList* expr_list) -> void
    {
        node("ExprList", expr_list->span());
        node_end();

        ChildLevelGuard guard{*this};
        dump_children(expr_list->begin(), expr_list->end(), /*is_last=*/true);
    }

    auto visit(StringExpr* string_expr) -> void
    {
        node("StringExpr", string_expr->span());
        os_ << " " << string_expr->span().content();
        node_end();
    }

    auto visit(IntegerExpr* integer_expr) -> void
    {
        node("IntegerExpr", integer_expr->span());
        os_ << " " << integer_expr->span().content();
        node_end();
    }

    auto visit(NameExpr* name_expr) -> void
    {
        node("NameExpr", name_expr->span());
        node_attr(name_expr->name());
        node_end();
    }

    auto visit(CallExpr* call_expr) -> void
    {
        node("CallExpr", call_expr->span());
        node_attr(call_expr->callee());
        if (call_expr->is_builtin_call()) {
            node_tag("builtin");
        }
        node_end();

        ChildLevelGuard guard{*this};
        dump_child(call_expr->args(), /*is_last=*/true);
    }

    auto visit(UnaryExpr* unary_expr) -> void
    {
        node("UnaryExpr", unary_expr->span());
        node_attr(unary_expr->op());
        node_end();

        ChildLevelGuard guard{*this};
        dump_child(unary_expr->operand(), /*is_last=*/true);
    }

    auto visit(BinaryExpr* binary_expr) -> void
    {
        node("BinaryExpr", binary_expr->span());
        node_attr(binary_expr->op());
        node_end();

        ChildLevelGuard guard{*this};
        dump_child(binary_expr->left());
        dump_child(binary_expr->right(), /*is_last=*/true);
    }

    auto node(std::string_view name, source::SourceSpan span) -> void
    {
        i32 line = span.location().line;
        i32 column = span.location().column;
        os_ << name << " <" << line << ":" << column << ">";
    }

    template<typename T>
    auto node_attr(std::string_view name, const T& value) -> void
    {
        os_ << " " << name << "=" << format_value(value);
    }

    template<typename T>
    auto node_attr(const T& value) -> void
    {
        os_ << " " << format_value(value);
    }

    auto node_tag(std::string_view tag) -> void
    {
        os_ << " @" << tag;
    }

    auto node_end() -> void
    {
        os_ << "\n";
    }

    auto format_value(std::string_view str) -> std::string_view
    {
        return str;
    }

    auto format_value(UnaryOp kind) -> std::string_view
    {
        switch (kind) {
        case UnaryOp::Minus:
            return "-";
        case UnaryOp::Increment:
            return "++";
        case UnaryOp::Decrement:
            return "--";
        }
        CPRIME_UNREACHABLE("Unknown unary operator kind.");
    }

    auto format_value(BinaryOp kind) -> std::string_view
    {
        switch (kind) {
        case BinaryOp::Add:
            return "+";
        case BinaryOp::Sub:
            return "-";
        case BinaryOp::Mul:
            return "*";
        case BinaryOp::Div:
            return "/";
        case BinaryOp::Modulo:
            return "%";
        }
        CPRIME_UNREACHABLE("Unknown binary operator kind.");
    }

    template<std::bidirectional_iterator It>
    auto dump_children(It begin, It end, bool is_last = false) -> void
    {
        if (begin == end) {
            return;
        }
        It last = std::prev(end);
        for (It current = begin; current != last; ++current) {
            dump_child(&*current);
        }
        dump_child(&*last, is_last);
    }

    template<typename T>
    auto dump_child(T child, bool is_last = false) -> void
    {
        CPRIME_DEBUG_ASSERT(depth_ > 0);

        if (is_last) {
            size_t num_removed = active_tree_guides_.erase(depth_);
            CPRIME_DEBUG_ASSERT(num_removed == 1);
        }

        for (i32 i = 1; i < depth_; ++i) {
            if (active_tree_guides_.contains(i)) {
                os_ << "| ";
            } else {
                os_ << "  ";
            }
        }
        os_ << (is_last ? "`-" : "|-");

        dispatch(child);
    }

    template<typename T>
    auto dispatch(T child) -> void
    {
        if constexpr (std::is_same_v<decltype(child), Stmt*>) {
            return dispatch_stmt(child);
        } else if constexpr (std::is_same_v<decltype(child), Expr*>) {
            return dispatch_expr(child);
        } else {
            return visit(child);
        }
    }

private:
    std::ostream& os_;
    i32 depth_;
    std::unordered_set<i32> active_tree_guides_;

    // TODO: можно улучшить производительность tree guides, используя стек
    // булов.
};

} // namespace

auto dump_syntax_ast(
    std::ostream& os,
    TranslationUnit* translation_unit) -> void
{
    CPRIME_DEBUG_ASSERT(translation_unit != nullptr);

    SyntaxAstDumper{os}.dump(translation_unit);
}

} // namespace cprime::ast

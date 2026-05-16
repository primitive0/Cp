#include "lower.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <cprime/ast/ast.hpp>
#include <cprime/support/contract.hpp>

namespace cprime::codegen_llvm {

namespace {

class LowerToLlvmModule final
    : private ast::StmtVisitor<>
{
    friend class ast::StmtVisitor<>;

public:
    explicit LowerToLlvmModule(
        llvm::LLVMContext& llvm_context,
        llvm::Module& module,
        ast::AstContext& ast_context)
        : llvm_context_{&llvm_context}
        , module_{&module}
        , builder_{llvm_context}
        , ast_context_{&ast_context}
    {
    }

    auto lower() -> void
    {
        visit(ast_context_->translation_unit());
    }

private:
    auto visit(ast::TranslationUnit* translation_unit) -> void
    {
        for (ast::FunctionDecl& function_decl_ref : *translation_unit) {
            ast::FunctionDecl* function_decl = &function_decl_ref;
            if (function_decl->name() == "main") {
                visit(function_decl);
                return;
            }
        }
        CPRIME_UNREACHABLE("main() function not found.");
    }

    auto visit(ast::FunctionDecl* function_decl) -> void
    {
        llvm::FunctionType* ir_func_signature = llvm::FunctionType::get(
            builder_.getInt32Ty(),
            /*isVarArg=*/false);
        llvm::Function* ir_func = llvm::Function::Create(
            ir_func_signature,
            llvm::Function::ExternalLinkage,
            function_decl->name(),
            module_);

        builder_.SetInsertPoint(
            llvm::BasicBlock::Create(*llvm_context_, "entry", ir_func));
        visit(function_decl->body());

        // LLVM требует, чтобы все базовые блоки завершались терминальной
        // инструкцией.
        if (builder_.GetInsertBlock()->getTerminator() == nullptr) {
            builder_.CreateUnreachable();
        }
    }

    auto visit(ast::Block* block) -> void
    {
        for (ast::Stmt& stmt_ref : *block) {
            accept_stmt(&stmt_ref);
        }
    }

    auto visit(ast::EmptyStmt*) -> void
    {
    }

    auto visit(ast::ReturnStmt* return_stmt) -> void
    {
        if (builder_.GetInsertBlock()->getTerminator() != nullptr) {
            return;
        }

        ast::Expr* return_value = return_stmt->value();
        CPRIME_ASSERT(
            return_value->kind() == ast::NodeKind::IntegerExpr,
            "Currently only integers are allowed in return statement.");
        i64 return_value_i64 =
            static_cast<ast::IntegerExpr*>(return_value)->value();
        i32 return_value_i32 = static_cast<i32>(return_value_i64);

        builder_.CreateRet(builder_.getInt32(return_value_i32));
    }

    auto visit(ast::CallStmt* call_stmt) -> void
    {
        if (builder_.GetInsertBlock()->getTerminator() != nullptr) {
            return;
        }

        CPRIME_ASSERT(
            call_stmt->name() == "print" || call_stmt->name() == "println",
            "Unknown function '{}' to call.", call_stmt->name());

        gen_print_call(call_stmt->args());
        if (call_stmt->name() == "println") {
            builder_.CreateCall(
                get_puts_ir_callee(),
                {
                    builder_.CreateGlobalString(""),
                });
        }
    }

    // TODO: refactor this method later
    auto gen_print_call(ast::ExprList* args) -> void
    {
        llvm::FunctionCallee printf_ir_callee = get_printf_ir_callee();
        for (ast::Expr& expr_ref : *args) {
            ast::Expr* expr = &expr_ref;
            CPRIME_ASSERT(
                expr->kind() == ast::NodeKind::StringExpr,
                "Currently only string literals are allowed in print/println.");
            auto* string_expr = static_cast<ast::StringExpr*>(expr);

            builder_.CreateCall(
                printf_ir_callee,
                {
                    builder_.CreateGlobalString("%s"),
                    builder_.CreateGlobalString(
                        std::string_view{string_expr->value()}),
                });
        }
    }

    auto get_printf_ir_callee() -> llvm::FunctionCallee
    {
        llvm::FunctionType* signature = llvm::FunctionType::get(
            builder_.getInt32Ty(),
            {builder_.getPtrTy()},
            /*isVarArg=*/true);
        return module_->getOrInsertFunction("printf", signature);
    }

    auto get_puts_ir_callee() -> llvm::FunctionCallee
    {
        llvm::FunctionType* signature = llvm::FunctionType::get(
            builder_.getInt32Ty(),
            {builder_.getPtrTy()},
            /*isVarArg=*/false);
        return module_->getOrInsertFunction("puts", signature);
    }

private:
    llvm::LLVMContext* llvm_context_;
    llvm::Module* module_;
    llvm::IRBuilder<> builder_;

    ast::AstContext* ast_context_;
};

} // namespace

auto lower_to_llvm_ir(
    llvm::LLVMContext& llvm_context,
    ast::AstContext& ast_context)
    -> std::unique_ptr<llvm::Module>
{
    auto module = std::make_unique<llvm::Module>("main module", llvm_context);

    LowerToLlvmModule{llvm_context, *module, ast_context}.lower();

    // TODO: pass error output stream through params
    if (llvm::verifyModule(*module, &llvm::errs())) {
        CPRIME_UNREACHABLE("LLVM Module verification failed.");
    }

    return module;
}

} // namespace cprime::codegen_llvm

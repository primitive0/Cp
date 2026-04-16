#include "codegen.hpp"

#include <cassert>
#include <cstdlib>
#include <format>
#include <iostream>
#include <unordered_map>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Triple.h>

namespace cprime::compiler::codegen_llvm {

namespace {

class LowerAstToLlvmModule final : private ast::Visitor
{
public:
    explicit LowerAstToLlvmModule(
        llvm::LLVMContext& context,
        llvm::Module& module)
        : context_{&context}
        , module_{&module}
        , builder_{context}
        , tu_functions_{}
        , current_irblock_{nullptr}
        , block_terminated_{false}
    {
    }

    auto lower(ast::AstContext& context) -> void
    {
        context.translation_unit->accept(*this);
        if (llvm::verifyModule(*module_, &llvm::errs())) {
            assert(false && "LLVM Module is broken.");
        }
    }

private:
    auto visit(ast::TranslationUnit& tu) -> void override
    {
        for (auto& function : tu.functions) {
            auto [_, ok] = tu_functions_.insert({function->name, &*function});
            if (!ok) {
                std::cerr << "duplicate function detected: '"
                          << function->name << "'\n";
                assert(false);
            }
        }

        if (auto it = tu_functions_.find("main"); it != tu_functions_.cend()) {
            ast::Function& main_function = *it->second;
            generate_function(main_function);
        } else {
            std::cerr << "main() function not found\n";
            assert(false);
        }
    }

    auto generate_function(ast::Function& function) -> void
    {
        auto* irfunc_type = llvm::FunctionType::get(
            builder_.getInt32Ty(), /*isVarArg=*/false);
        auto* irfunc = llvm::Function::Create(
            irfunc_type,
            llvm::Function::ExternalLinkage,
            function.name,
            module_);

        auto* irblock = llvm::BasicBlock::Create(*context_, "entry", irfunc);
        builder_.SetInsertPoint(irblock);
        block_terminated_ = false;
        for (auto& stmt : function.stmts) {
            stmt->accept(*this);
            if (block_terminated_) {
                break;
            }
        }
        if (!block_terminated_) {
            builder_.CreateUnreachable();
        }
    }

    auto visit(ast::ReturnStmt& stmt) -> void override
    {
        auto* integer_expr = dynamic_cast<ast::IntegerExpr*>(&*stmt.value);
        if (integer_expr == nullptr) {
            assert(false && "Only integers allowed in return statement.");
        }
        builder_.CreateRet(
            builder_.getInt32(
                static_cast<i32>(integer_expr->value)));
        block_terminated_ = true;
    }

    auto visit(ast::PrintStmt& stmt) -> void override
    {
        auto printf_fn = get_printf();
        for (auto& arg : stmt.args) {
            auto* string_expr = dynamic_cast<ast::StringExpr*>(&*arg);
            assert(string_expr != nullptr &&
                   "Only string expressions allowed in print.");
            builder_.CreateCall(
                printf_fn,
                {
                    builder_.CreateGlobalString("%s"),
                    builder_.CreateGlobalString(string_expr->value),
                });
        }
    }

    auto visit(ast::PrintlnStmt& stmt) -> void override
    {
        auto printf_fn = get_printf();
        for (auto& arg : stmt.args) {
            auto* string_expr = dynamic_cast<ast::StringExpr*>(&*arg);
            assert(string_expr != nullptr &&
                   "Only string expressions allowed in println.");
            builder_.CreateCall(
                printf_fn,
                {
                    builder_.CreateGlobalString("%s"),
                    builder_.CreateGlobalString(string_expr->value),
                });
        }
        builder_.CreateCall(get_puts(), {builder_.CreateGlobalString("")});
    }

    auto visit(ast::EmptyStmt&) -> void override {}

    auto get_printf() -> llvm::FunctionCallee
    {
        auto* printf_type = llvm::FunctionType::get(
            builder_.getInt32Ty(), {builder_.getPtrTy()}, /*isVarArg=*/true);
        return module_->getOrInsertFunction("printf", printf_type);
    }

    auto get_puts() -> llvm::FunctionCallee
    {
        auto* puts_type = llvm::FunctionType::get(
            builder_.getInt32Ty(), {builder_.getPtrTy()}, /*isVarArg=*/false);
        return module_->getOrInsertFunction("puts", puts_type);
    }

private:
    llvm::LLVMContext* context_;
    llvm::Module* module_;
    llvm::IRBuilder<> builder_;

    std::unordered_map<std::string_view, ast::Function*> tu_functions_;

    llvm::BasicBlock* current_irblock_;
    bool block_terminated_;
};

} // namespace

auto generate_executable(
    ast::AstContext& ast_context,
    const std::filesystem::path& exe_path) -> void
{
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmParser();
    LLVMInitializeX86AsmPrinter();

    llvm::LLVMContext llvm_ctx{};
    llvm::Module module{"main module", llvm_ctx};

    llvm::Triple triple("x86_64-pc-linux-gnu");
    module.setTargetTriple(triple);

    std::string error;
    const auto* target = llvm::TargetRegistry::lookupTarget(
        triple.getTriple(), error);
    assert(target != nullptr && "Target lookup failed");

    llvm::TargetOptions opt;
    auto* machine = target->createTargetMachine(
        triple, "generic", "", opt, llvm::Reloc::PIC_);

    module.setDataLayout(machine->createDataLayout());

    LowerAstToLlvmModule{llvm_ctx, module}.lower(ast_context);

    const auto obj_path =
        exe_path.parent_path() / (exe_path.stem().string() + ".o");

    std::error_code ec;
    llvm::raw_fd_ostream dest(obj_path.string(), ec, llvm::sys::fs::OF_None);
    assert(!ec && "Could not open object file for writing");

    llvm::legacy::PassManager pm;
    const auto failed = machine->addPassesToEmitFile(
        pm, dest, nullptr, llvm::CodeGenFileType::ObjectFile);
    assert(!failed && "TargetMachine cannot emit object files");

    pm.run(module);
    dest.flush();

    const auto link_cmd = std::format(
        "cc {} -o {}", obj_path.string(), exe_path.string());
    const auto ret = std::system(link_cmd.c_str());
    assert(ret == 0 && "Linking failed");
}

} // namespace cprime::compiler::codegen_llvm

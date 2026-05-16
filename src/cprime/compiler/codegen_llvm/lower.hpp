#ifndef CPRIME_COMPILER_CODEGENLLVM_LOWER_H_
#define CPRIME_COMPILER_CODEGENLLVM_LOWER_H_

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <cprime/support/prelude.hpp>
#include <cprime/ast/ast_context.hpp>

namespace cprime::codegen_llvm {

auto lower_to_llvm_ir(
    llvm::LLVMContext& llvm_context,
    ast::AstContext& ast_context)
    -> std::unique_ptr<llvm::Module>;

} // namespace cprime::codegen_llvm

#endif // CPRIME_COMPILER_CODEGENLLVM_LOWER_H_

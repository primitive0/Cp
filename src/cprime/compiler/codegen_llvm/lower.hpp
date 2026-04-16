#ifndef CPRIME_COMPILER_CODEGENLLVM_LOWER_H_
#define CPRIME_COMPILER_CODEGENLLVM_LOWER_H_

#include <llvm/IR/Module.h>
#include <cprime/support/prelude.hpp>
#include <cprime/ast/ast.hpp>

namespace cprime::compiler::codegen_llvm {

auto lower_to_llvm_ir(ast::AstContext& ast_context)
    -> std::unique_ptr<llvm::Module>;

} // namespace cprime::compiler::codegen_llvm

#endif // CPRIME_COMPILER_CODEGENLLVM_LOWER_H_

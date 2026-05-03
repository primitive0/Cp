#ifndef CPRIME_COMPILER_CODEGENLLVM_CODEGEN_H_
#define CPRIME_COMPILER_CODEGENLLVM_CODEGEN_H_

#include <filesystem>
#include <cprime/support/prelude.hpp>
#include <cprime/ast/ast.hpp>

namespace cprime::codegen_llvm {

auto generate_executable(
    ast::AstContext& ast_context,
    const std::filesystem::path& exe_path) -> void;

} // namespace cprime::codegen_llvm

#endif // CPRIME_COMPILER_CODEGENLLVM_CODEGEN_H_

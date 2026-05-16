#include "codegen.hpp"

#include "lower.hpp"
#include <cstdlib>
#include <format>
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
#include <cprime/support/contract.hpp>

namespace cprime::codegen_llvm {

auto initialize_llvm_targets() -> void
{
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmParser();
    LLVMInitializeX86AsmPrinter();
}

// TODO: refactor this function
auto generate_executable(
    ast::AstContext& ast_context,
    const std::filesystem::path& exe_path) -> void
{
    llvm::LLVMContext llvm_context{};

    std::unique_ptr<llvm::Module> module =
        lower_to_llvm_ir(llvm_context, ast_context);

    llvm::Triple target_triple{"x86_64-pc-linux-gnu"};
    module->setTargetTriple(target_triple);

    std::string error{};
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(
        target_triple, error);
    CPRIME_ASSERT(target != nullptr && "Target lookup failed");

    llvm::TargetOptions opt{};
    auto* machine = target->createTargetMachine(
        target_triple, "generic", "", opt, llvm::Reloc::PIC_);

    module->setDataLayout(machine->createDataLayout());

    const auto obj_path =
        exe_path.parent_path() / (exe_path.stem().string() + ".o");

    std::error_code ec;
    llvm::raw_fd_ostream dest(obj_path.string(), ec, llvm::sys::fs::OF_None);
    assert(!ec && "Could not open object file for writing");

    llvm::legacy::PassManager pm;
    const auto failed = machine->addPassesToEmitFile(
        pm, dest, nullptr, llvm::CodeGenFileType::ObjectFile);
    CPRIME_ASSERT(!failed, "TargetMachine cannot emit object files");

    pm.run(*module);
    dest.flush();

    const auto link_cmd = std::format(
        "cc {} -o {}", obj_path.string(), exe_path.string());
    const auto ret = std::system(link_cmd.c_str());
    CPRIME_ASSERT(ret == 0, "Linking failed");
}

} // namespace cprime::codegen_llvm

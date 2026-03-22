#include <cstdlib>
#include <iostream>
#include <memory>
#include <print>
#include <string>

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

int main()
{
    // ── 1. Initialize the X86 back-end ──────────────────────
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmParser();
    LLVMInitializeX86AsmPrinter();

    // ── 2. Create LLVM context & module ─────────────────────
    llvm::LLVMContext ctx;
    auto mod = std::make_unique<llvm::Module>("hello_llvm", ctx);

    // ── 3. Configure target: x86_64-pc-linux-gnu ────────────
    llvm::Triple triple("x86_64-pc-linux-gnu");
    mod->setTargetTriple(triple);

    std::string error;
    const auto* target = llvm::TargetRegistry::lookupTarget(
        triple.getTriple(), error);
    if (!target) {
        std::cerr << "Target lookup failed: " << error << '\n';
        return 1;
    }

    llvm::TargetOptions opt;
    auto* machine = target->createTargetMachine(
        triple, "generic", "", opt, llvm::Reloc::PIC_);

    mod->setDataLayout(machine->createDataLayout());

    // ── 4. Build the IR ─────────────────────────────────────
    llvm::IRBuilder<> builder(ctx);

    // Declare:  int puts(const char*)
    auto* putsType = llvm::FunctionType::get(
        builder.getInt32Ty(), {builder.getPtrTy()}, /*isVarArg=*/false);
    auto putsFn = mod->getOrInsertFunction("puts", putsType);

    // Define:  int main(void)
    auto* mainType = llvm::FunctionType::get(
        builder.getInt32Ty(), /*isVarArg=*/false);
    auto* mainFn = llvm::Function::Create(
        mainType, llvm::Function::ExternalLinkage, "main", mod.get());

    // entry:
    auto* entry = llvm::BasicBlock::Create(ctx, "entry", mainFn);
    builder.SetInsertPoint(entry);

    //   call i32 @puts(ptr @.str)
    auto* helloStr = builder.CreateGlobalString("Hello, LLVM!");
    builder.CreateCall(putsFn, {helloStr});

    //   ret i32 0
    builder.CreateRet(builder.getInt32(0));

    // ── 5. Verify the module ────────────────────────────────
    if (llvm::verifyModule(*mod, &llvm::errs())) {
        std::cerr << "Module verification failed!\n";
        return 1;
    }

    // (Optional) Dump the generated IR to stdout
    std::println("─── Generated LLVM IR ───");
    mod->print(llvm::outs(), nullptr);
    std::println("─────────────────────────\n");

    // ── 6. Emit native object file ──────────────────────────
    constexpr auto objPath = "hello.o";

    std::error_code ec;
    llvm::raw_fd_ostream dest(objPath, ec, llvm::sys::fs::OF_None);
    if (ec) {
        std::cerr << "Could not open " << objPath << ": "
                  << ec.message() << '\n';
        return 1;
    }

    llvm::legacy::PassManager pm;
    if (machine->addPassesToEmitFile(
            pm, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        std::cerr << "TargetMachine cannot emit object files\n";
        return 1;
    }

    pm.run(*mod);
    dest.flush();

    std::println("Object file written: {}", objPath);

    // ── 7. Link into a final executable ─────────────────────
    if (std::system("cc hello.o -o hello") != 0) {
        std::cerr << "Linking failed!\n";
        return 1;
    }

    std::println("Executable created: ./hello");
    return 0;
}

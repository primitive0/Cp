# TODO: configure LLVM properly
find_package(LLVM REQUIRED CONFIG)
# Distro LLVM is typically built as a single libLLVM.so.
# Detect this and link accordingly.
if(TARGET LLVM)
    # Single shared library build — just use the LLVM target
    set(llvm_libs LLVM)
    message(STATUS "Linking against single shared libLLVM")
else()
    # Component-library build — resolve individual libs
    llvm_map_components_to_libnames(llvm_libs
        Core
        Support
        X86CodeGen
        X86AsmParser
        X86Desc
        X86Info
    )
    message(STATUS "Linking against component libraries: ${llvm_libs}")
endif()


add_library(cprime_compiler_codegen_llvm STATIC
  lower.cpp
  codegen.cpp)
target_link_libraries(cprime_compiler_codegen_llvm
  PRIVATE
    ${llvm_libs}
  PUBLIC
    cprime_headers
    cprime_support
    cprime_ast)
# TODO: format lines below
target_include_directories(cprime_compiler_codegen_llvm SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(cprime_compiler_codegen_llvm PRIVATE ${LLVM_DEFINITIONS})

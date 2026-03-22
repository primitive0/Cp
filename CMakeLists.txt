cmake_minimum_required(VERSION 3.28)

project(Cprime LANGUAGES C CXX)
enable_testing()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(IPOSupport)


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


add_executable(hello_llvm src/compiler/driver/main.cpp)
target_compile_definitions(hello_llvm PRIVATE ${LLVM_DEFINITIONS})
target_link_libraries(hello_llvm PRIVATE ${llvm_libs})

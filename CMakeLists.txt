cmake_minimum_required(VERSION 3.28)

project(Cprime LANGUAGES C CXX)
enable_testing()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(IPOSupport)


find_package(magic_enum REQUIRED)
find_package(utf8cpp REQUIRED)
find_package(Boost REQUIRED)

find_package(Catch2 REQUIRED)
include(Catch)


# add_executable(hello_llvm src/cprime/compiler/driver/main.cpp)
# target_compile_definitions(hello_llvm PRIVATE ${LLVM_DEFINITIONS})
# target_link_libraries(hello_llvm PRIVATE ${llvm_libs})

add_subdirectory(src/)
add_subdirectory(test/)

add_library(cprime_ast STATIC
  dump.cpp)
target_link_libraries(cprime_ast
  PUBLIC
    cprime_headers
    cprime_support
    Boost::headers
    magic_enum::magic_enum)

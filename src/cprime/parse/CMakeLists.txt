add_library(cprime_parse STATIC
  parse.cpp)
target_link_libraries(cprime_parse
  PRIVATE
    cprime_lex
  PUBLIC
    cprime_headers
    cprime_support
    cprime_source
    cprime_diagnostics
    cprime_ast)

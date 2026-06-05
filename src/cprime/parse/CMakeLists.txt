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

add_executable(cprime_parse_file_test
  parse_file_test.cpp)
target_link_libraries(cprime_parse_file_test
  PRIVATE
    cprime_support
    cprime_source
    cprime_diagnostics
    cprime_ast
    cprime_parse
    cprime_testing)

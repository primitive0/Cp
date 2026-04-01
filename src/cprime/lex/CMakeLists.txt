add_library(cprime_lex STATIC
  lex.cpp)
target_link_libraries(cprime_lex
  PRIVATE
    utf8cpp::utf8cpp
  PUBLIC
    cprime_headers
    cprime_support
    cprime_source
    cprime_diagnostics)

add_executable(cprime_lex_unittests
  token_unittest.cpp
  lex_unittest.cpp)
target_link_libraries(cprime_lex_unittests
  PRIVATE
    cprime_lex
    magic_enum::magic_enum
    Catch2::Catch2WithMain)
catch_discover_tests(cprime_lex_unittests)

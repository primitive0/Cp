add_library(cprime_ast INTERFACE)
target_link_libraries(cprime_ast
  INTERFACE
    cprime_headers
    cprime_support
    Boost::headers)

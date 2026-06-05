add_library(cprime_testing STATIC
  file_test.cpp)
target_link_libraries(cprime_testing
  PRIVATE
    cxxopts::cxxopts
  PUBLIC
    cprime_headers
    cprime_support)

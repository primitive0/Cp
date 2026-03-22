add_library(cprime_source STATIC
  source_buffer.cpp)
target_link_libraries(cprime_source
  PUBLIC
    cprime_headers
    cprime_support)

add_executable(cprime_source_unittests
  source_buffer_unittest.cpp)
target_link_libraries(cprime_source_unittests
  PRIVATE
    cprime_source
    Catch2::Catch2WithMain)
catch_discover_tests(cprime_source_unittests)

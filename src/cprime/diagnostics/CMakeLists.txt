add_library(cprime_diagnostics STATIC
  diagnostic_recorder.cpp)
target_link_libraries(cprime_diagnostics
  PUBLIC
    cprime_headers
    cprime_support
    cprime_source)

add_executable(cprime_diagnostics_unittests
  diagnostic_recorder_unittest.cpp)
target_link_libraries(cprime_diagnostics_unittests
  PRIVATE
    cprime_diagnostics
    Catch2::Catch2WithMain)
catch_discover_tests(cprime_diagnostics_unittests)

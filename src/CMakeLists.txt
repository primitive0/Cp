add_library(cprime_headers INTERFACE)
target_include_directories(cprime_headers
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(cprime/)

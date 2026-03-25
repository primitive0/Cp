add_library(cprime_support STATIC
  temp_file.cpp)
target_link_libraries(cprime_support INTERFACE cprime_headers)

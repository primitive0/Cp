configure_file(lit.site.cfg.py.in lit.site.cfg.py @ONLY)

find_program(LIT_EXE
  NAMES lit llvm-lit
  REQUIRED)

add_custom_target(check
  COMMAND ${LIT_EXE} ${CMAKE_CURRENT_BINARY_DIR} -v
  DEPENDS cpc)

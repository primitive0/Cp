find_program(LIT_EXE NAMES lit llvm-lit REQUIRED)
find_program(FILECHECK_EXE FileCheck REQUIRED)

configure_file(lit.site.cfg.py.in lit.site.cfg.py @ONLY)

add_custom_target(check
  COMMAND ${LIT_EXE} ${CMAKE_CURRENT_BINARY_DIR} -v
  DEPENDS cpc)

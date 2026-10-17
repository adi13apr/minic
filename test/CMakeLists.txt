find_program(LIT_EXECUTABLE NAMES lit REQUIRED)
find_program(FILECHECK_EXECUTABLE NAMES FileCheck
             HINTS ${LLVM_TOOLS_BINARY_DIR} REQUIRED)

configure_file(lit.site.cfg.py.in lit.site.cfg.py @ONLY)

add_custom_target(check
  COMMAND ${LIT_EXECUTABLE} -sv ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS minic minicRuntime
  USES_TERMINAL
  COMMENT "Running minic lit tests")
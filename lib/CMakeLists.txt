add_library(minicCompiler
  Diagnostic.cpp
  Lexer.cpp
  Token.cpp)

target_include_directories(minicCompiler PUBLIC ${PROJECT_SOURCE_DIR}/include)

llvm_map_components_to_libnames(MINIC_COMPILER_LLVM_LIBS support)
target_link_libraries(minicCompiler PUBLIC ${MINIC_COMPILER_LLVM_LIBS})
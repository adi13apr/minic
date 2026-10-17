add_library(minicCompiler
  AST.cpp
  ASTDumper.cpp
  CodeGen.cpp
  Diagnostic.cpp
  Lexer.cpp
  Optimizer.cpp
  Parser.cpp
  Sema.cpp
  Token.cpp)

target_include_directories(minicCompiler PUBLIC ${PROJECT_SOURCE_DIR}/include)

llvm_map_components_to_libnames(MINIC_COMPILER_LLVM_LIBS
  core passes support targetparser)
target_link_libraries(minicCompiler PUBLIC ${MINIC_COMPILER_LLVM_LIBS})
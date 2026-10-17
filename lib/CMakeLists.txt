set(MINIC_COMPILER_SOURCES
  AST.cpp
  ASTDumper.cpp
  CodeGen.cpp
  Diagnostic.cpp
  Lexer.cpp
  Optimizer.cpp
  Parser.cpp
  Sema.cpp
  Token.cpp)

set(MINIC_COMPILER_LLVM_COMPONENTS core passes support targetparser)

if(MINIC_HAS_NVPTX)
  list(APPEND MINIC_COMPILER_SOURCES PTXEmitter.cpp)
  list(APPEND MINIC_COMPILER_LLVM_COMPONENTS nvptxcodegen nvptxdesc nvptxinfo)
endif()

add_library(minicCompiler ${MINIC_COMPILER_SOURCES})

target_include_directories(minicCompiler PUBLIC ${PROJECT_SOURCE_DIR}/include)

llvm_map_components_to_libnames(MINIC_COMPILER_LLVM_LIBS
  ${MINIC_COMPILER_LLVM_COMPONENTS})
target_link_libraries(minicCompiler PUBLIC ${MINIC_COMPILER_LLVM_LIBS})

if(MINIC_HAS_NVPTX)
  target_compile_definitions(minicCompiler PUBLIC MINIC_HAS_NVPTX)
endif()
llvm_map_components_to_libnames(MINIC_LLVM_LIBS support)

add_executable(minic main.cpp)
target_include_directories(minic PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(minic PRIVATE ${MINIC_LLVM_LIBS})
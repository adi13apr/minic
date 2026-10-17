add_executable(minic main.cpp)
target_link_libraries(minic PRIVATE minicCompiler)

# Tell the driver where the runtime library is, so it can link it into every
# compiled program. It can be overridden at run time with --runtime=<path>.
target_compile_definitions(minic PRIVATE
  MINIC_RUNTIME_PATH="$<TARGET_FILE:minicRuntime>")
add_dependencies(minic minicRuntime)
add_library(minicRuntime STATIC runtime.c)

# Compiled programs are position-independent executables by default, so the
# runtime linked into them must be position-independent too.
set_target_properties(minicRuntime PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Programs compiled by minic are linked without sanitizers, so the runtime
# must not depend on the sanitizer libraries, even in a MINIC_SANITIZE build.
target_compile_options(minicRuntime PRIVATE -fno-sanitize=all)
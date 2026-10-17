cmake_minimum_required(VERSION 3.20)
project(minic
  VERSION 0.1.0
  DESCRIPTION "A small C-like language compiled to LLVM IR"
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()


find_package(LLVM 18.1 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM built with RTTI: ${LLVM_ENABLE_RTTI}")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})



if(NOT LLVM_ENABLE_RTTI)
  add_compile_options(-fno-rtti)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)


option(MINIC_WERROR "Treat compiler warnings as errors" OFF)
option(MINIC_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(MINIC_WERROR)
  add_compile_options(-Werror)
endif()

if(MINIC_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

add_subdirectory(runtime)
add_subdirectory(lib)
add_subdirectory(tools/minic)
add_subdirectory(test)
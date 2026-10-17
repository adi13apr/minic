// RUN: %minic --help | FileCheck %s
// CHECK: USAGE: minic [options] <input file>
// CHECK: --dump-ast
// CHECK: --dump-tokens
// CHECK: --emit-llvm
// CHECK: --emit-ptx
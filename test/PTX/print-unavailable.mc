// REQUIRES: nvptx
// RUN: not %minic --emit-ptx %s 2>&1 | FileCheck %s

fn show(x: int) { print(x); }
// CHECK: print-unavailable.mc:[[@LINE-1]]:19: error: 'print' is not available when compiling for the GPU
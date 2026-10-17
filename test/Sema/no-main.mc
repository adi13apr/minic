// RUN: not %minic %s 2>&1 | FileCheck %s
// CHECK: error: program has no 'main' function

fn helper() -> int { return 0; }
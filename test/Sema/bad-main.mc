// RUN: not %minic %s 2>&1 | FileCheck %s

fn main(argc: int) -> int { return argc; }
// CHECK: bad-main.mc:[[@LINE-1]]:4: error: 'main' must be declared as 'fn main() -> int'
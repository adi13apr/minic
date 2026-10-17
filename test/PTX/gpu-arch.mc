// REQUIRES: nvptx
// RUN: %minic --emit-ptx --gpu-arch=sm_80 %s | FileCheck %s
// RUN: not %minic --emit-ptx --gpu-arch=sm_999 %s 2>&1 | FileCheck %s --check-prefix=BAD

fn one() -> int { return 1; }

// CHECK: .target sm_80
// BAD: error: unknown GPU architecture 'sm_999'
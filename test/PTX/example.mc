// REQUIRES: nvptx
// RUN: %minic --emit-ptx -O2 %examples/gpu.mc | FileCheck %s

// Keeps examples/gpu.mc compiling to PTX.
// CHECK: .func {{.*}}max(
// CHECK: .func {{.*}}gcd(
// CHECK: .func {{.*}}collatzSteps(
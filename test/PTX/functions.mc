// REQUIRES: nvptx
// RUN: %minic --emit-ptx -O2 %s | FileCheck %s
// RUN: %minic --emit-ptx --emit-llvm %s | FileCheck %s --check-prefix=IR

// GPU code has no main: every function becomes a device function (.func).

fn add(a: int, b: int) -> int { return a + b; }

fn max(a: int, b: int) -> int {
  if (a > b) {
    return a;
  }
  return b;
}

fn divide(a: int, b: int) -> int { return a / b; }

// CHECK:       .target sm_70
// CHECK:       .address_size 64

// CHECK-LABEL: .visible .func (.param .b32 func_retval0) add(
// CHECK:         add.s32
// CHECK:         ret;

// The optimizer recognizes the if/return pattern as a maximum, and PTX has
// an instruction for it.
// CHECK-LABEL: .visible .func (.param .b32 func_retval0) max(
// CHECK:         max.s32
// CHECK:         ret;

// Division by zero still traps, now with PTX's trap instruction.
// CHECK-LABEL: .visible .func (.param .b32 func_retval0) divide(
// CHECK-DAG:     trap;
// CHECK-DAG:     div.s32

// IR: target datalayout = "e-{{.*}}"
// IR: target triple = "nvptx64-nvidia-cuda"
// IR: define i32 @add(i32 %a, i32 %b) {
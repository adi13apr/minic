// RUN: not %minic %t.does-not-exist 2>&1 | FileCheck %s
// CHECK: minic: error: could not open '{{.*}}does-not-exist'
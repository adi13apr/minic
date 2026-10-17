// Device functions to compile for an NVIDIA GPU:
//   minic --emit-ptx -O2 examples/gpu.mc
// GPU code has no main and cannot use print.

fn max(a: int, b: int) -> int {
  if (a > b) {
    return a;
  }
  return b;
}

fn gcd(a: int, b: int) -> int {
  while (b != 0) {
    let t: int = b;
    b = a - a / b * b;
    a = t;
  }
  return a;
}

fn isEven(n: int) -> bool { return n - n / 2 * 2 == 0; }

fn collatzSteps(start: int) -> int {
  let n: int = start;
  let steps: int = 0;
  while (n != 1) {
    if (isEven(n)) {
      n = n / 2;
    } else {
      n = 3 * n + 1;
    }
    steps = steps + 1;
  }
  return steps;
}
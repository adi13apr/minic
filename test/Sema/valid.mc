// RUN: %minic %s -o %t
// Every feature used correctly: Sema must accept this program.

fn main() -> int {
  let x: int = add(1, 2);
  let ok: bool = isEven(x) || x > 10 && !false;
  if (ok) {
    show(x);
  } else if (x == 3) {
    x = -x;
  } else {
    return 1;
  }
  while (x > 0) {
    x = x - 1;
  }
  {
    let x: bool = true;
    if (x == true) {
      show(factorial(5));
    }
  }
  let smallest: int = -2147483648;
  let largest: int = 2147483647;
  return x;
}

fn add(a: int, b: int) -> int { return a + b; }

fn isEven(n: int) -> bool { return n / 2 * 2 == n; }

fn show(value: int) { print(value); }

fn factorial(n: int) -> int {
  if (n <= 1) {
    return 1;
  }
  return n * factorial(n - 1);
}
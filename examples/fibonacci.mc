// Prints the first 10 Fibonacci numbers, computed iteratively.

fn main() -> int {
  let a: int = 0;
  let b: int = 1;
  let i: int = 0;
  while (i < 10) {
    print(a);
    let next: int = a + b;
    a = b;
    b = next;
    i = i + 1;
  }
  return 0;
}
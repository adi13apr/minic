// Counts the steps 27 takes to reach 1 in the Collatz sequence.

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

fn main() -> int {
  print(collatzSteps(27));
  return 0;
}
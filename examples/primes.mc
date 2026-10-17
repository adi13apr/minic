// Prints every prime below 30, using trial division.

fn isPrime(n: int) -> bool {
  if (n < 2) {
    return false;
  }
  let d: int = 2;
  while (d * d <= n) {
    // n % d == 0, written without a modulo operator.
    if (n - n / d * d == 0) {
      return false;
    }
    d = d + 1;
  }
  return true;
}

fn main() -> int {
  let n: int = 2;
  while (n < 30) {
    if (isPrime(n)) {
      print(n);
    }
    n = n + 1;
  }
  return 0;
}
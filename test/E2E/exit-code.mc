// RUN: %minic %s -o %t
// RUN: not %t

// main's return value becomes the process exit code, so this run "fails".
fn main() -> int { return 42; }
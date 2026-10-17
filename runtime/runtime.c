/*===- runtime.c - minic runtime library -----------------------------------===*
 *
 * Functions that compiled minic programs call. The minic driver links this
 * library into every program it compiles.
 *
 *===----------------------------------------------------------------------===*/

#include <stdio.h>

/* Implements the builtin print(int). */
void minic_print(int Value) {
  printf("%d\n", Value);
  /* Flush right away: if the program later traps (e.g. on division by zero),
   * output still sitting in the stdio buffer would be lost. */
  fflush(stdout);
}

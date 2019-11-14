#include <stdio.h>

extern int log_entry_validator_tests (void);

int main (void)
{
  int failed = 0;
  if (log_entry_validator_tests() != 0) { ++failed; }
  printf ("\n[SUITE ERR ] %d suite(s)\n", failed);
  return failed;
}

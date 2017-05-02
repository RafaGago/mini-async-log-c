#include <stdio.h>

extern int tls_buffer_tests (void);
extern int interface_tests (void);
extern int serialization_tests (void);

int main (void)
{
  int failed = 0;
  if (tls_buffer_tests() != 0)    { ++failed; }
  if (interface_tests() != 0)     { ++failed; }
  if (serialization_tests() != 0) { ++failed; }
  printf ("\n[SUITE ERR ] %d suite(s)\n", failed);
  return failed;
}
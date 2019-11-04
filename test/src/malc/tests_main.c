#include <stdio.h>
#include <bl/time_extras/time_extras.h>

extern int tls_buffer_tests (void);
extern int bounded_buffer_tests (void);
extern int serialization_tests (void);
extern int entry_parser_tests (void);
extern int array_dst_tests (void);
extern int file_dst_tests (void);
extern int destinations_tests (void);

int main (void)
{
  bl_time_extras_init();
  int failed = 0;
  if (tls_buffer_tests() != 0)     { ++failed; }
  if (bounded_buffer_tests() != 0) { ++failed; }
  if (serialization_tests() != 0)  { ++failed; }
  if (entry_parser_tests() != 0)   { ++failed; }
  if (array_dst_tests() != 0)      { ++failed; }
  if (file_dst_tests() != 0)       { ++failed; }
  if (destinations_tests() != 0)   { ++failed; }

  printf ("\n[SUITE ERR ] %d suite(s)\n", failed);
  bl_time_extras_destroy();
  return failed;
}
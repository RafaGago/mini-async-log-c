#include <malcpp/compile-time/validation_test_boilerplate.hpp>

void tests()
{
  fmttest ("");
  fmttest ("{}", 1);
  fmttest ("{{}       {{{}", 1);
  fmttest ("{}       {}  ", 2, 3);
}

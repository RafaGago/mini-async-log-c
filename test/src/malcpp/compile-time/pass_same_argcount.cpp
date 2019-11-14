#include <malcpp/compile-time/validation_test_boilerplate.hpp>

void tests()
{
  valtest ("");
  valtest ("{}", 1);
  valtest ("{{}       {{{}", 1);
  valtest ("{}       {}  ", 2, 3);
}

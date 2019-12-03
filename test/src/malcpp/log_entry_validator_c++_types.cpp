#include <memory>
#include <vector>

#include <malc/malc.hpp>
/* cmocka is so braindead to define a fail() macro!!!, which clashes with e.g.
ostream's fail(), we include this header the last and hope it never breaks.*/
#include <bl/cmocka_pre.h>

using namespace malcpp::detail::fmt;
/*----------------------------------------------------------------------------*/
template <int N, class... types>
int fmt (const char(&arr)[N], types... args)
{
  return fmtret::get_code(
    format_string::validate<::malcpp::detail::typelist<types...> > (arr)
    );
}
/*----------------------------------------------------------------------------*/
template <class... types>
int refs (types... args)
{
  return fmtret::get_code(
    refvalues::validate<::malcpp::detail::typelist<types...> >()
    );
}
/*----------------------------------------------------------------------------*/
static void vector_int_modifiers (void **state)
{
  // The implementation is reusing the int validation, so this is just added
  // without testing features in dept.
  using namespace std;
  int exp = fmterr_success;
  assert_int_equal (exp, fmt (""));
  assert_int_equal(
    exp,
    fmt(
      "{0Nx}",
      make_shared<vector<int> >(
        initializer_list<int>{ 1, 2, 3, 4, 5, 6, 7}
        )
      )
    );
}
/*----------------------------------------------------------------------------*/
static void vector_float_modifiers (void **state)
{
  // The implementation is reusing the int validation, so this is just added
  // without testing features in dept.
  using namespace std;
  int exp = fmterr_success;
  assert_int_equal (exp, fmt (""));
  assert_int_equal(
    exp,
    fmt(
      "{a}",
      make_shared<vector<float> >(
        initializer_list<float>{ 1.f, 2.f }
        )
      )
    );
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test (vector_int_modifiers),
  cmocka_unit_test (vector_float_modifiers),
};
/*----------------------------------------------------------------------------*/
int log_entry_validator_cpp_types_tests (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/

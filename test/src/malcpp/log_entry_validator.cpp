
#include <bl/cmocka_pre.h>

#include <malc/impl/cpp11/cpp11.hpp>

using namespace malcpp::detail::fmt;
/*----------------------------------------------------------------------------*/
template <int N, class... types>
int fmt (const char(&arr)[N], types... args)
{
  return format_string::validate<typelist<types...> > (arr);
}
/*----------------------------------------------------------------------------*/
static void matching_placeholders (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt (""));
  assert_int_equal (exp, fmt ("{}", 1));
  assert_int_equal (exp, fmt ("  {}", 1));
  assert_int_equal (exp, fmt ("{}{}", 1, 2));
  assert_int_equal (exp, fmt ("{}   {}", 1, 2));
  assert_int_equal (exp, fmt ("{}{}   {}", 1, 2, 3));
  assert_int_equal (exp, fmt ("  {}   {}  ", 1, 2));
}
/*----------------------------------------------------------------------------*/
static void excess_arguments (void **state)
{
  int exp = fmterr_excess_arguments;
  assert_int_equal (exp, fmt ("", 1));
  assert_int_equal (exp, fmt ("{}", 1, 2));
  assert_int_equal (exp, fmt ("  {}", 1, 2));
  assert_int_equal (exp, fmt ("{}{}", 1, 2, 3));
  assert_int_equal (exp, fmt ("{}   {}", 1, 2, 3));
  assert_int_equal (exp, fmt ("{}{}   {}", 1, 2, 3, 4));
  assert_int_equal (exp, fmt ("  {}   {}  ", 1, 2, 3));
}
/*----------------------------------------------------------------------------*/
static void excess_placeholders (void **state)
{
  int exp = fmterr_excess_placeholders;
  assert_int_equal (exp, fmt ("{}"));
  assert_int_equal (exp, fmt ("  {}"));
  assert_int_equal (exp, fmt ("{}{}", 1));
  assert_int_equal (exp, fmt ("{}   {}", 1));
  assert_int_equal (exp, fmt ("{}{}   {}", 1, 2));
  assert_int_equal (exp, fmt ("  {}   {}  ", 1));
}
/*----------------------------------------------------------------------------*/
static void lbracket_escaping (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{{}"));
  assert_int_equal (exp, fmt ("  {{}"));
  assert_int_equal (exp, fmt ("{{}{}", 1));
  assert_int_equal (exp, fmt ("{}   {{}", 1));
  assert_int_equal (exp, fmt ("{}{}   {{}", 1, 2));
  assert_int_equal (exp, fmt ("  {{}   {}  ", 1));
  assert_int_equal (exp, fmt ("  {{{{{{}   {}  ", 1));
}
/*----------------------------------------------------------------------------*/
static void unclosed_lbrackets (void **state)
{
  int exp = fmterr_unclosed_lbracket;
  assert_int_equal (exp, fmt ("{"));
  assert_int_equal (exp, fmt ("{   "));
  assert_int_equal (exp, fmt ("{}    { ", 1));
  assert_int_equal (exp, fmt ("{}    {", 1));
  assert_int_equal (exp, fmt ("{}{}{} {", 1, 2, 3));
}
/*----------------------------------------------------------------------------*/
static void ignoring_rbrackets (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("}"));
  assert_int_equal (exp, fmt ("}   "));
  assert_int_equal (exp, fmt ("{}    }", 1));
  assert_int_equal (exp, fmt ("}{}    }", 1));
  assert_int_equal (exp, fmt ("{}}}{}}{}}} }", 1, 2, 3));
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test (matching_placeholders),
  cmocka_unit_test (excess_arguments),
  cmocka_unit_test (excess_placeholders),
  cmocka_unit_test (lbracket_escaping),
  cmocka_unit_test (unclosed_lbrackets),
  cmocka_unit_test (ignoring_rbrackets),
};
/*----------------------------------------------------------------------------*/
int log_entry_validator_tests (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/

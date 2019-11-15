
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
static void int_modifiers_stage1 (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{#}", 1));
  assert_int_equal (exp, fmt ("{ }", 1));
  assert_int_equal (exp, fmt ("{+}", 1));
  assert_int_equal (exp, fmt ("{-}", 1));
  assert_int_equal (exp, fmt ("{0}", 1));
  assert_int_equal (exp, fmt ("{# }", 1));
  assert_int_equal (exp, fmt ("{#+}", 1));
  assert_int_equal (exp, fmt ("{#-}", 1));
  assert_int_equal (exp, fmt ("{#0-}", 1));
  assert_int_equal (exp, fmt ("{#0+}", 1));
  assert_int_equal (exp, fmt ("{#0+-}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_stage1_no_repetitions (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{##}", 1));
  assert_int_equal (exp, fmt ("{  }", 1));
  assert_int_equal (exp, fmt ("{+-+}", 1));
  assert_int_equal (exp, fmt ("{-0#-}", 1));
  assert_int_equal (exp, fmt ("{#00}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_stage2 (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{1}", 1));
  assert_int_equal (exp, fmt ("{20}", 1));
  assert_int_equal (exp, fmt ("{11}", 1));
  assert_int_equal (exp, fmt ("{22}", 1));
  assert_int_equal (exp, fmt ("{33}", 1));
  assert_int_equal (exp, fmt ("{44}", 1));
  assert_int_equal (exp, fmt ("{55}", 1));
  assert_int_equal (exp, fmt ("{66}", 1));
  assert_int_equal (exp, fmt ("{77}", 1));
  assert_int_equal (exp, fmt ("{88}", 1));
  assert_int_equal (exp, fmt ("{99}", 1));
  assert_int_equal (exp, fmt ("{W}", 1));
  assert_int_equal (exp, fmt ("{N}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_stage2_no_mixing_types (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{1W}", 1));
  assert_int_equal (exp, fmt ("{1N}", 1));
  assert_int_equal (exp, fmt ("{N1}", 1));
  assert_int_equal (exp, fmt ("{W1}", 1));
  assert_int_equal (exp, fmt ("{NW1}", 1));
  assert_int_equal (exp, fmt ("{1NW}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_stage3 (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{x}", 1));
  assert_int_equal (exp, fmt ("{X}", 1));
  assert_int_equal (exp, fmt ("{o}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_stage3_only_one (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{xx}", 1));
  assert_int_equal (exp, fmt ("{XX}", 1));
  assert_int_equal (exp, fmt ("{oo}", 1));
  assert_int_equal (exp, fmt ("{ox}", 1));
  assert_int_equal (exp, fmt ("{oxX}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_stages_combined (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{0Nx}", 1));
  assert_int_equal (exp, fmt ("{+12o}", 1));
  assert_int_equal (exp, fmt ("{-11X}", 1));
  assert_int_equal (exp, fmt ("{#W}", 1));
  assert_int_equal (exp, fmt ("{NX}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_bad_stage_orderings (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{0xN}", 1));
  assert_int_equal (exp, fmt ("{12+o}", 1));
  assert_int_equal (exp, fmt ("{11X-}", 1));
  assert_int_equal (exp, fmt ("{W#}", 1));
  assert_int_equal (exp, fmt ("{XN}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_multiple_values (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{0Nx}{x} paco {+o}", 1, 2, 3));
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test (matching_placeholders),
  cmocka_unit_test (excess_arguments),
  cmocka_unit_test (excess_placeholders),
  cmocka_unit_test (lbracket_escaping),
  cmocka_unit_test (unclosed_lbrackets),
  cmocka_unit_test (ignoring_rbrackets),
  cmocka_unit_test (int_modifiers_stage1),
  cmocka_unit_test (int_modifiers_stage1_no_repetitions),
  cmocka_unit_test (int_modifiers_stage2),
  cmocka_unit_test (int_modifiers_stage2_no_mixing_types),
  cmocka_unit_test (int_modifiers_stage3),
  cmocka_unit_test (int_modifiers_stage3_only_one),
  cmocka_unit_test (int_modifiers_stages_combined),
  cmocka_unit_test (int_modifiers_bad_stage_orderings),
  cmocka_unit_test (int_modifiers_multiple_values),
};
/*----------------------------------------------------------------------------*/
int log_entry_validator_tests (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/

#include <malcpp/malcpp_lean.hpp>

#include <bl/cmocka_pre.h>
/* cmocka is so braindead to define a fail() macro!!!, which clashes with e.g.
ostream's fail(), we include this header the last and hope it never breaks.*/
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
static void void_pointer (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{}", (void*) nullptr));
}
/*----------------------------------------------------------------------------*/
static void void_pointer_no_modifiers (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{0}", (void*) nullptr));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_flags (void **state)
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
static void int_modifiers_flags_no_repetitions (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{##}", 1));
  assert_int_equal (exp, fmt ("{  }", 1));
  assert_int_equal (exp, fmt ("{+-+}", 1));
  assert_int_equal (exp, fmt ("{-0#-}", 1));
  assert_int_equal (exp, fmt ("{#00}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_width (void **state)
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
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_width_too_wide (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{0101}", 1));
  assert_int_equal (exp, fmt ("{101}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_precision (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{.1}", 1));
  assert_int_equal (exp, fmt ("{.20}", 1));
  assert_int_equal (exp, fmt ("{.11}", 1));
  assert_int_equal (exp, fmt ("{.22}", 1));
  assert_int_equal (exp, fmt ("{.33}", 1));
  assert_int_equal (exp, fmt ("{.44}", 1));
  assert_int_equal (exp, fmt ("{.55}", 1));
  assert_int_equal (exp, fmt ("{.66}", 1));
  assert_int_equal (exp, fmt ("{.77}", 1));
  assert_int_equal (exp, fmt ("{.88}", 1));
  assert_int_equal (exp, fmt ("{.99}", 1));
  assert_int_equal (exp, fmt ("{.w}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_precision_no_mixing_types (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{.1w}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_precision_too_wide (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{.1012}", 1));
  assert_int_equal (exp, fmt ("{.001}", 1));
  assert_int_equal (exp, fmt ("{.101}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_specifiers (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{x}", 1));
  assert_int_equal (exp, fmt ("{X}", 1));
  assert_int_equal (exp, fmt ("{o}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_specifiers_only_one (void **state)
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
  assert_int_equal (exp, fmt ("{.wx}", 1));
  assert_int_equal (exp, fmt ("{+12o}", 1));
  assert_int_equal (exp, fmt ("{-11X}", 1));
  assert_int_equal (exp, fmt ("{#.w}", 1));
  assert_int_equal (exp, fmt ("{.wX}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_bad_stage_orderings (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{0x.w}", 1));
  assert_int_equal (exp, fmt ("{12+o}", 1));
  assert_int_equal (exp, fmt ("{11X-}", 1));
  assert_int_equal (exp, fmt ("{.w#}", 1));
  assert_int_equal (exp, fmt ("{X.w}", 1));
}
/*----------------------------------------------------------------------------*/
static void int_modifiers_multiple_values (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{.wx}{x} paco {+o}", 1, 2, 3));
}
/*----------------------------------------------------------------------------*/
static void float_modifiers_flags (void **state)
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
static void float_modifiers_flags_no_repetitions (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{##}", 1.));
  assert_int_equal (exp, fmt ("{  }", 1.));
  assert_int_equal (exp, fmt ("{+-+}", 1.));
  assert_int_equal (exp, fmt ("{-0#-}", 1.));
  assert_int_equal (exp, fmt ("{#00}", 1.));
}
/*----------------------------------------------------------------------------*/
static void float_modifiers_width_precission (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{20}", 1.));
  assert_int_equal (exp, fmt ("{.1}", 1.));
  assert_int_equal (exp, fmt ("{.11}", 1.));
  assert_int_equal (exp, fmt ("{22.11}", 1.));
}
/*----------------------------------------------------------------------------*/
static void float_modifiers_width_precission_no_repeated_dots (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{..1}", 1.));
  assert_int_equal (exp, fmt ("{.11.}", 1.));
  assert_int_equal (exp, fmt ("{22.11.}", 1.));
}
/*----------------------------------------------------------------------------*/
static void float_modifiers_specifiers (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{f}", 1.));
  assert_int_equal (exp, fmt ("{F}", 1.));
  assert_int_equal (exp, fmt ("{e}", 1.));
  assert_int_equal (exp, fmt ("{E}", 1.));
  assert_int_equal (exp, fmt ("{g}", 1.));
  assert_int_equal (exp, fmt ("{G}", 1.));
  assert_int_equal (exp, fmt ("{a}", 1.));
  assert_int_equal (exp, fmt ("{A}", 1.));
}
/*----------------------------------------------------------------------------*/
static void float_modifiers_specifiers_only_one (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{ff}", 1.));
  assert_int_equal (exp, fmt ("{gg}", 1.));
  assert_int_equal (exp, fmt ("{EE}", 1.));
  assert_int_equal (exp, fmt ("{AA}", 1.));
}
/*----------------------------------------------------------------------------*/
static void float_modifiers_stages_combined (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{012.2G}", 1.));
  assert_int_equal (exp, fmt ("{12.12g}", 1.));
  assert_int_equal (exp, fmt ("{-11A}", 1.));
  assert_int_equal (exp, fmt ("{+}", 1.));
  assert_int_equal (exp, fmt ("{3.3}", 1.));
}
/*----------------------------------------------------------------------------*/
static void float_modifiers_bad_stage_orderings (void **state)
{
  int exp = fmterr_invalid_modifiers;
  assert_int_equal (exp, fmt ("{0G3}", 1.));
  assert_int_equal (exp, fmt ("{12+a}", 1.));
  assert_int_equal (exp, fmt ("{11a-}", 1.));
  assert_int_equal (exp, fmt ("{12.#}", 1.));
}
/*----------------------------------------------------------------------------*/
static void float_modifiers_multiple_values (void **state)
{
  int exp = fmterr_success;
  assert_int_equal (exp, fmt ("{0g}{a} paco {+f}", 1., 2., 3.));
}
/*----------------------------------------------------------------------------*/
static void strref_pass (void **state)
{
  using namespace malcpp;
  auto ref  = strref (nullptr, 0);
  auto dtor = refdtor ([] (void*, malc_ref const*, bl_uword){}, nullptr);
  assert_int_equal (refs (ref, dtor), 1);
  assert_int_equal (fmterr_success, fmt ("{}", ref, dtor));
}
/*----------------------------------------------------------------------------*/
static void memref_pass (void **state)
{
  using namespace malcpp;
  auto ref  = memref (nullptr, 0);
  auto dtor = refdtor ([] (void*, malc_ref const*, bl_uword){}, nullptr);
  assert_int_equal (refs (ref, dtor), 1);
  assert_int_equal (fmterr_success, fmt ("{}", ref, dtor));
}
/*----------------------------------------------------------------------------*/
static void reference_type_errors (void **state)
{
  using namespace malcpp;
  auto ref  = memref (nullptr, 0);
  auto dtor = refdtor ([] (void*, malc_ref const*, bl_uword){}, nullptr);

  assert_int_equal (fmterr_missing_refdtor, refs (ref));
  assert_int_equal (fmterr_excess_refdtor, refs (dtor));
  assert_int_equal (fmterr_misplaced_refdtor, refs (ref, dtor, ref));
  assert_int_equal (fmterr_repeated_refdtor, refs (ref, dtor, dtor));
  assert_int_equal (fmterr_repeated_refdtor, refs (dtor, ref, dtor));
  assert_int_equal (fmterr_repeated_refdtor, refs (dtor, dtor, ref));
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test (matching_placeholders),
  cmocka_unit_test (excess_arguments),
  cmocka_unit_test (excess_placeholders),
  cmocka_unit_test (lbracket_escaping),
  cmocka_unit_test (unclosed_lbrackets),
  cmocka_unit_test (ignoring_rbrackets),
  cmocka_unit_test (void_pointer),
  cmocka_unit_test (void_pointer_no_modifiers),
  cmocka_unit_test (int_modifiers_flags),
  cmocka_unit_test (int_modifiers_flags_no_repetitions),
  cmocka_unit_test (int_modifiers_width),
  cmocka_unit_test (int_modifiers_width_too_wide),
  cmocka_unit_test (int_modifiers_precision),
  cmocka_unit_test (int_modifiers_precision_no_mixing_types),
  cmocka_unit_test (int_modifiers_precision_too_wide),
  cmocka_unit_test (int_modifiers_specifiers),
  cmocka_unit_test (int_modifiers_specifiers_only_one),
  cmocka_unit_test (int_modifiers_stages_combined),
  cmocka_unit_test (int_modifiers_bad_stage_orderings),
  cmocka_unit_test (int_modifiers_multiple_values),
  cmocka_unit_test (float_modifiers_flags),
  cmocka_unit_test (float_modifiers_flags_no_repetitions),
  cmocka_unit_test (float_modifiers_width_precission),
  cmocka_unit_test (float_modifiers_width_precission_no_repeated_dots),
  cmocka_unit_test (float_modifiers_specifiers),
  cmocka_unit_test (float_modifiers_specifiers_only_one),
  cmocka_unit_test (float_modifiers_stages_combined),
  cmocka_unit_test (float_modifiers_bad_stage_orderings),
  cmocka_unit_test (float_modifiers_multiple_values),
  cmocka_unit_test (strref_pass),
  cmocka_unit_test (memref_pass),
  cmocka_unit_test (reference_type_errors),
};
/*----------------------------------------------------------------------------*/
int log_entry_validator_tests (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/

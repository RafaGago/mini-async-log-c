/*----------------------------------------------------------------------------*/
#define bl_pp_add_line(name) bl_pp_tokconcat(name, __LINE__)
/*----------------------------------------------------------------------------*/
#define SER_TEST_GET_ENTRY(var, sev, ...)\
  MALC_LOG_CREATE_CONST_ENTRY (sev, __VA_ARGS__); \
  var = &bl_pp_add_line(malc_const_entry_)
/*----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>

#include <bl/cmocka_pre.h>
#include <bl/base/autoarray.h>
#include <bl/base/integer.h>
#include <bl/base/default_allocator.h>
#include <bl/base/hex_string.h>

#include <malc/entry_parser.h>

/* TODO: add tests to verify that incorrect flags are ignored (tedious and low
   ROI knowing the implementation but it's a good practice) */
/*----------------------------------------------------------------------------*/
typedef struct eparser_context {
  bl_alloc_tbl     alloc;
  entry_parser     ep;
  log_entry        le;
  malc_log_strings strs;
}
eparser_context;
/*----------------------------------------------------------------------------*/
static int eparser_test_setup (void **state)
{
  static eparser_context c;
  c.alloc = bl_get_default_alloc();
  assert_int_equal (entry_parser_init (&c.ep, &c.alloc).own, bl_ok);
  memset (&c.le, 0, sizeof c.le);
  memset (&c.strs, 0, sizeof c.strs);
  *state = &c;
  return 0;
}
/*----------------------------------------------------------------------------*/
static int eparser_test_teardown (void **state)
{
  eparser_context* c = (eparser_context*) *state;
  entry_parser_destroy (&c->ep);
  return 0;
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_timestamp (void **state)
{
  eparser_context* c = (eparser_context*) *state;
  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_error, "NOTHING");
  assert_int_equal(
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own
    );
  assert_string_equal (c->strs.timestamp, "00000000000.000000000");
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_severities (void **state)
{
  eparser_context* c = (eparser_context*) *state;

  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_debug, "NOTHING");
  assert_int_equal(
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own
    );
  assert_string_equal (c->strs.sev, MALC_EP_DEBUG);

  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_trace, "NOTHING");
  assert_int_equal(
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own
    );
  assert_string_equal (c->strs.sev, MALC_EP_TRACE);

  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_note, "NOTHING");
  assert_int_equal(
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own
    );
  assert_string_equal (c->strs.sev, MALC_EP_NOTE);

  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_warning, "NOTHING");
  assert_int_equal(
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own
    );
  assert_string_equal (c->strs.sev, MALC_EP_WARN);

  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_error, "NOTHING");
  assert_int_equal(
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own
    );
  assert_string_equal (c->strs.sev, MALC_EP_ERROR);

  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_critical, "NOTHING");
  assert_int_equal(
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own
    );
  assert_string_equal (c->strs.sev, MALC_EP_CRIT);
}
/*----------------------------------------------------------------------------*/
#define parser_run_integer_arg(c, fmtstr, arg) \
  memset (&c->le, 0, sizeof c->le); \
  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_error, fmtstr, arg); \
  log_argument bl_pp_add_line (la); \
  switch (c->le.entry->info[1]) { \
  case malc_type_i8: \
    bl_pp_add_line(la).vu8 = (bl_u8) arg; break; \
  case malc_type_u8: \
    bl_pp_add_line(la).vi8 = (bl_i8) arg; break; \
  case malc_type_i16: \
    bl_pp_add_line(la).vi16 = (bl_i16) arg; break; \
  case malc_type_u16: \
    bl_pp_add_line(la).vu16 = (bl_u16) arg; break; \
  case malc_type_i32: \
    bl_pp_add_line(la).vi32 = (bl_i32) arg; break; \
  case malc_type_u32: \
    bl_pp_add_line(la).vu32 = (bl_u32) arg; break; \
  case malc_type_i64: \
    bl_pp_add_line(la).vi64 = (bl_i64) arg; break; \
  case malc_type_u64: \
    bl_pp_add_line(la).vu64 = (bl_u64) arg; break; \
  default: \
    assert_false("bug"); \
  } \
  c->le.args       = &bl_pp_add_line(la); \
  c->le.args_count = 1; \
  assert_int_equal( \
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own \
    );
/*----------------------------------------------------------------------------*/
#define parser_run_ptr_arg(c, fmtstr, arg) \
  memset (&c->le, 0, sizeof c->le); \
  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_error, fmtstr, arg); \
  log_argument bl_pp_add_line(la); \
  switch (c->le.entry->info[1]) { \
  case malc_type_ptr: \
    bl_pp_add_line(la).vptr = (void*) arg; break; \
  default: \
    assert_false("bug"); \
  } \
  c->le.args       = &bl_pp_add_line(la); \
  c->le.args_count = 1; \
  assert_int_equal( \
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own \
    );
/*----------------------------------------------------------------------------*/
#define parser_run_float_arg(c, fmtstr, arg) \
  memset (&c->le, 0, sizeof c->le); \
  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_error, fmtstr, arg); \
  log_argument bl_pp_add_line(la); \
  switch (c->le.entry->info[1]) { \
  case malc_type_float: \
    bl_pp_add_line(la).vfloat = (float) arg; break; \
  case malc_type_double: \
    bl_pp_add_line(la).vdouble = (double) arg; break; \
  default: \
    assert_false("bug"); \
  } \
  c->le.args       = &bl_pp_add_line(la); \
  c->le.args_count = 1; \
  assert_int_equal( \
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own \
    );
/*----------------------------------------------------------------------------*/
#define parser_run_aggregate_arg(c, fmt, arg) \
  memset (&c->le, 0, sizeof c->le); \
  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_error, fmt, arg); \
  log_argument bl_pp_add_line(la); \
  void* bl_pp_add_line (arg) = (void*) &arg;\
  switch (c->le.entry->info[1]) { \
  case malc_type_lit: \
    bl_pp_add_line(la).vlit = *((malc_lit*) bl_pp_add_line (arg)); break; \
  case malc_type_strcp: \
    bl_pp_add_line(la).vstrcp = *((malc_strcp*) bl_pp_add_line (arg)); break; \
  case malc_type_memcp: \
    bl_pp_add_line(la).vmemcp = *((malc_memcp*) bl_pp_add_line (arg)); break; \
  case malc_type_strref: \
    bl_pp_add_line(la).vstrref = *((malc_strref*) bl_pp_add_line (arg)); break; \
  case malc_type_memref: \
    bl_pp_add_line(la).vmemref = *((malc_memref*) bl_pp_add_line (arg)); break; \
  default: \
    assert_false("bug"); \
  } \
  c->le.args       = &bl_pp_add_line(la); \
  c->le.args_count = 1; \
  assert_int_equal( \
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own \
    );
/*----------------------------------------------------------------------------*/
static void entry_parser_test_uints (void **state)
{
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  parser_run_integer_arg (c, "PREFIX {} SUFFIX", (bl_u8) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRIu8 " SUFFIX", (bl_u8) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {} SUFFIX", (bl_u16) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRIu16 " SUFFIX", (bl_u16) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {} SUFFIX", (bl_u32) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRIu32 " SUFFIX", (bl_u32) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {} SUFFIX", (bl_u64) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRIu64 " SUFFIX", (bl_u64) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_ints (void **state)
{
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  parser_run_integer_arg (c, "PREFIX {} SUFFIX", (bl_i8) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRId8 " SUFFIX", (bl_i8) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {} SUFFIX", (bl_i16) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRId16 " SUFFIX", (bl_i16) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {} SUFFIX", (bl_i32) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRId32 " SUFFIX", (bl_i32) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {} SUFFIX", (bl_i64) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRId64 " SUFFIX", (bl_i64) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_ints_with_modifs (void **state)
{
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  parser_run_integer_arg (c, "PREFIX { 03} SUFFIX", -1);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX % 03d SUFFIX", -1) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {+03} SUFFIX", -1);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %+03d SUFFIX", -1) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX { -3} SUFFIX", -1);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX % -3d SUFFIX", -1) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {03x} SUFFIX", (unsigned) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %03x SUFFIX", (unsigned) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {03X} SUFFIX", (unsigned) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %03X SUFFIX", (unsigned) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {03o} SUFFIX", (unsigned) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %03o SUFFIX", (unsigned) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_ints_with_malc_modifs (void **state)
{
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  parser_run_integer_arg (c, "PREFIX {0Nx} SUFFIX", (bl_u8) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %02" PRIx8 " SUFFIX", (bl_u16) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {0Nx} SUFFIX", (bl_u16) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %04" PRIx16 " SUFFIX", (bl_u16) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {0Nx} SUFFIX", (bl_u32) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %08" PRIx32 " SUFFIX", (bl_u32) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {0Nx} SUFFIX", (bl_u64) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %016" PRIx64 " SUFFIX", (bl_u64) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {0W} SUFFIX", (bl_u8) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %03" PRIu8 " SUFFIX", (bl_u16) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {0W} SUFFIX", (bl_u16) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %05" PRIu16 " SUFFIX", (bl_u16) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {0W} SUFFIX", (bl_u32) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %010" PRIu32 " SUFFIX", (bl_u32) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_integer_arg (c, "PREFIX {0W} SUFFIX", (bl_u64) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %020" PRIu64 " SUFFIX", (bl_u64) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_floats_modifs (void **state)
{
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  parser_run_float_arg (c, "PREFIX { .4} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX % .4f SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_float_arg (c, "PREFIX { 12.4} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX % 12.4f SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_float_arg (c, "PREFIX {+12.4} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %+12.4f SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_float_arg (c, "PREFIX {-12.4} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %-12.4f SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_float_arg (c, "PREFIX {e} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %e SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_float_arg (c, "PREFIX {E} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %E SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_float_arg (c, "PREFIX {f} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %f SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_float_arg (c, "PREFIX {F} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %F SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_float_arg (c, "PREFIX {g} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %g SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_float_arg (c, "PREFIX {G} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %G SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_float_arg (c, "PREFIX {a} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %a SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  parser_run_float_arg (c, "PREFIX {A} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %A SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_ptr (void **state)
{
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  parser_run_ptr_arg (c, "PREFIX {} SUFFIX", (void*) 0xdeadface);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %p SUFFIX", (void*) 0xdeadface) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_lit (void **state)
{
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  malc_lit v = loglit ("a literal");
  parser_run_aggregate_arg (c, "PREFIX {} SUFFIX", v);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %s SUFFIX", "a literal") > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_strcp (void **state)
{
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  malc_strcp v = logstrcpyl ("a literal");
  parser_run_aggregate_arg (c, "PREFIX {} SUFFIX", v);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %s SUFFIX", "a literal") > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_memcp (void **state)
{
  char cmp[512];
  bl_u8 const mem[] = { 0x00, 0x01, 0x03, 0x0a, 0xde };
  char expected[(sizeof mem * 2) + 1];

  eparser_context* c = (eparser_context*) *state;
  assert_int_equal(
    bl_bytes_to_hex_string (expected, sizeof expected, mem, sizeof mem),
    sizeof mem * 2
    );
  malc_memcp v = logmemcpy (mem, sizeof mem);
  parser_run_aggregate_arg (c, "PREFIX {} SUFFIX", v);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %s SUFFIX", expected) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_strref (void **state)
{
  char cmp[512];
  log_argument args;
  eparser_context* c = (eparser_context*) *state;

  memset (&c->le, 0, sizeof c->le);

  args.vstrref     = logstrrefl ("a literal");
  c->le.args       = &args;
  c->le.args_count = 1;
  c->le.refdtor    = logrefdtor (nullptr, nullptr);

  SER_TEST_GET_ENTRY(
    c->le.entry, malc_sev_error, "PREFIX {} SUFFIX", args.vstrref, c->le.refdtor
    );
  assert_int_equal(
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own
    );
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %s SUFFIX", "a literal") > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_memref (void **state)
{
  char cmp[512];
  log_argument args;
  bl_u8 mem[] = { 0x00, 0x01, 0x03, 0x0a, 0xde };
  char expected[(sizeof mem * 2) + 1];

  eparser_context* c = (eparser_context*) *state;
  assert_int_equal(
    bl_bytes_to_hex_string (expected, sizeof expected, mem, sizeof mem),
    sizeof mem * 2
    );

  args.vmemref     = logmemref (mem, sizeof mem);
  c->le.args       = &args;
  c->le.args_count = 1;
  c->le.refdtor    = logrefdtor (nullptr, nullptr);

  SER_TEST_GET_ENTRY(
    c->le.entry, malc_sev_error, "PREFIX {} SUFFIX", args.vmemref, c->le.refdtor
    );
  assert_int_equal(
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own
    );
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %s SUFFIX", expected) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_excess_args (void **state)
{
  eparser_context* c = (eparser_context*) *state;

  parser_run_integer_arg (c, "PREFIX ", -1);
  assert_string_equal ("PREFIX " MALC_EP_EXCESS_ARGS, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_missing_args (void **state)
{
  eparser_context* c = (eparser_context*) *state;

  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_debug, "PREFIX {} SUFFIX");
  assert_int_equal(
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own
    );
  assert_string_equal("PREFIX " MALC_EP_MISSING_ARG " SUFFIX", c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_escaped_braces (void **state)
{
  eparser_context* c = (eparser_context*) *state;

  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_debug, "PREFIX {{} SUFFIX");
  assert_int_equal(
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own
    );
  assert_string_equal ("PREFIX {} SUFFIX", c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_unclosed_fmt (void **state)
{
  eparser_context* c = (eparser_context*) *state;

  SER_TEST_GET_ENTRY (c->le.entry, malc_sev_debug, "PREFIX { SUFFIX");
  assert_int_equal(
    bl_ok, entry_parser_get_log_strings (&c->ep, &c->le, &c->strs).own
    );
  assert_string_equal ("PREFIX " MALC_EP_UNCLOSED_FMT " SUFFIX", c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_misplaced_open_braces (void **state)
{
  eparser_context* c = (eparser_context*) *state;

  malc_lit v = loglit ("");
  parser_run_aggregate_arg (c, "PREFIX {s{} SUFFIX", v);
  assert_string_equal(
    "PREFIX " MALC_EP_MISPLACED_OPEN_BRACES " SUFFIX", c->strs.text
    );
}
/*----------------------------------------------------------------------------*/
static void entry_parser_esc_braces_in_fmt (void **state)
{
  eparser_context* c = (eparser_context*) *state;

  malc_lit v = loglit ("");
  parser_run_aggregate_arg (c, "PREFIX {s{{} SUFFIX", v);
  assert_string_equal(
    "PREFIX " MALC_EP_ESC_BRACES_IN_FMT " SUFFIX", c->strs.text
    );
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test_setup_teardown(
    entry_parser_test_timestamp, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_test_severities, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_test_uints, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_test_ints, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_test_ints_with_modifs,
    eparser_test_setup,
    eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_test_ints_with_malc_modifs,
    eparser_test_setup,
    eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_test_floats_modifs, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_test_ptr, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_test_lit, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_test_strcp, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_test_memcp, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_test_strref, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_test_memref, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_excess_args, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_missing_args, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_escaped_braces, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_unclosed_fmt, eparser_test_setup, eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_misplaced_open_braces,
    eparser_test_setup,
    eparser_test_teardown
    ),
  cmocka_unit_test_setup_teardown(
    entry_parser_esc_braces_in_fmt, eparser_test_setup, eparser_test_teardown
    ),
};
/*----------------------------------------------------------------------------*/
int entry_parser_tests (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/

#define MALC_GET_MIN_SEVERITY_FNAME malc_entry_get_min_severity_test
#define MALC_LOG_FNAME              malc_entry_parser_test
#define MALC_NO_BUILTIN_COMPRESSION
#define MALC_NO_PTR_COMPRESSION

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>

#include <bl/cmocka_pre.h>
#include <bl/base/autoarray.h>
#include <bl/base/integer.h>
#include <bl/base/default_allocator.h>
#include <bl/base/hex_string.h>

#include <malc/malc.h>
#include <malc/alltypes.h>
#include <malc/entry_parser.h>
#include <malc/stack_args.h>

/* TODO: add tests to verify that incorrect flags are ignored (tedious and low
   ROI knowing the implementation but it's a good practice) */
/*----------------------------------------------------------------------------*/
typedef struct malc {
  alloc_tbl    alloc;
  bl_err       ep_err;
  tstamp       timestamp;
  entry_parser ep;
  log_strings  strs;
}
eparser_context;
/*----------------------------------------------------------------------------*/
define_autoarray_types (log_args, log_argument);
declare_autoarray_funcs (log_args, log_argument);
/*----------------------------------------------------------------------------*/
MALC_EXPORT uword malc_entry_get_min_severity_test (malc const* l)
{
  return malc_sev_debug;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_entry_parser_test(
  struct malc* l, malc_const_entry const* e, uword size_dummy, ...
  )
{
  log_args args;
  assert_int_equal (log_args_init (&args, 64, &l->alloc), bl_ok);

  va_list vargs;
  va_start (vargs, size_dummy);
  char const* partype = &e->info[1];

  while (*partype) {
    bool no_insert = false;
    log_argument arg;
    switch (*partype) {
    case malc_type_float:
      arg.vfloat = malc_get_va_arg (vargs, arg.vfloat);
      break;
    case malc_type_double:
      arg.vdouble = malc_get_va_arg (vargs, arg.vdouble);
      break;
    case malc_type_i8:
      arg.vi8 = malc_get_va_arg (vargs, arg.vi8);
      break;
    case malc_type_u8:
      arg.vu8 = malc_get_va_arg (vargs, arg.vu8);
      break;
    case malc_type_i16:
      arg.vi16 = malc_get_va_arg (vargs, arg.vi16);
      break;
    case malc_type_u16:
      arg.vu16 = malc_get_va_arg (vargs, arg.vu16);
      break;
    case malc_type_i32:
      arg.vi32 = malc_get_va_arg (vargs, arg.vi32);
      break;
    case malc_type_u32:
      arg.vu32 = malc_get_va_arg (vargs, arg.vu32);
      break;
    case malc_type_i64:
      arg.vi64 = malc_get_va_arg (vargs, arg.vi64);
      break;
    case malc_type_u64:
      arg.vu64 = malc_get_va_arg (vargs, arg.vu64);
      break;
    case malc_type_ptr:
      arg.vptr = malc_get_va_arg (vargs, arg.vptr);
      break;
    case malc_type_lit:
      arg.vlit = malc_get_va_arg (vargs, arg.vlit);
      break;
    case malc_type_strref:
      arg.vstrref = malc_get_va_arg (vargs, arg.vstrref);
      break;
    case malc_type_memref:
      arg.vmemref = malc_get_va_arg (vargs, arg.vmemref);
      break;
    case malc_type_refdtor:
      no_insert = true;
      break;
    case malc_type_strcp:
      arg.vstrcp = malc_get_va_arg (vargs, arg.vstrcp);
      break;
    case malc_type_memcp:
      arg.vmemcp = malc_get_va_arg (vargs, arg.vmemcp);
      break;
    default:
      assert_true (false);
      break;
    }
    ++partype;
    if (no_insert) {
      continue;
    }
    bl_err err = log_args_insert_tail (&args, &arg, &l->alloc);
    assert_int_equal (err, bl_ok);
  }
  va_end (vargs);

  log_entry le;
  le.entry      = e;
  le.timestamp  = l->timestamp;
  le.args       = log_args_beg (&args);
  le.args_count = log_args_size (&args);
  le.refs       = nullptr;
  le.refs_count = 0;
  le.refdtor.func    = nullptr;
  le.refdtor.context = nullptr;
  l->ep_err = entry_parser_get_log_strings (&l->ep, &le, &l->strs);

  log_args_destroy (&args, &l->alloc);
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
static int eparser_test_setup (void **state)
{
  static eparser_context c;
  c.timestamp = 0;
  c.alloc     = get_default_alloc();
  assert_int_equal (entry_parser_init (&c.ep, &c.alloc), bl_ok);
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
  bl_err err;
  log_error_i (err, c, "NOTHING");
  assert_string_equal (c->strs.tstamp, "00000000000.000000000");
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_severities (void **state)
{
  eparser_context* c = (eparser_context*) *state;
  bl_err err;

  log_debug_i (err, c, "NOTHING");
  assert_string_equal (c->strs.sev, MALC_EP_DEBUG);

  log_trace_i (err, c, "NOTHING");
  assert_string_equal (c->strs.sev, MALC_EP_TRACE);

  log_notice_i (err, c, "NOTHING");
  assert_string_equal (c->strs.sev, MALC_EP_NOTE);

  log_warning_i (err, c, "NOTHING");
  assert_string_equal (c->strs.sev, MALC_EP_WARN);

  log_error_i (err, c, "NOTHING");
  assert_string_equal (c->strs.sev, MALC_EP_ERROR);

  log_critical_i (err, c, "NOTHING");
  assert_string_equal (c->strs.sev, MALC_EP_CRIT);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_uints (void **state)
{
  bl_err err;
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  log_error_i (err, c, "PREFIX {} SUFFIX", (u8) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRIu8 " SUFFIX", (u8) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {} SUFFIX", (u16) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRIu16 " SUFFIX", (u16) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {} SUFFIX", (u32) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRIu32 " SUFFIX", (u32) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {} SUFFIX", (u64) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRIu64 " SUFFIX", (u64) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_ints (void **state)
{
  bl_err err;
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  log_error_i (err, c, "PREFIX {} SUFFIX", (i8) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRId8 " SUFFIX", (i8) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {} SUFFIX", (i16) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRId16 " SUFFIX", (i16) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {} SUFFIX", (i32) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRId32 " SUFFIX", (i32) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {} SUFFIX", (i64) -1ull);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %" PRId64 " SUFFIX", (i64) -1ull) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_ints_with_modifs (void **state)
{
  bl_err err;
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  log_error_i (err, c, "PREFIX { 03} SUFFIX", -1);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX % 03d SUFFIX", -1) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {+03} SUFFIX", -1);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %+03d SUFFIX", -1) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX { -3} SUFFIX", -1);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX % -3d SUFFIX", -1) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {03x} SUFFIX", (unsigned) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %03x SUFFIX", (unsigned) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {03X} SUFFIX", (unsigned) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %03X SUFFIX", (unsigned) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {03o} SUFFIX", (unsigned) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %03o SUFFIX", (unsigned) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_ints_with_malc_modifs (void **state)
{
  bl_err err;
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  log_error_i (err, c, "PREFIX {0Nx} SUFFIX", (u8) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %02" PRIx8 " SUFFIX", (u16) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {0Nx} SUFFIX", (u16) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %04" PRIx16 " SUFFIX", (u16) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {0Nx} SUFFIX", (u32) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %08" PRIx32 " SUFFIX", (u32) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {0Nx} SUFFIX", (u64) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %016" PRIx64 " SUFFIX", (u64) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {0W} SUFFIX", (u8) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %03" PRIu8 " SUFFIX", (u16) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {0W} SUFFIX", (u16) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %05" PRIu16 " SUFFIX", (u16) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {0W} SUFFIX", (u32) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %010" PRIu32 " SUFFIX", (u32) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {0W} SUFFIX", (u64) 15);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %020" PRIu64 " SUFFIX", (u64) 15) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_floats_modifs (void **state)
{
  bl_err err;
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  log_error_i (err, c, "PREFIX { .4} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX % .4f SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX { 12.4} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX % 12.4f SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {+12.4} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %+12.4f SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {-12.4} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %-12.4f SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {e} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %e SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {E} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %E SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {f} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %f SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {F} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %F SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {g} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %g SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {G} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %G SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {a} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %a SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);

  log_error_i (err, c, "PREFIX {A} SUFFIX", -1.);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %A SUFFIX", -1.) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_ptr (void **state)
{
  bl_err err;
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  log_error_i (err, c, "PREFIX {} SUFFIX", (void*) 0xdeadface);
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %p SUFFIX", (void*) 0xdeadface) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_lit (void **state)
{
  bl_err err;
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  log_error_i (err, c, "PREFIX {} SUFFIX", loglit ("a literal"));
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %s SUFFIX", "a literal") > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_strcp (void **state)
{
  bl_err err;
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  log_error_i (err, c, "PREFIX {} SUFFIX", logstrcpyl ("a literal"));
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %s SUFFIX", "a literal") > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_memcp (void **state)
{
  bl_err err;
  char cmp[512];
  u8 const mem[] = { 0x00, 0x01, 0x03, 0x0a, 0xde };
  char expected[(sizeof mem * 2) + 1];

  eparser_context* c = (eparser_context*) *state;
  assert_int_equal(
    bl_bytes_to_hex_string (expected, sizeof expected, mem, sizeof mem),
    sizeof mem * 2
    );

  log_error_i (err, c, "PREFIX {} SUFFIX", logmemcpy (mem, sizeof mem));
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %s SUFFIX", expected) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_strref (void **state)
{
  bl_err err;
  char cmp[512];
  eparser_context* c = (eparser_context*) *state;

  log_error_i(
    err,
    c,
    "PREFIX {} SUFFIX",
    logstrrefl ("a literal"),
    logrefdtor (nullptr, nullptr)
    );
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %s SUFFIX", "a literal") > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_test_memref (void **state)
{
  bl_err err;
  char cmp[512];
  u8 const mem[] = { 0x00, 0x01, 0x03, 0x0a, 0xde };
  char expected[(sizeof mem * 2) + 1];

  eparser_context* c = (eparser_context*) *state;
  assert_int_equal(
    bl_bytes_to_hex_string (expected, sizeof expected, mem, sizeof mem),
    sizeof mem * 2
    );

  log_error_i(
    err,
    c,
    "PREFIX {} SUFFIX",
    logmemref (mem, sizeof mem),
    logrefdtor (nullptr, nullptr)
    );
  assert_true(
    snprintf (cmp, sizeof cmp, "PREFIX %s SUFFIX", expected) > 0
    );
  assert_string_equal (cmp, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_excess_args (void **state)
{
  bl_err err;
  eparser_context* c = (eparser_context*) *state;

  log_error_i(err, c, "PREFIX ", 1);
  assert_string_equal ("PREFIX " MALC_EP_EXCESS_ARGS, c->strs.text);

  log_error_i(err, c, "PREFIX ", 1, 2, 3, 4);
  assert_string_equal ("PREFIX " MALC_EP_EXCESS_ARGS, c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_missing_args (void **state)
{
  bl_err err;
  eparser_context* c = (eparser_context*) *state;

  log_error_i(err, c, "PREFIX {} SUFFIX");
  assert_string_equal ("PREFIX " MALC_EP_MISSING_ARG " SUFFIX", c->strs.text);

  log_error_i(err, c, "PREFIX {} SUFFIX {} SUFFIX");
  assert_string_equal(
    "PREFIX " MALC_EP_MISSING_ARG " SUFFIX " MALC_EP_MISSING_ARG " SUFFIX",
    c->strs.text
    );
}
/*----------------------------------------------------------------------------*/
static void entry_parser_escaped_braces (void **state)
{
  bl_err err;
  eparser_context* c = (eparser_context*) *state;

  log_error_i(err, c, "PREFIX {{} SUFFIX");
  assert_string_equal ("PREFIX {} SUFFIX", c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_unclosed_fmt (void **state)
{
  bl_err err;
  eparser_context* c = (eparser_context*) *state;

  log_error_i(err, c, "PREFIX { SUFFIX");
  assert_string_equal ("PREFIX " MALC_EP_UNCLOSED_FMT " SUFFIX", c->strs.text);
}
/*----------------------------------------------------------------------------*/
static void entry_parser_misplaced_open_braces (void **state)
{
  bl_err err;
  eparser_context* c = (eparser_context*) *state;

  log_error_i (err, c, "PREFIX {s{} SUFFIX", loglit (""));
  assert_string_equal(
    "PREFIX " MALC_EP_MISPLACED_OPEN_BRACES " SUFFIX", c->strs.text
    );
}
/*----------------------------------------------------------------------------*/
static void entry_parser_esc_braces_in_fmt (void **state)
{
  bl_err err;
  eparser_context* c = (eparser_context*) *state;

  log_error_i (err, c, "PREFIX {s{{} SUFFIX", loglit (""));
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

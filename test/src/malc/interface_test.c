#define MALC_GET_MIN_SEVERITY_FNAME malc_get_min_severity_test
#define MALC_LOG_FNAME              malc_log_test

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <bl/cmocka_pre.h>

#include <bl/base/integer.h>

#include <malc/malc.h>
#include <malc/alltypes.h>
#include <malc/stack_args.h>

#define FMT_STRING "value: {}"

/*----------------------------------------------------------------------------*/
typedef struct malc {
  malc_const_entry const* entry;
  uword                   size;
  alltypes                types;
}
malc;
/*----------------------------------------------------------------------------*/
MALC_EXPORT uword malc_get_min_severity_test (malc const*l )
{
  return malc_sev_note;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_log_test(
  struct malc* l, malc_const_entry const* e, uword size, ...
  )
{
  bl_err err = bl_ok;
  l->entry   = e;
  l->size    = size;

  va_list vargs;
  va_start (vargs, size);
  char const* partype = &e->info[1];

  while (*partype) {
    switch (*partype) {
    case malc_type_float: {
      l->types.vfloat = malc_get_va_arg (vargs, l->types.vfloat);
      break;
      }
    case malc_type_double: {
      l->types.vdouble = malc_get_va_arg (vargs, l->types.vdouble);
      break;
      }
    case malc_type_i8: {
      l->types.vi8 = malc_get_va_arg (vargs, l->types.vi8);
      break;
      }
    case malc_type_u8: {
      l->types.vu8 = malc_get_va_arg (vargs, l->types.vu8);
      break;
      }
    case malc_type_i16: {
      l->types.vi16 = malc_get_va_arg (vargs, l->types.vi16);
      break;
      }
    case malc_type_u16: {
      l->types.vu16 = malc_get_va_arg (vargs, l->types.vu16);
      break;
      }
#ifdef MALC_NO_BUILTIN_COMPRESSION
    case malc_type_i32: {
      l->types.vi32 = malc_get_va_arg (vargs, l->types.vi32);
      break;
      }
    case malc_type_u32: {
      l->types.vu32 = malc_get_va_arg (vargs, l->types.vu32);
      break;
      }
    case malc_type_i64: {
      l->types.vi64 = malc_get_va_arg (vargs, l->types.vi64);
      break;
      }
    case malc_type_u64: {
      l->types.vu64 = malc_get_va_arg (vargs, l->types.vu64);
      break;
      }
#else
    case malc_type_i32: {
      malc_compressed_32 v;
      v = malc_get_va_arg (vargs, v);
      uword size = v.format_nibble & ((1 << 3) - 1);
      ++size;
      uword neg  = v.format_nibble >> 3;
      l->types.vi32 = 0;
      for (uword i = 0; i < size; ++i) {
        l->types.vi32 |= (v.v & (0xff << (i * 8)));
      }
      l->types.vi32 = (neg) ? ~l->types.vi32 : l->types.vi32;
      break;
      }
    case malc_type_u32: {
      malc_compressed_32 v;
      v = malc_get_va_arg (vargs, v);
      uword size = v.format_nibble & ((1 << 3) - 1);
      ++size;
      l->types.vu32 = 0;
      for (uword i = 0; i < size; ++i) {
        l->types.vu32 |= (v.v & (0xff << (i * 8)));
      }
      break;
      }
    case malc_type_i64: {
      malc_compressed_64 v;
      v = malc_get_va_arg (vargs, v);
      uword size = v.format_nibble & ((1 << 3) - 1);
      ++size;
      uword neg  = v.format_nibble >> 3;
      l->types.vi64 = 0;
      for (uword i = 0; i < size; ++i) {
        l->types.vi64 |= (v.v & (0xffULL << (i * 8)));
      }
      l->types.vi64 = (neg) ? ~l->types.vi64 : l->types.vi64;
      break;
      }
    case malc_type_u64: {
      malc_compressed_64 v;
      v = malc_get_va_arg (vargs, v);
      uword size = v.format_nibble & ((1 << 3) - 1);
      ++size;
      l->types.vu64 = 0;
      for (uword i = 0; i < size; ++i) {
        l->types.vu64 |= (v.v & (0xffULL << (i * 8)));
      }
      break;
      }
#endif
    case malc_type_ptr: {
      l->types.vptr = malc_get_va_arg (vargs, l->types.vptr);
      break;
      }
    case malc_type_lit: {
      l->types.vlit = malc_get_va_arg (vargs, l->types.vlit);
      break;
      }
    case malc_type_strcp: {
      l->types.vstrcp = malc_get_va_arg (vargs, l->types.vstrcp);
      break;
      }
    case malc_type_memcp: {
      l->types.vmemcp = malc_get_va_arg (vargs, l->types.vmemcp);
      break;
      }
    default: {
      err = bl_invalid;
      goto end_process_loop;
      }
    }
    ++partype;
  }
end_process_loop:
  va_end (vargs);
  return err;
}
/*----------------------------------------------------------------------------*/
static void interface_test_float (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  float v = 12345.12345f;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vfloat);
  assert_true (m.size == sizeof (v));
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static void interface_test_double (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  double v = 123451231234.12341231235;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vdouble);
  assert_true (m.size == sizeof (v));
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static void interface_test_i8 (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  i8 v = -92;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vi8);
  assert_true (m.size == sizeof (v));
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static void interface_test_i16 (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  i16 v = -92 * 256;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vi16);
  assert_true (m.size == sizeof (v));
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static void interface_test_i32 (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  i32 v = -92 * 256 * 256 * 256;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vi32);
  assert_true (m.size == 4);
  v = -92;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vi32);
#ifdef MALC_NO_BUILTIN_COMPRESSION
  assert_true (m.size == 4);
  assert_true (m.entry->compressed_count == 0);
#else
  assert_true (m.size == 1);
  assert_true (m.entry->compressed_count == 1);
#endif
}
/*----------------------------------------------------------------------------*/
static void interface_test_i64 (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  i64 v = -92 * ((u64) 1 << 58);
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vi64);
  assert_true (m.size == 8);
  v = -92;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vi64);
#ifdef MALC_NO_BUILTIN_COMPRESSION
  assert_true (m.size == 8);
  assert_true (m.entry->compressed_count == 0);
#else
  assert_true (m.size == 1);
  assert_true (m.entry->compressed_count == 1);
#endif
}
/*----------------------------------------------------------------------------*/
static void interface_test_u8 (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  u8 v = 92;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vu8);
  assert_true (m.size == sizeof (v));
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static void interface_test_u16 (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  u16 v = 92 * 256;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vu16);
  assert_true (m.size == sizeof (v));
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static void interface_test_u32 (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  u32 v = 92 * 256 * 256 * 256;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vu32);
  assert_true (m.size == 4);
  v = 92;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vu32);
#ifdef MALC_NO_BUILTIN_COMPRESSION
  assert_true (m.size == 4);
  assert_true (m.entry->compressed_count == 0);
#else
  assert_true (m.size == 1);
  assert_true (m.entry->compressed_count == 1);
#endif
}
/*----------------------------------------------------------------------------*/
static void interface_test_u64 (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  u64 v = 92 * ((u64) 1 << 58);;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vu64);
  assert_true (m.size == 8);
  v = 92;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vu64);
#ifdef MALC_NO_BUILTIN_COMPRESSION
  assert_true (m.size == 8);
  assert_true (m.entry->compressed_count == 0);
#else
  assert_true (m.size == 1);
  assert_true (m.entry->compressed_count == 1);
#endif
}
/*----------------------------------------------------------------------------*/
static void interface_test_ptr (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  void* v = (void*) 0xaa00aa00;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.vptr);
  assert_true (m.size == sizeof (v));
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static void interface_test_lit (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  malc_lit v = {(char const*) 0xaa00aa00 };
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v.lit == m.types.vlit.lit);
  assert_true (m.size == sizeof (v));
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static void interface_test_str (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  malc_strcp v = {(char const*) 0xaa00aa00, 12345 };
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v.str == m.types.vstrcp.str);
  assert_true (v.len == m.types.vstrcp.len);
  assert_true (m.size == sizeof (u16) + m.types.vstrcp.len);
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static void interface_test_bytes (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  malc_memcp v = {(u8 const*) 0xaa00aa00, 12345 };
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v.mem == m.types.vmemcp.mem);
  assert_true (v.size == m.types.vmemcp.size);
  assert_true (m.size == sizeof (u16) + m.types.vmemcp.size);
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static void interface_test_all (void **state)
{
  malc m;
  alltypes all;
  memset (&m, 0, sizeof m);
  memset (&all, 0, sizeof all);
  char expected_info_str[] = {
    malc_sev_error,
    malc_type_u8,
    malc_type_i8,
    malc_type_u16,
    malc_type_i16,
    malc_type_u32,
    malc_type_i32,
    malc_type_u64,
    malc_type_i64,
    malc_type_float,
    malc_type_double,
    malc_type_ptr,
    malc_type_strcp,
    malc_type_lit,
    malc_type_memcp,
    0
  };

  all.vu8         = 2;
  all.vi8         = 5;
  all.vu16        = 23422;
  all.vi16        = -22222;
  all.vu32        = 23459999;
  all.vi32        = -234243442;
  all.vu64        = 3222222222222222222;
  all.vi64        = -5666666666566666666;
  all.vfloat      = 195953.2342f;
  all.vdouble     = 1231231123123123.234234444;
  all.vptr        = (void*) 0xaa00aa00;
  all.vstrcp.str  = (char const*) 0x123123;
  all.vstrcp.len  = 12;
  all.vlit.lit    = (char const*) 0x16783123;
  all.vmemcp.mem  = (u8 const*) 0xaa55aa55;
  all.vmemcp.size = 2345;

  bl_err err;
  malc_error_i(
    err,
    &m,
    "{} {} {} {} {} {} {} {} {} {} {} {} {} {}",
    all.vu8,
    all.vi8,
    all.vu16,
    all.vi16,
    all.vu32,
    all.vi32,
    all.vu64,
    all.vi64,
    all.vfloat,
    all.vdouble,
    all.vptr,
    all.vstrcp,
    all.vlit,
    all.vmemcp,
    );

  assert_int_equal (err, bl_ok);
  assert_memory_equal (&all, &m.types, sizeof all);
  assert_string_equal (m.entry->info, expected_info_str);
}
/*----------------------------------------------------------------------------*/
/* this is to test the compiler's preprocessor (verify that it compiles) */
static void interface_test_casting (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  i16 v = -92 * 256;
  malc_error_i (err, &m, FMT_STRING, (u16) v);
  assert_int_equal (err, bl_ok);
  assert_true ((u16) v == m.types.vu16);
  assert_true (m.size == sizeof (v));
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static u8 a_function (i16 v) { return (u8) v; }
/*----------------------------------------------------------------------------*/
/* this is to test the compiler's preprocessor (verify that  it compiles) */
static void interface_test_func_call_with_casting (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  i16 v = -92 * 256;
  malc_error_i (err, &m, FMT_STRING, (u16) a_function (v));
  assert_int_equal (err, bl_ok);
  assert_true ((u16) a_function (v) == m.types.vu16);
  assert_true (m.size == sizeof (v));
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
/* this is to test the compiler's preprocessor (verify that  it compiles) */
static void interface_test_ternary (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  uword choice = (((uword) &err) > 4) & 1;
  malc_error_i (err, &m, FMT_STRING, (u16) (choice ? (u16) 1 : (u16) 2));
  assert_int_equal (err, bl_ok);
  assert_true ((u16) (choice ? 1 : 2) == m.types.vu16);
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
  cmocka_unit_test (interface_test_float),
  cmocka_unit_test (interface_test_double),
  cmocka_unit_test (interface_test_i8),
  cmocka_unit_test (interface_test_i16),
  cmocka_unit_test (interface_test_i32),
  cmocka_unit_test (interface_test_i64),
  cmocka_unit_test (interface_test_u8),
  cmocka_unit_test (interface_test_u16),
  cmocka_unit_test (interface_test_u32),
  cmocka_unit_test (interface_test_u64),
  cmocka_unit_test (interface_test_ptr),
  cmocka_unit_test (interface_test_lit),
  cmocka_unit_test (interface_test_str),
  cmocka_unit_test (interface_test_bytes),
  cmocka_unit_test (interface_test_all),
  cmocka_unit_test (interface_test_casting),
  cmocka_unit_test (interface_test_func_call_with_casting),
  cmocka_unit_test (interface_test_ternary),
};
/*----------------------------------------------------------------------------*/
int interface_tests (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/

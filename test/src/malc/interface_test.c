#define MALC_GET_MIN_SEVERITY_FNAME malc_get_min_severity_test
#define MALC_LOG_FNAME              malc_log_test

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <bl/cmocka_pre.h>

#include <bl/base/integer.h>

#include <malc/malc.h>
#include <malc/stack_args.h>

#define FMT_STRING "value: {}"
/*----------------------------------------------------------------------------*/
typedef struct alltypes {
  double   val_double;
  u8       val_u8;
  u32      val_u32;
  u16      val_u16;
  u64      val_u64;
  i8       val_i8;
  i32      val_i32;
  i16      val_i16;
  i64      val_i64;
  float    val_float;
  void*    val_void_ptr;
  malc_str val_malc_str;
  malc_lit val_malc_lit;
  malc_mem val_malc_mem;
}
alltypes;
/*----------------------------------------------------------------------------*/
typedef struct malc {
  malc_const_entry const* entry;
  uword                   va_min_size;
  uword                   va_max_size;
  int                     argc;
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
  struct malc*            l,
  malc_const_entry const* e,
  uword                   va_min_size,
  uword                   va_max_size,
  int                     argc,
  ...
  )
{
  bl_err err     = bl_ok;
  l->entry       = e;
  l->va_min_size = va_min_size;
  l->va_max_size = va_max_size;
  l->argc        = argc;

  va_list vargs;
  va_start (vargs, argc);
  char const* partype = &e->info[1];

  while (*partype) {
    switch (*partype) {
    case malc_type_float: {
      l->types.val_float = malc_get_va_arg (vargs, l->types.val_float);
      break;
      }
    case malc_type_double: {
      l->types.val_double = malc_get_va_arg (vargs, l->types.val_double);
      break;
      }
    case malc_type_i8: {
      l->types.val_i8 = malc_get_va_arg (vargs, l->types.val_i8);
      break;
      }
    case malc_type_u8: {
      l->types.val_u8 = malc_get_va_arg (vargs, l->types.val_u8);
      break;
      }
    case malc_type_i16: {
      l->types.val_i16 = malc_get_va_arg (vargs, l->types.val_i16);
      break;
      }
    case malc_type_u16: {
      l->types.val_u16 = malc_get_va_arg (vargs, l->types.val_u16);
      break;
      }
    case malc_type_i32: {
      l->types.val_i32 = malc_get_va_arg (vargs, l->types.val_i32);
      break;
      }
    case malc_type_u32: {
      l->types.val_u32 = malc_get_va_arg (vargs, l->types.val_u32);
      break;
      }
    case malc_type_i64: {
      l->types.val_i64 = malc_get_va_arg (vargs, l->types.val_i64);
      break;
      }
    case malc_type_u64: {
      l->types.val_u64 = malc_get_va_arg (vargs, l->types.val_u64);
      break;
      }
    case malc_type_vptr: {
      l->types.val_void_ptr = malc_get_va_arg (vargs, l->types.val_void_ptr);
      break;
      }
    case malc_type_lit: {
      l->types.val_malc_lit = malc_get_va_arg (vargs, l->types.val_malc_lit);
      break;
      }
    case malc_type_str: {
      l->types.val_malc_str = malc_get_va_arg (vargs, l->types.val_malc_str);
      break;
      }
    case malc_type_bytes: {
      l->types.val_malc_mem = malc_get_va_arg (vargs, l->types.val_malc_mem);
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
#if 0
/*----------------------------------------------------------------------------*/
static void interface_test_float (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  float v = 12345.12345f;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.val_float);
  assert_true (m.va_min_size == sizeof (v));
  assert_true (m.va_max_size == sizeof (v));
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
  assert_true (v == m.types.val_double);
  assert_true (m.va_min_size == sizeof (v));
  assert_true (m.va_max_size == sizeof (v));
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
  assert_true (v == m.types.val_i8);
  assert_true (m.va_min_size == sizeof (v));
  assert_true (m.va_max_size == sizeof (v));
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
  assert_true (v == m.types.val_i16);
  assert_true (m.va_min_size == sizeof (v));
  assert_true (m.va_max_size == sizeof (v));
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
  assert_true (v == m.types.val_i32);
  assert_true (m.va_min_size == 1);
  assert_true (m.va_max_size == sizeof (v));
  assert_true (m.entry->compressed_count == 1);
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
  assert_true (v == m.types.val_i64);
  assert_true (m.va_min_size == 1);
  assert_true (m.va_max_size == sizeof (v));
  assert_true (m.entry->compressed_count == 1);
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
  assert_true (v == m.types.val_u8);
  assert_true (m.va_min_size == sizeof (v));
  assert_true (m.va_max_size == sizeof (v));
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
  assert_true (v == m.types.val_u16);
  assert_true (m.va_min_size == sizeof (v));
  assert_true (m.va_max_size == sizeof (v));
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
  assert_true (v == m.types.val_u32);
  assert_true (m.va_min_size == 1);
  assert_true (m.va_max_size == sizeof (v));
  assert_true (m.entry->compressed_count == 1);
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
  assert_true (v == m.types.val_u64);
  assert_true (m.va_min_size == 1);
  assert_true (m.va_max_size == sizeof (v));
  assert_true (m.entry->compressed_count == 1);
}
#endif
/*----------------------------------------------------------------------------*/
static void interface_test_void_ptr (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  void* v = (void*) 0xaa00aa00;
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v == m.types.val_void_ptr);
  assert_true (m.va_min_size == sizeof (v));
  assert_true (m.va_max_size == sizeof (v));
  assert_true (m.entry->compressed_count == 0);
}
#if 0
/*----------------------------------------------------------------------------*/
static void interface_test_lit (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  malc_lit v = {(char const*) 0xaa00aa00 };
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v.lit == m.types.val_malc_lit.lit);
  assert_true (m.va_min_size == sizeof (v));
  assert_true (m.va_max_size == sizeof (v));
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static void interface_test_str (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  malc_str v = {(char const*) 0xaa00aa00, 12345 };
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v.str == m.types.val_malc_str.str);
  assert_true (v.len == m.types.val_malc_str.len);
  assert_true (m.va_min_size == sizeof (u16));
  assert_true (m.va_max_size == sizeof (u16) + utype_max (u16));
  assert_true (m.entry->compressed_count == 0);
}
/*----------------------------------------------------------------------------*/
static void interface_test_bytes (void **state)
{
  malc m;
  memset (&m, 0, sizeof m);
  bl_err err;
  malc_mem v = {(u8 const*) 0xaa00aa00, 12345 };
  malc_error_i (err, &m, FMT_STRING, v);
  assert_int_equal (err, bl_ok);
  assert_true (v.mem == m.types.val_malc_mem.mem);
  assert_true (v.size == m.types.val_malc_mem.size);
  assert_true (m.va_min_size == sizeof (u16));
  assert_true (m.va_max_size == sizeof (u16) + utype_max (u16));
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
    malc_type_vptr,
    malc_type_str,
    malc_type_lit,
    malc_type_bytes,
    0
  };

  all.val_u8            = 2;
  all.val_i8            = 5;
  all.val_u16           = 23422;
  all.val_i16           = -22222;
  all.val_u32           = 23459999;
  all.val_i32           = -234243442;
  all.val_u64           = 3222222222222222222;
  all.val_i64           = -5666666666566666666;
  all.val_float         = 195953.2342f;
  all.val_double        = 1231231123123123.234234444;
  all.val_void_ptr      = (void*) 0xaa00aa00;
  all.val_malc_str.str  = (char const*) 0x123123;
  all.val_malc_str.len  = 12;
  all.val_malc_lit.lit  = (char const*) 0x16783123;
  all.val_malc_mem.mem  = (u8 const*) 0xaa55aa55;
  all.val_malc_mem.size = 2345;

  bl_err err;
  malc_error_i(
    err,
    &m,
    "{} {} {} {} {} {} {} {} {} {} {} {} {} {}",
    all.val_u8,
    all.val_i8,
    all.val_u16,
    all.val_i16,
    all.val_u32,
    all.val_i32,
    all.val_u64,
    all.val_i64,
    all.val_float,
    all.val_double,
    all.val_void_ptr,
    all.val_malc_str,
    all.val_malc_lit,
    all.val_malc_mem,
    );

  assert_int_equal (err, bl_ok);
  assert_memory_equal (&all, &m.types, sizeof all);
  assert_string_equal (m.entry->info, expected_info_str);
}
#endif
/*----------------------------------------------------------------------------*/
static const struct CMUnitTest tests[] = {
#if 0
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
#endif
  cmocka_unit_test (interface_test_void_ptr),
#if 0
  cmocka_unit_test (interface_test_lit),
  cmocka_unit_test (interface_test_str),
  cmocka_unit_test (interface_test_bytes),
  cmocka_unit_test (interface_test_all),
#endif
};
/*----------------------------------------------------------------------------*/
int interface_tests (void)
{
  return cmocka_run_group_tests (tests, nullptr, nullptr);
}
/*----------------------------------------------------------------------------*/

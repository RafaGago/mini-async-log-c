#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>

#include <bl/base/platform.h>
#include <bl/base/integer.h>
#include <bl/base/integer_manipulation.h>
#include <bl/base/assert.h>
#include <bl/base/error.h>
#include <bl/base/preprocessor.h>

#include <malc/malc.h>

/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_log(
  struct malc*            l,
  const malc_const_entry* e,
  uword                   min_size,
  uword                   max_size,
  int                     argc,
  ...
  )
{
  printf ("fmt: %s\n",e->format);
  printf ("info: %s\n",e->info);
  printf ("compressed: %d\n", (int) e->compressed_count);
  printf ("min field size: %d\n", (int) min_size);
  printf ("max field size: %d\n", (int) max_size);

  bl_err err = bl_ok;
  va_list vargs;
  va_start (vargs, argc);

  char const* partype = &e->info[1];

  while (*partype) {
    switch (*partype) {
    case malc_type_float: {
      float v = (float) va_arg (vargs, double);
      printf ("float: %f\n", v);
      break;
      }
    case malc_type_double: {
      double v = (double) va_arg (vargs, double);
      printf ("double: %f\n", v);
      break;
      }
    case malc_type_i8: {
      i8 v = (i8) va_arg (vargs, int);
      printf ("i8: %"PRId8"\n", v);
      break;
      }
    case malc_type_u8: {
      u8 v = (u8) va_arg (vargs, int);
      printf ("u8: %"PRIu8"\n", v);
      break;
      }
    case malc_type_i16: {
      i16 v = (i16) va_arg (vargs, int);
      printf ("i16: %"PRId16"\n", v);
      break;
      }
    case malc_type_u16: {
      u16 v = (u16) va_arg (vargs, int);
      printf ("u16: %"PRIu16"\n", v);
      break;
      }
#ifdef BL_32
    case malc_type_i32: {
      i32 v = (i32) va_arg (vargs, i32);
      printf ("i32: %"PRId32"\n", v);
      break;
      }
    case malc_type_u32: {
      u32 v = (u32)  va_arg (vargs, u32);
      printf ("u32: %"PRIu32"\n", v);
      break;
      }
#else
      case malc_type_i32: {
      i32 v = (i32) va_arg (vargs, int);
      printf ("i32: %"PRId32"\n", v);
      break;
      }
    case malc_type_u32: {
      u32 v = (u32) va_arg (vargs, int);
      printf ("u32: %"PRIu32"\n", v);
      break;
      }
#endif
      case malc_type_i64: {
      i64 v = (i64) va_arg (vargs, i64);
      printf ("i64: %"PRId64"\n", v);
      break;
      }
    case malc_type_u64: {
      u64 v = (u64) va_arg (vargs, u64);
      printf ("u64: %"PRIu64"\n", v);
      break;
      }
    case malc_type_vptr: {
      void* v = va_arg (vargs, void*);
      printf ("vptr: 0x%08lx\n", (u64) v);
      break;
      }
    case malc_type_lit: {
      malc_lit v = va_arg (vargs, malc_lit);
      printf ("string literal: %s\n", v.lit);
      break;
      }
    case malc_type_str: {
      malc_str v = va_arg (vargs, malc_str);
      printf ("string: len: %d, %s\n", (int) v.len, v.str);
      break;
      }
    case malc_type_bytes: {
      malc_mem  v = va_arg (vargs, malc_mem);
      printf ("mem: len: %d, ptr: 0x%08lx\n", v.size, (u64) v.mem);
      break;
      }
    default: {
      printf ("invalid\n");
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
MALC_EXPORT uword malc_get_min_severity (struct malc const* l)
{
  return (char) malc_sev_warning;
}
/*----------------------------------------------------------------------------*/
int main (void)
{
  u8  v8   = 1;
  u16 v16  = 2;
  u32 v32  = 3;
  i32 vi32 = -3;
  i64 vi64 = -4;

  u8 mem[] = { 0, 1, 2, 3, 4, 5 };

  bl_err err;
  puts ("- omitted entry -----");
  malc_debug_i (err, nullptr, "the format string", v8, v16, v32, vi32);
  puts ("- 4 builtins -------");
  malc_error_i (err, nullptr, "format string 1", v8, v16, v32, vi32, vi64);
  puts ("- no params -------");
  malc_error_i (err, nullptr, "format string 2");
  puts ("- non-builtin -------");
  malc_error_i (
    err,
    nullptr,
    "format string 2",
    (void*) nullptr,
    loglit ("a literal"),
    logstr ("a string", sizeof "a string" - 1),
    logmem (mem, sizeof mem)
    );
  return 0;
}
/*----------------------------------------------------------------------------*/

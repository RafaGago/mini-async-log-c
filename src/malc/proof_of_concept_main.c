#include <stdio.h>
#include <stdarg.h>
#include <base_library/hdr/platform.h>
#include <base_library/hdr/integer.h>
#include <base_library/hdr/error.h>
#include <base_library/hdr/preprocessor.h>
/*----------------------------------------------------------------------------*/
static inline u8 malc_get_sev (void* dummy)
{
  return 0;
}
/*----------------------------------------------------------------------------*/
typedef enum malc_type_ids {
  type_float,
  type_double,
  type_i8,
  type_u8,
  type_i16,
  type_u16,
  type_i32,
  type_u32,
  type_i64,
  type_u64,
  type_vptr,
  type_error,
}
malc_type_ids;
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
#define malc_get_typeid(token)\
  _Generic ((token),\
    float:   type_float,\
    double:  type_double,\
    u8:      type_u8,\
    i8:      type_i8,\
    u16:     type_u16,\
    i16:     type_i16,\
    u32:     type_u32,\
    i32:     type_i32,\
    u64:     type_u64,\
    i64:     type_i64,\
    default: type_error\
    )
#define malc_prepend_typeid(token) malc_get_typeid (token), token
/*----------------------------------------------------------------------------*/
#else
  /*templates for Visual Studio*/
#endif
/*----------------------------------------------------------------------------*/
static inline bl_err malc_log(
  void* malc_ptr, uword sev, const char* format, int argc, ...
  )
{
  bl_err err = bl_ok;
  va_list vargs;
  va_start (vargs, argc);
  while (argc--) {
    int type = va_arg (vargs, int);
    switch (type) {
    case type_float: {
      printf ("float\n");
      float dummy = (float) va_arg (vargs, double);
      break;
      }
    case type_double: {
      printf ("double\n");
      double dummy = va_arg (vargs, double);
      break;
      }
    case type_i8: {
      printf ("i8\n");
      i8 dummy = (i8) va_arg (vargs, int);
      break;
      }
    case type_u8: {
      printf ("u8\n");
      u8 dummy = (u8) va_arg (vargs, int);
      break;
      }
    case type_i16: {
      printf ("i16\n");
      i16 dummy = (i16) va_arg (vargs, int);
      break;
      }
    case type_u16: {
      printf ("u16\n");
      u16 dummy = (u16) va_arg (vargs, int);
      break;
      }
#ifndef BL_32
    case type_i32: {
      printf ("i32\n");
      i32 dummy = va_arg (vargs, i32);
      break;
      }
    case type_u32: {
      printf ("u32\n");
      u32 dummy = va_arg (vargs, u32);
      break;
      }
#else
    case type_i32: {
      printf ("i32\n");
      i32 dummy = (i32) va_arg (vargs, int);
      break;
      }
    case type_u32: {
      printf ("u32\n");
      u32 dummy = (u32) va_arg (vargs, int);
      break;
      }
#endif
      case type_i64: {
      printf ("i64\n");
      i64 dummy = va_arg (vargs, i64);
      break;
      }

    case type_u64: {
      printf ("u64\n");
      u64 dummy = va_arg (vargs, u64);
      break;
      }
    case type_vptr: {
      printf ("vptr\n");
      void* dummy = va_arg (vargs, void*);
      break;
      }
    default: {
      printf ("invalid\n");
      err = bl_invalid;
      goto end_process_loop;
      }
    }
  }
end_process_loop:
  va_end (vargs);
  return err;
}
/*----------------------------------------------------------------------------*/
#define malc_log_private_impl(malc_ptr, sev, ...)\
  malc_log(\
    malc_ptr,\
    sev,\
    pp_vargs_first (__VA_ARGS__),\
    pp_vargs_count (pp_vargs_ignore_first (__VA_ARGS__))\
    pp_if (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))(\
      pp_comma()\
      pp_apply(\
        malc_prepend_typeid, pp_comma, pp_vargs_ignore_first (__VA_ARGS__)\
        )))

#define malc_log_private(malc_ptr, sev, ...)\
  ((sev >= malc_get_sev (malc_ptr)) ?\
      malc_log_private_impl (malc_ptr, sev, __VA_ARGS__) : bl_ok)
/*----------------------------------------------------------------------------*/
int main (void)
{
  u8  v8   = 1;
  u16 v16  = 2;
  u32 v32  = 3;
  i32 vi32 = 3;

  printf (" call 1, it shouldn't print anything\n");
  malc_log_private (nullptr, 0, "fmt string");
  printf (" call 2\n");
  malc_log_private (nullptr, 0, "fmt string", v8, v16, v32);
  printf (" call 3\n");
  malc_log_private (nullptr, 0, "fmt string", vi32, (u64) 4);

  return 0;
}
/*----------------------------------------------------------------------------*/

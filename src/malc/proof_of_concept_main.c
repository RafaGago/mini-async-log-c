#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <bl/base/platform.h>
#include <bl/base/integer.h>
#include <bl/base/integer_manipulation.h>
#include <bl/base/assert.h>
#include <bl/base/error.h>
#include <bl/base/preprocessor.h>
/*----------------------------------------------------------------------------*/
typedef enum malc_encodings {
  malc_end          = 0,
  malc_type_float   = 'a',
  malc_type_double  = 'b',
  malc_type_i8      = 'c',
  malc_type_u8      = 'd',
  malc_type_i16     = 'e',
  malc_type_u16     = 'f',
  malc_type_i32     = 'g',
  malc_type_u32     = 'h',
  malc_type_i64     = 'i',
  malc_type_u64     = 'j',
  malc_type_vptr    = 'k',
  malc_type_lit     = 'l',
  malc_type_str     = 'm',
  malc_type_bytes   = 'n',
  malc_type_error   = 'o',
  malc_sev_debug    = '3',
  malc_sev_trace    = '4',
  malc_sev_note     = '5',
  malc_sev_warning  = '6',
  malc_sev_error    = '7',
  malc_sev_critical = '8',
  malc_sev_off      = '9',
}
malc_type_ids;
/*----------------------------------------------------------------------------*/
typedef struct malc_lit {
  char const* lit;
}
malc_lit;

static inline malc_lit loglit (char const* literal)
{
  bl_assert (literal);
  malc_lit l = { literal };
  return l;
}
/*----------------------------------------------------------------------------*/
typedef struct malc_mem {
  u8 const* mem;
  u16       size;
}
malc_mem;

static inline malc_mem logmem (u8 const* mem, u16 size)
{
  bl_assert ((mem && size) || size == 0);
  malc_mem b = { mem, size };
  return b;
}
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
#define malc_get_type_code(value)\
  _Generic ((value),\
    float:    (char) malc_type_float,\
    double:   (char) malc_type_double,\
    i8:       (char) malc_type_i8,\
    u8:       (char) malc_type_u8,\
    i16:      (char) malc_type_i16,\
    u16:      (char) malc_type_u16,\
    i32:      (char) malc_type_i32,\
    u32:      (char) malc_type_u32,\
    i64:      (char) malc_type_i64,\
    u64:      (char) malc_type_u64,\
    void*:    (char) malc_type_vptr,\
    malc_lit: (char) malc_type_lit,\
    malc_str: (char) malc_type_str,\
    malc_mem: (char) malc_type_bytes,\
    default:  (char) malc_type_error\
    )

#define malc_get_type_min_size(value)\
  _Generic ((value),\
    float:    (uword) sizeof (float),\
    double:   (uword) sizeof (double),\
    i8:       (uword) sizeof (i8),\
    u8:       (uword) sizeof (u8),\
    i16:      (uword) sizeof (i16),\
    u16:      (uword) sizeof (u16),\
    i32:      (uword) 1,\
    u32:      (uword) 1,\
    i64:      (uword) 1,\
    u64:      (uword) 1,\
    void*:    (uword) sizeof (void*),\
    malc_lit: (uword) sizeof (void*),\
    malc_str: (uword) sizeof (u16),\
    malc_mem: (uword) sizeof (u16),\
    default:  (uword) 0\
    )

#define malc_get_type_max_size(value)\
  _Generic ((value),\
    float:    (uword) sizeof (float),\
    double:   (uword) sizeof (double),\
    i8:       (uword) sizeof (i8),\
    u8:       (uword) sizeof (u8),\
    i16:      (uword) sizeof (i16),\
    u16:      (uword) sizeof (u16),\
    i32:      (uword) sizeof (i32),\
    u32:      (uword) sizeof (u32),\
    i64:      (uword) sizeof (i64),\
    u64:      (uword) sizeof (u64),\
    void*:    (uword) sizeof (void*),\
    malc_lit: (uword) sizeof (void*),\
    malc_str: (uword) (sizeof (u16) + utype_max (u16)),\
    malc_mem: (uword) (sizeof (u16) + utype_max (u16)),\
    default:  (uword) 0\
    )
/*----------------------------------------------------------------------------*/
#else
template<typename T> struct mal_type_traits {};

template<> struct mal_type_traits<float> {
  static const char  code = malc_type_float;
  static const uword min  = sizeof (float);
  static const uword max  = min;
};

template<> struct mal_type_traits<double> {
  static const char  code = malc_type_double;
  static const uword min  = sizeof (double);
  static const uword max  = min;
};

template<> struct mal_type_traits<i8> {
  static const char  code = malc_type_i8;
  static const uword min  = sizeof (i8);
  static const uword max  = min;
};

template<> struct mal_type_traits<u8> {
  static const char  code = malc_type_u8;
  static const uword min  = sizeof (u8);
  static const uword max  = min;
};

template<> struct mal_type_traits<i16> {
  static const char  code = malc_type_i16;
  static const uword min  = sizeof (i16);
  static const uword max  = min;
};

template<> struct mal_type_traits<u16> {
  static const char  code = malc_type_u16;
  static const uword min  = sizeof (u16);
  static const uword max  = min;
};

template<> struct mal_type_traits<i32> {
  static const char  code = malc_type_i32;
  static const uword min  = 1;
  static const uword max  = sizeof (i32);
};

template<> struct mal_type_traits<u32> {
  static const char  code = malc_type_u32;
  static const uword min  = 1;
  static const uword max  = sizeof (u32);
};

template<> struct mal_type_traits<i64> {
  static const char  code = malc_type_i64;
  static const uword min  = 1;
  static const uword max  = sizeof (i64);
};

template<> struct mal_type_traits<u64> {
  static const char  code = malc_type_u64;
  static const uword min  = 1;
  static const uword max  = sizeof (u64);
};

template<> struct mal_type_traits<void*> {
  static const char  code = malc_type_vptr;
  static const uword min  = sizeof (void*);
  static const uword max  = min;
};

template<> struct mal_type_traits<malc_lit> {
  static const char  code = malc_type_lit;
  static const uword min  = sizeof (void*);
  static const uword max  = min;
};

template<> struct mal_type_traits<malc_str> {
  static const char code  = malc_type_str;
  static const uword min  = sizeof (u16);
  static const uword max  = sizeof (u16) + utype_max (u16);
};

template<> struct mal_type_traits<malc_mem> {
  static const char  code = malc_type_bytes;
  static const uword min  = sizeof (u16);
  static const uword max  = sizeof (u16) + utype_max (u16);
};

#define malc_get_type_code(value)\
  mal_type_traits<decltype (value)>::code

#define malc_get_type_min_size(value)\
  mal_type_traits<decltype (value)>::min

#define malc_get_type_max_size(value)\
  mal_type_traits<decltype (value)>::max

#endif
/*----------------------------------------------------------------------------*/
#define malc_is_compressed(x) \
  ((int) (malc_get_type_code ((x)) >= malc_type_i32 && \
    malc_get_type_code ((x)) <= malc_type_u64))
/*----------------------------------------------------------------------------*/
typedef struct malc_const_entry {
  char const* format;
  char const* info;
  u16         compressed_count;
}
malc_const_entry;
/*----------------------------------------------------------------------------*/
static inline bl_err malc_log(
  void* malc_ptr,
  const malc_const_entry* e,
  uword min_size,
  uword max_size,
  int   argc,
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
static inline char malc_get_sev (void* dummy)
{
  return (char) malc_sev_warning;
}
/*----------------------------------------------------------------------------*/
#define malc_log_private_impl(malc_ptr, sev, ...) \
  malc_log( \
    (malc_ptr), \
    (malc_const_entry[]) {{ \
      /* "" is prefixed to forbid compilation of non-literal format strings*/ \
      ""pp_vargs_first (__VA_ARGS__), \
      (char[]) { \
        sev, \
        /* this block builds the "info" string (the conditional is to skip*/ \
        /* the comma when empty */ \
        pp_if (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))( \
          pp_apply( \
            malc_get_type_code, pp_comma, pp_vargs_ignore_first (__VA_ARGS__) \
            ) \
          pp_comma() \
        ) /* endif */ \
        malc_end \
      }, \
      /* this block builds the compressed field count (0 if no vargs) */ \
      pp_if_else (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))( \
        pp_apply( \
          malc_is_compressed, pp_plus, pp_vargs_ignore_first (__VA_ARGS__) \
          ) \
      ,/* else */ \
        0 \
      ) \
    }, }, \
    /* min_size (0 if no args) */ \
    pp_if_else (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))( \
      pp_apply( \
        malc_get_type_min_size, pp_plus, pp_vargs_ignore_first (__VA_ARGS__) \
        ) \
    ,/* else */ \
      0 \
    ), \
    /* max_size (0 if no args) */ \
    pp_if_else (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))( \
      pp_apply( \
        malc_get_type_max_size, pp_plus, pp_vargs_ignore_first (__VA_ARGS__) \
        ) \
    ,/* else */ \
      0 \
    ), \
    /* arg count */ \
    pp_vargs_count (pp_vargs_ignore_first (__VA_ARGS__)) \
    /* vargs (conditional to skip the trailing comma when ther are no vargs */ \
    pp_if (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))( \
      pp_comma() \
        pp_apply( \
          pp_pass, pp_comma, pp_vargs_ignore_first (__VA_ARGS__) \
          ) \
    ) /* endif */ \
    )

#define malc_log_private(malc_ptr, sev, ...) \
  ((sev >= malc_get_sev (malc_ptr)) ? \
      malc_log_private_impl (malc_ptr, sev, __VA_ARGS__) : bl_ok)
/*----------------------------------------------------------------------------*/
int main (void)
{
  u8  v8   = 1;
  u16 v16  = 2;
  u32 v32  = 3;
  i32 vi32 = -3;
  i64 vi64 = -4;

  u8 mem[4] = { 0, 1, 2, 3, };

  puts ("- omitted entry -----");
  malc_log_private(
    nullptr, malc_sev_debug, "the format string", v8, v16, v32, vi32
    );
  puts ("- 4 builtins -------");
  malc_log_private(
    nullptr, malc_sev_warning, "format string 1", v8, v16, v32, vi32, vi64
    );
  puts ("- no params -------");
  malc_log_private (nullptr, malc_sev_error, "format string 2");
  puts ("- non-builtin -------");
  malc_log_private(
    nullptr,
    malc_sev_error,
    "format string 2",
    (void*) nullptr,
    loglit ("a literal"),
    logstr ("a string", sizeof "a string" - 1),
    logmem (mem, sizeof mem)
    );
  return 0;
}
/*----------------------------------------------------------------------------*/

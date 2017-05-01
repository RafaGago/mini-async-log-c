#ifndef __MALC_INTERFACE_STACK_ARGS_H__
#define __MALC_INTERFACE_STACK_ARGS_H__

#include <bl/base/platform.h>
#include <malc/malc.h>

#ifndef __cplusplus

#if BL_WORDSIZE == 64

#define malc_get_va_arg(va_list_var, value)\
  _Generic ((value),\
    malc_tgen_cv_cases (float, (float) va_arg  (va_list_var, double)),\
    malc_tgen_cv_cases (double, (double) va_arg  (va_list_var, double)),\
    malc_tgen_cv_cases (i8, (i8) va_arg  (va_list_var, int)),\
    malc_tgen_cv_cases (u8, (u8) va_arg  (va_list_var, unsigned int)),\
    malc_tgen_cv_cases (i16, (i16) va_arg  (va_list_var, int)),\
    malc_tgen_cv_cases (u16, (u16) va_arg  (va_list_var, unsigned int)),\
    malc_tgen_cv_cases (i32, (i32) va_arg  (va_list_var, int)),\
    malc_tgen_cv_cases (u32, (u32) va_arg  (va_list_var, unsigned int)),\
    malc_tgen_cv_cases (i64, (i64) va_arg  (va_list_var, i64)),\
    malc_tgen_cv_cases (u64, (u64) va_arg  (va_list_var, u64)),\
    malc_tgen_cv_cases (void*, (void*) va_arg  (va_list_var, void*)),\
    malc_tgen_cv_cases(\
        void* const, (void* const) va_arg  (va_list_var, void*)\
        ),\
    malc_lit: (malc_lit) va_arg (va_list_var, malc_lit),\
    malc_str: (malc_str) va_arg (va_list_var, malc_str),\
    malc_mem: (malc_mem) va_arg (va_list_var, malc_mem),\
    malc_compressed_32:\
      (malc_compressed_32) va_arg (va_list_var, malc_compressed_32),\
    malc_compressed_64:\
      (malc_compressed_64) va_arg (va_list_var, malc_compressed_64),\
    default: (void) va_arg (va_list_var, void)\
    )

#else /* BL_WORDSIZE == 64 */

#define malc_get_va_arg(va_list_var, value)\
  _Generic ((value),\
    malc_tgen_cv_cases (float, (float) va_arg  (va_list_var, float)),\
    malc_tgen_cv_cases (double, (double) va_arg  (va_list_var, double)),\
    malc_tgen_cv_cases (i8, (i8) va_arg  (va_list_var, int)),\
    malc_tgen_cv_cases (u8, (u8) va_arg  (va_list_var, unsigned int)),\
    malc_tgen_cv_cases (i16, (i16) va_arg  (va_list_var, int)),\
    malc_tgen_cv_cases (u16, (u16) va_arg  (va_list_var, unsigned int)),\
    malc_tgen_cv_cases (i32, (i32) va_arg  (va_list_var, i32)),\
    malc_tgen_cv_cases (u32, (u32) va_arg  (va_list_var, u32)),\
    malc_tgen_cv_cases (i64, (i64) va_arg  (va_list_var, i64)),\
    malc_tgen_cv_cases (u64, (u64) va_arg  (va_list_var, u64)),\
    malc_tgen_cv_cases (void*, (void*) va_arg  (va_list_var, void*)),\
    malc_tgen_cv_cases(\
        void* const, (void* const) va_arg  (va_list_var, void*)\
        ),\
    malc_lit: (malc_lit) va_arg (va_list_var, malc_lit),\
    malc_str: (malc_str) va_arg (va_list_var, malc_str),\
    malc_mem: (malc_mem) va_arg (va_list_var, malc_mem),\
    malc_compressed_32:\
      (malc_compressed_32) va_arg (va_list_var, malc_compressed_32),\
    malc_compressed_64:\
      (malc_compressed_64) va_arg (va_list_var, malc_compressed_64),\
    default: (void) va_arg (va_list_var, void)\
    )

#endif /* BL_WORDSIZE == 64 */

#else /* __cplusplus */

template<typename T> struct malc_stack_type
{
  typedef T type;
};

template<> struct malc_stack_type<i8> {
  typedef int type;
};

template<> struct malc_stack_type<u8> {
  typedef unsigned int type;
};

template<> struct malc_stack_type<i16> {
  typedef int type;
};

template<> struct malc_stack_type<u16> {
  typedef unsigned int type;
};

#ifdef BL_WORDSIZE == 64

template<> struct malc_stack_type<i32> {
  typedef int type;
};

template<> struct malc_stack_type<u32> {
  typedef unsigned int type;
};

template<> struct malc_stack_type<float> {
  typedef int double;
};

#endif /* BL_WORDSIZE == 64 */

#include <type_traits>

#define malc_get_stack_type(va_list_var, value)\
  static_cast<decltype (value)>( \
    va_arg( \
      va_list_var, \
      typename malc_stack_type< \
        std::remove_cv< \
          std::remove_reference<decltype (value)>::type \
          >::type \
        >::type \
      ) \
    )

#endif /* __cplusplus */

#endif /* __MALC_INTERFACE_STACK_ARGS_H__ */
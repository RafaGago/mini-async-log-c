#ifndef __MALC_INTERFACE_STACK_ARGS_H__
#define __MALC_INTERFACE_STACK_ARGS_H__

#include <bl/base/platform.h>
#include <malc/malc.h>

#ifndef __cplusplus

#if BL_WORDSIZE == 64
  #define U32_ONSTACK int
  #define I32_ONSTACK unsigned int
  #define FLT_ONSTACK double
#else
  #define U32_ONSTACK u32
  #define I32_ONSTACK i32
  #define FLT_ONSTACK float
#endif

#define malc_get_va_arg(va_list_var, value)\
  _Generic ((value),\
    malc_tgen_cv_cases (float,  (float)  va_arg (va_list_var, FLT_ONSTACK)),\
    malc_tgen_cv_cases (double, (double) va_arg (va_list_var, double)),\
    malc_tgen_cv_cases (i8,     (i8)     va_arg (va_list_var, int)),\
    malc_tgen_cv_cases (u8,     (u8)     va_arg (va_list_var, unsigned int)),\
    malc_tgen_cv_cases (i16,    (i16)    va_arg (va_list_var, int)),\
    malc_tgen_cv_cases (u16,    (u16)    va_arg (va_list_var, unsigned int)),\
    malc_tgen_cv_cases (i32,    (i32)    va_arg (va_list_var, I32_ONSTACK)),\
    malc_tgen_cv_cases (u32,    (u32)    va_arg (va_list_var, U32_ONSTACK)),\
    malc_tgen_cv_cases (i64,    (i64)    va_arg (va_list_var, i64)),\
    malc_tgen_cv_cases (u64,    (u64)    va_arg (va_list_var, u64)),\
    malc_tgen_cv_cases (void*,  (void*)  va_arg (va_list_var, void*)),\
    malc_tgen_cv_cases(\
        void* const, (void* const) va_arg  (va_list_var, void*)\
        ),\
    malc_lit:          (malc_lit)        va_arg (va_list_var, malc_lit),\
    malc_strcp:        (malc_strcp)      va_arg (va_list_var, malc_strcp),\
    malc_strref:       (malc_strref)     va_arg (va_list_var, malc_strref),\
    malc_memcp:        (malc_memcp)      va_arg (va_list_var, malc_memcp),\
    malc_memref:       (malc_memref)     va_arg (va_list_var, malc_memref),\
    malc_refdtor:      (malc_refdtor)    va_arg (va_list_var, malc_refdtor),\
    malc_compressed_32:\
      (malc_compressed_32) va_arg (va_list_var, malc_compressed_32),\
    malc_compressed_64:\
      (malc_compressed_64) va_arg (va_list_var, malc_compressed_64),\
    malc_compressed_ref:\
      (malc_compressed_ref) va_arg (va_list_var, malc_compressed_ref),\
    malc_compressed_refdtor:\
      (malc_compressed_refdtor) va_arg (va_list_var, malc_compressed_refdtor),\
    default: (void) va_arg (va_list_var, void)\
    )

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

template<> struct malc_stack_type<i32> {
  typedef I32_ONSTACK type;
};

template<> struct malc_stack_type<u32> {
  typedef U32_ONSTACK type;
};

template<> struct malc_stack_type<float> {
  typedef FLT_ONSTACK type;
};

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
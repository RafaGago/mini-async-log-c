#ifndef __MALC_INTERFACE_STACK_ARGS_H__
#define __MALC_INTERFACE_STACK_ARGS_H__

#include <bl/base/platform.h>
#include <malc/malc.h>

#ifndef __cplusplus

#if BL_WORDSIZE == 64

#define malc_get_va_arg(va_list_var, value)\
  _Generic ((value),\
    float:                      (float)    va_arg (va_list_var, double),\
    const float:                (float)    va_arg (va_list_var, double),\
    volatile float:             (float)    va_arg (va_list_var, double),\
    const volatile float:       (float)    va_arg (va_list_var, double),\
    double:                     (double)   va_arg (va_list_var, double),\
    const double:               (double)   va_arg (va_list_var, double),\
    volatile double:            (double)   va_arg (va_list_var, double),\
    const volatile double:      (double)   va_arg (va_list_var, double),\
    i8:                         (i8)       va_arg (va_list_var, int),\
    const i8:                   (i8)       va_arg (va_list_var, int),\
    volatile i8:                (i8)       va_arg (va_list_var, int),\
    const volatile i8:          (i8)       va_arg (va_list_var, int),\
    u8:                         (u8)       va_arg (va_list_var, unsigned int),\
    const u8:                   (u8)       va_arg (va_list_var, unsigned int),\
    volatile u8:                (u8)       va_arg (va_list_var, unsigned int),\
    const volatile u8:          (u8)       va_arg (va_list_var, unsigned int),\
    i16:                        (i16)      va_arg (va_list_var, int),\
    const i16:                  (i16)      va_arg (va_list_var, int),\
    volatile i16:               (i16)      va_arg (va_list_var, int),\
    const volatile i16:         (i16)      va_arg (va_list_var, int),\
    u16:                        (u16)      va_arg (va_list_var, unsigned int),\
    const u16:                  (u16)      va_arg (va_list_var, unsigned int),\
    volatile u16:               (u16)      va_arg (va_list_var, unsigned int),\
    const volatile u16:         (u16)      va_arg (va_list_var, unsigned int),\
    i32:                        (i32)      va_arg (va_list_var, int),\
    const i32:                  (i32)      va_arg (va_list_var, int),\
    volatile i32:               (i32)      va_arg (va_list_var, int),\
    const volatile i32:         (i32)      va_arg (va_list_var, int),\
    u32:                        (u32)      va_arg (va_list_var, unsigned int),\
    const u32:                  (u32)      va_arg (va_list_var, unsigned int),\
    volatile u32:               (u32)      va_arg (va_list_var, unsigned int),\
    const volatile u32:         (u32)      va_arg (va_list_var, unsigned int),\
    i64:                        (i64)      va_arg (va_list_var, i64),\
    const i64:                  (i64)      va_arg (va_list_var, i64),\
    volatile i64:               (i64)      va_arg (va_list_var, i64),\
    const volatile i64:         (i64)      va_arg (va_list_var, i64),\
    u64:                        (u64)      va_arg (va_list_var, u64),\
    const u64:                  (u64)      va_arg (va_list_var, u64),\
    volatile u64:               (u64)      va_arg (va_list_var, u64),\
    const volatile u64:         (u64)      va_arg (va_list_var, u64),\
    void*:                      (void*)    va_arg (va_list_var, void*),\
    const void*:                (void*)    va_arg (va_list_var, void*),\
    volatile void*:             (void*)    va_arg (va_list_var, void*),\
    const volatile void*:       (void*)    va_arg (va_list_var, void*),\
    void* const:                (void*)    va_arg (va_list_var, void*),\
    const void* const:          (void*)    va_arg (va_list_var, void*),\
    volatile void* const:       (void*)    va_arg (va_list_var, void*),\
    const volatile void* const: (void*)    va_arg (va_list_var, void*),\
    malc_lit:                   (malc_lit) va_arg (va_list_var, malc_lit),\
    malc_str:                   (malc_str) va_arg (va_list_var, malc_str),\
    malc_mem:                   (malc_mem) va_arg (va_list_var, malc_mem),\
    default:                    (void)     va_arg (va_list_var, void)\
    )

#else /* BL_WORDSIZE == 64 */

#define malc_get_va_arg(va_list_var, value)\
  _Generic ((value),\
    float:                      (float)    va_arg (va_list_var, float),\
    const float:                (float)    va_arg (va_list_var, float),\
    volatile float:             (float)    va_arg (va_list_var, float),\
    const volatile float:       (float)    va_arg (va_list_var, float),\
    double:                     (double)   va_arg (va_list_var, double),\
    const double:               (double)   va_arg (va_list_var, double),\
    volatile double:            (double)   va_arg (va_list_var, double),\
    const volatile double:      (double)   va_arg (va_list_var, double),\
    i8:                         (i8)       va_arg (va_list_var, int),\
    const i8:                   (i8)       va_arg (va_list_var, int),\
    volatile i8:                (i8)       va_arg (va_list_var, int),\
    const volatile i8:          (i8)       va_arg (va_list_var, int),\
    u8:                         (u8)       va_arg (va_list_var, unsigned int),\
    const u8:                   (u8)       va_arg (va_list_var, unsigned int),\
    volatile u8:                (u8)       va_arg (va_list_var, unsigned int),\
    const volatile u8:          (u8)       va_arg (va_list_var, unsigned int),\
    i16:                        (i16)      va_arg (va_list_var, int),\
    const i16:                  (i16)      va_arg (va_list_var, int),\
    volatile i16:               (i16)      va_arg (va_list_var, int),\
    const volatile i16:         (i16)      va_arg (va_list_var, int),\
    u16:                        (u16)      va_arg (va_list_var, unsigned int),\
    const u16:                  (u16)      va_arg (va_list_var, unsigned int),\
    volatile u16:               (u16)      va_arg (va_list_var, unsigned int),\
    const volatile u16:         (u16)      va_arg (va_list_var, unsigned int),\
    i32:                        (i32)      va_arg (va_list_var, i32),\
    const i32:                  (i32)      va_arg (va_list_var, i32),\
    volatile i32:               (i32)      va_arg (va_list_var, i32),\
    const volatile i32:         (i32)      va_arg (va_list_var, i32),\
    u32:                        (u32)      va_arg (va_list_var, u32),\
    const u32:                  (u32)      va_arg (va_list_var, u32),\
    volatile u32:               (u32)      va_arg (va_list_var, u32),\
    const volatile u32:         (u32)      va_arg (va_list_var, u32),\
    i64:                        (i64)      va_arg (va_list_var, i64),\
    const i64:                  (i64)      va_arg (va_list_var, i64),\
    volatile i64:               (i64)      va_arg (va_list_var, i64),\
    const volatile i64:         (i64)      va_arg (va_list_var, i64),\
    u64:                        (u64)      va_arg (va_list_var, u64),\
    const u64:                  (u64)      va_arg (va_list_var, u64),\
    volatile u64:               (u64)      va_arg (va_list_var, u64),\
    const volatile u64:         (u64)      va_arg (va_list_var, u64),\
    void*:                      (void*)    va_arg (va_list_var, void*),\
    const void*:                (void*)    va_arg (va_list_var, void*),\
    volatile void*:             (void*)    va_arg (va_list_var, void*),\
    const volatile void*:       (void*)    va_arg (va_list_var, void*),\
    void* const:                (void*)    va_arg (va_list_var, void*),\
    const void* const:          (void*)    va_arg (va_list_var, void*),\
    volatile void* const:       (void*)    va_arg (va_list_var, void*),\
    const volatile void* const: (void*)    va_arg (va_list_var, void*),\
    malc_lit:                   (malc_lit) va_arg (va_list_var, malc_lit),\
    malc_str:                   (malc_str) va_arg (va_list_var, malc_str),\
    malc_mem:                   (malc_mem) va_arg (va_list_var, malc_mem),\
    default:                    (void)     va_arg (va_list_var, void)\
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
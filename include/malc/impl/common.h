#if (!defined (__MALC_IMPL_COMMON_H__) && !defined (MALC_COMMON_NAMESPACED)) || \
    (!defined (__MALC_IMPL_COMMON_CPP_H__) && defined (MALC_COMMON_NAMESPACED))

#ifdef MALC_COMMON_NAMESPACED
#define __MALC_IMPL_COMMON_CPP_H__
#else
#define __MALC_IMPL_COMMON_H__
#endif
/* non user-facing macros and data types */

#include <bl/base/integer.h>

#include <malc/common.h>
#include <malc/libexport.h>
/*----------------------------------------------------------------------------*/
#if MALC_BUILTIN_COMPRESSION == 0 && MALC_PTR_COMPRESSION == 0
  #define MALC_COMPRESSION 0
#else
  #define MALC_COMPRESSION 1
#endif
/*----------------------------------------------------------------------------*/
#ifndef MALC_IMPL_COMMON_HAS_MALC_ENTRY
  #define MALC_IMPL_COMMON_HAS_MALC_ENTRY 1
  /* avoid defining twice on the wrapper */
  typedef struct malc_const_entry {
    char const* format;
    char const* info; /* first char is the severity */
    bl_u16      compressed_count;
  }
  malc_const_entry;
#endif
/*----------------------------------------------------------------------------*/
#ifdef MALC_COMMON_NAMESPACED
namespace malcpp { namespace detail { namespace serialization {
#endif
/*----------------------------------------------------------------------------*/
typedef enum malc_encodings {
  malc_end           = 0,
  malc_type_i8       = 'a',
  malc_type_u8       = 'b',
  malc_type_i16      = 'c',
  malc_type_u16      = 'd',
  malc_type_i32      = 'e',
  malc_type_u32      = 'f',
  malc_type_float    = 'g',
  malc_type_i64      = 'h',
  malc_type_u64      = 'i',
  malc_type_double   = 'j',
  malc_type_ptr      = 'k',
  malc_type_lit      = 'l',
  malc_type_strcp    = 'm',
  malc_type_memcp    = 'n',
  malc_type_strref   = 'o',
  malc_type_memref   = 'p',
  malc_type_refdtor  = 'q',
  malc_type_obj      = 'r',
  malc_type_obj_ctx  = 't',
  malc_type_obj_flag = 's',
  malc_type_error    = 'Z',
}
malc_type_ids;
/*----------------------------------------------------------------------------*/
typedef struct malc_lit {
  char const* lit;
}
malc_lit;
/*----------------------------------------------------------------------------*/
typedef struct malc_memcp {
  bl_u8 const* mem;
  bl_u16       size;
}
malc_memcp;
/*----------------------------------------------------------------------------*/
typedef struct malc_memref {
  bl_u8* mem;
  bl_u16 size;
}
malc_memref;
/*----------------------------------------------------------------------------*/
typedef struct malc_strcp {
  char const* str;
  bl_u16      len;
}
malc_strcp;
/*----------------------------------------------------------------------------*/
typedef struct malc_strref {
  char*  str;
  bl_u16 len;
}
malc_strref;
/*----------------------------------------------------------------------------*/
typedef struct malc_refdtor {
  malc_refdtor_fn func;
  void*           context;
}
malc_refdtor;
/*----------------------------------------------------------------------------*/
typedef struct malc_obj {
  malc_obj_get_data_fn getdata;
  malc_obj_destroy_fn  destroy;
  void*                obj;
  bl_u8                obj_sizeof;
}
malc_obj;
/*----------------------------------------------------------------------------*/
typedef struct malc_obj_flag {
  malc_obj base;
  bl_u8    flag;
}
malc_obj_flag;
/*----------------------------------------------------------------------------*/
typedef struct malc_obj_ctx {
  malc_obj base;
  void*    context;
}
malc_obj_ctx;
/*----------------------------------------------------------------------------*/
#ifdef MALC_COMMON_NAMESPACED
}}}  //namespace malcpp { namespace detail { namespace serialization {
#endif

#endif  /* #ifndef __MALC_IMPL_COMMON_H__ */

#if (!defined (__MALC_IMPL_COMMON_H__) && !defined (MALC_COMMON_NAMESPACED)) || \
    (!defined (__MALC_IMPL_COMMON_CPP_H__) && defined (MALC_COMMON_NAMESPACED))

#ifdef MALC_COMMON_NAMESPACED
#define __MALC_IMPL_COMMON_CPP_H__
#else
#define __MALC_IMPL_COMMON_H__
#endif
/* non user-facing macros and data types */

#include <bl/base/integer.h>
#include <malc/libexport.h>
/*----------------------------------------------------------------------------*/
#if MALC_BUILTIN_COMPRESSION == 0 && MALC_PTR_COMPRESSION == 0
  #define MALC_COMPRESSION 0
#else
  #define MALC_COMPRESSION 1
#endif
/*----------------------------------------------------------------------------*/
typedef struct malc_const_entry {
  char const* format;
  char const* info; /* first char is the severity */
  bl_u16      compressed_count;
}
malc_const_entry;
/*----------------------------------------------------------------------------*/
#ifdef MALC_COMMON_NAMESPACED
namespace malcpp { namespace detail {
#endif
/*----------------------------------------------------------------------------*/
typedef enum malc_encodings {
  malc_end          = 0,
  malc_type_i8      = 'a',
  malc_type_u8      = 'b',
  malc_type_i16     = 'c',
  malc_type_u16     = 'd',
  malc_type_i32     = 'e',
  malc_type_u32     = 'f',
  malc_type_float   = 'g',
  malc_type_i64     = 'h',
  malc_type_u64     = 'i',
  malc_type_double  = 'j',
  malc_type_ptr     = 'k',
  malc_type_lit     = 'l',
  malc_type_strcp   = 'm',
  malc_type_memcp   = 'n',
  malc_type_strref  = 'o',
  malc_type_memref  = 'p',
  malc_type_refdtor = 'q',
  malc_type_error   = 'r',
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
  bl_u8 const* mem;
  bl_u16       size;
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
  char const* str;
  bl_u16      len;
}
malc_strref;
/*----------------------------------------------------------------------------*/
typedef struct malc_ref {
  void const* ref;
  bl_u16      size;
}
malc_ref;
/*----------------------------------------------------------------------------*/
typedef void (*malc_refdtor_fn)(
  void* context, malc_ref const* refs, bl_uword refs_count
  );
/*----------------------------------------------------------------------------*/
typedef struct malc_refdtor {
  malc_refdtor_fn func;
  void*           context;
}
malc_refdtor;
/*----------------------------------------------------------------------------*/
#ifdef MALC_COMMON_NAMESPACED
}}  //namespace malcpp { namespace detail {
#endif

#endif  /* #ifndef __MALC_IMPL_COMMON_H__ */

#ifndef __MALC_COMMON_H__
#define __MALC_COMMON_H__


#include <bl/base/integer.h>
#include <bl/base/error.h>

#include <malc/libexport.h>

/*----------------------------------------------------------------------------*/
#if MALC_BUILTIN_COMPRESSION == 0 && MALC_PTR_COMPRESSION == 0
  #define MALC_COMPRESSION 0
#else
  #define MALC_COMPRESSION 1
#endif
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
  extern "C" {
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
static inline bool malc_is_valid_severity (bl_uword sev)
{
  return (sev >= malc_sev_debug) && (sev <= malc_sev_critical);
}
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
typedef struct malc_const_entry {
  char const* format;
  char const* info; /* first char is the severity */
  bl_u16      compressed_count;
}
malc_const_entry;
/*----------------------------------------------------------------------------*/
struct malc;
struct malc_serializer;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_log_entry_prepare(
  struct malc*            l,
  struct malc_serializer* ext_ser,
  malc_const_entry const* entry,
  bl_uword                payload_size
  );
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT void malc_log_entry_commit(
  struct malc* l, struct malc_serializer const* ext_ser
  );
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_uword malc_get_min_severity (struct malc const* l);
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif

#endif  /* #ifndef __MALC_COMMON_H__ */

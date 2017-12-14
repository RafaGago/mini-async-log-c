#ifndef __MALC_PRIVATE_SERIALIZER_H__
#define __MALC_PRIVATE_SERIALIZER_H__

#include <string.h>

#include <bl/base/assert.h>
#include <bl/base/preprocessor_basic.h>
#include <bl/base/integer.h>

#include <malc/impl/malc_private.h>

#ifndef __cplusplus
  #define MALC_SERIALIZER_FN_NAME(suffix) pp_tokconcat(malc_serialize, suffix)
#else /* using function overloading on C++ */
  #define MALC_SERIALIZER_FN_NAME(suffix) malc_serialize
#endif

/*----------------------------------------------------------------------------*/
typedef struct malc_serializer {
    u8*   node_mem;
#ifndef MALC_NO_COMPRESSION
    u8*   compressed_header;
    uword compressed_header_idx;
#endif
    u8*   field_mem;
}
malc_serializer;
/*----------------------------------------------------------------------------*/
static inline void wrong (void) {}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_u8) (malc_serializer* s, u8 v)
{
  *s->field_mem = v;
  s->field_mem += 1;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_i8) (malc_serializer* s, i8 v)
{
  *s->field_mem = (u8) v;
  s->field_mem += 1;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_u16) (malc_serializer* s, u16 v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_i16) (malc_serializer* s, i16 v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_u32) (malc_serializer* s, u32 v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_i32) (malc_serializer* s, i32 v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_u64) (malc_serializer* s, u64 v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_i64) (malc_serializer* s, i64 v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_float)(
  malc_serializer* s, float v
  )
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_double)(
  malc_serializer* s, double v
  )
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
/* inline ONLY to let the linker optimize without issuing warnings */
static inline void MALC_SERIALIZER_FN_NAME(_comp32)(
  malc_serializer* s, malc_compressed_32 v
  )
{
#ifndef MALC_NO_COMPRESSION
  uword size = malc_compressed_get_size (v.format_nibble);
  bl_assert (size <= sizeof (u32));
  u8* hdr   = s->compressed_header;
  uword idx = s->compressed_header_idx;
  hdr[idx / 2] |= (u8) v.format_nibble << ((idx & 1) * 4);
  ++s->compressed_header_idx;
  for (uword i = 0; i < size; ++i) {
    *s->field_mem = (u8) (v.v >> (i * 8));
    ++s->field_mem;
  }
#else
  bl_assert(false && "bug! this shouldn't be called");
#endif
}
/*----------------------------------------------------------------------------*/
/* inline ONLY to let the linker optimize without issuing warnings */
static inline void MALC_SERIALIZER_FN_NAME(_comp64)(
  malc_serializer* s, malc_compressed_64 v
  )
{
#ifndef MALC_NO_COMPRESSION
  uword size = malc_compressed_get_size (v.format_nibble);
  u8* hdr   = s->compressed_header;
  uword idx = s->compressed_header_idx;
  hdr[idx / 2] |= (u8) v.format_nibble << ((idx & 1) * 4);
  ++s->compressed_header_idx;
  for (uword i = 0; i < size; ++i) {
    *s->field_mem = (u8) (v.v >> (i * 8));
    ++s->field_mem;
  }
#else
  bl_assert(false && "bug! this shouldn't be called");
#endif
}
/*----------------------------------------------------------------------------*/
#if BL_WORDSIZE == 64
  static inline void malc_serialize_compressed_ptr(
    malc_serializer* s, malc_compressed_ptr v
    )
  {
    MALC_SERIALIZER_FN_NAME(_comp64) (s, v);
  }
#else
  static inline void malc_serialize_compressed_ptr(
    malc_serializer* s, malc_compressed_ptr v
    )
  {
    MALC_SERIALIZER_FN_NAME(_comp32) (s, v);
  }
#endif
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_compref)(
  malc_serializer* s, malc_compressed_ref v
  )
{
  MALC_SERIALIZER_FN_NAME(_u16) (s, v.size);
  malc_serialize_compressed_ptr (s, v.ref);
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_comprefdtor)(
  malc_serializer* s, malc_compressed_refdtor v
  )
{
  malc_serialize_compressed_ptr (s, v.func);
  malc_serialize_compressed_ptr (s, v.context);
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_ptr) (malc_serializer* s, void* v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
/*lit come as compressed_ptr when !MALC_NO_PTR_COMPRESSION */
static inline void MALC_SERIALIZER_FN_NAME(_lit)(
  malc_serializer* s, malc_lit v
  )
{
  return MALC_SERIALIZER_FN_NAME(_ptr) (s, (void*) v.lit);
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_strcp)(
  malc_serializer* s, malc_strcp v
  )
{
  MALC_SERIALIZER_FN_NAME(_u16) (s, v.len);
  memcpy (s->field_mem, v.str, v.len);
  s->field_mem += v.len;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_strref)(
  malc_serializer* s, malc_strref v
  )
{
  MALC_SERIALIZER_FN_NAME(_u16) (s, v.len);
  MALC_SERIALIZER_FN_NAME(_ptr) (s, (void*) v.str);
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_memcp)(
  malc_serializer* s, malc_memcp v
  )
{
  MALC_SERIALIZER_FN_NAME(_u16) (s, v.size);
  memcpy (s->field_mem, v.mem, v.size);
  s->field_mem += v.size;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_memref)(
  malc_serializer* s, malc_memref v
  )
{
  MALC_SERIALIZER_FN_NAME(_u16) (s, v.size);
  MALC_SERIALIZER_FN_NAME(_ptr) (s, (void*) v.mem);
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZER_FN_NAME(_refdtor)(
  malc_serializer* s, malc_refdtor v
  )
{
  MALC_SERIALIZER_FN_NAME(_ptr) (s, (void*) v.func);
  MALC_SERIALIZER_FN_NAME(_ptr) (s, (void*) v.context);
}
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
#define malc_serialize(s, val)\
  _Generic ((val),\
    u8:                      malc_serialize_u8,\
    i8:                      malc_serialize_i8,\
    u16:                     malc_serialize_u16,\
    i16:                     malc_serialize_i16,\
    u32:                     malc_serialize_u32,\
    i32:                     malc_serialize_i32,\
    u64:                     malc_serialize_u64,\
    i64:                     malc_serialize_i64,\
    double:                  malc_serialize_double,\
    float:                   malc_serialize_float,\
    void*:                   malc_serialize_ptr,\
    malc_compressed_32:      malc_serialize_comp32,\
    malc_compressed_64:      malc_serialize_comp64,\
    malc_compressed_ref:     malc_serialize_compref,\
    malc_compressed_refdtor: malc_serialize_comprefdtor,\
    malc_lit:                malc_serialize_lit,\
    malc_strcp:              malc_serialize_strcp,\
    malc_strref:             malc_serialize_strref,\
    malc_memcp:              malc_serialize_memcp,\
    malc_memref:             malc_serialize_memref,\
    malc_refdtor:            malc_serialize_refdtor,\
    default:                 wrong\
    )\
  ((s), (val))
#endif

#endif /* __MALC_PRIVATE_SERIALIZER_H__ */

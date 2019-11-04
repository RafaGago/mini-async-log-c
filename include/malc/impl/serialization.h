#ifndef __MALC_PRIVATE_SERIALIZATION_H__
#define __MALC_PRIVATE_SERIALIZATION_H__

#include <string.h>

#include <bl/base/assert.h>
#include <bl/base/preprocessor_basic.h>
#include <bl/base/integer.h>
#include <bl/base/integer_manipulation.h>
#include <bl/base/integer_math.h>
#include <bl/base/static_assert.h>
#include <bl/base/utility.h>

#include <malc/impl/common.h>

/*----------------------------------------------------------------------------*/
typedef struct malc_compressed_32 {
  bl_u32 v;
  bl_u32 format_nibble; /*1 bit sign + 3 bit size (0-7)*/
}
malc_compressed_32;
/*----------------------------------------------------------------------------*/
typedef struct malc_compressed_64 {
  bl_u64   v;
  bl_uword format_nibble; /*1 bit sign + 3 bit size (0-7)*/
}
malc_compressed_64;
/*----------------------------------------------------------------------------*/
#if BL_WORDSIZE == 64
typedef malc_compressed_64 malc_compressed_ptr;
#elif BL_WORDSIZE == 32
typedef malc_compressed_32 malc_compressed_ptr;
#else
  #error "Unsupported bl_word size or bad compiler detection"
#endif
/*----------------------------------------------------------------------------*/
typedef struct malc_compressed_ref {
  malc_compressed_ptr ref;
  bl_u16              size;
}
malc_compressed_ref;
/*----------------------------------------------------------------------------*/
typedef struct malc_compressed_refdtor {
  malc_compressed_ptr func;
  malc_compressed_ptr context;
}
malc_compressed_refdtor;
/*----------------------------------------------------------------------------*/
static inline bl_uword malc_compressed_get_size (bl_uword nibble)
{
  return (nibble & (bl_u_bit (3) - 1)) + 1;
}
/*----------------------------------------------------------------------------*/
static inline bool malc_compressed_is_negative (bl_uword nibble)
{
  return (nibble >> 3) & 1;
}
/*----------------------------------------------------------------------------*/
static inline malc_compressed_32 malc_get_compressed_u32 (bl_u32 v)
{
  malc_compressed_32 r;
  r.v              = v;
  r.format_nibble  = v ? bl_log2_floor_unsafe_u32 (v) / 8 : 0;
  return r;
}
/*----------------------------------------------------------------------------*/
static inline malc_compressed_32 malc_get_compressed_i32 (bl_i32 v)
{
  malc_compressed_32 r = malc_get_compressed_u32 ((bl_u32) (v < 0 ? ~v : v));
  r.format_nibble     |= (v < 0) << 3;
  return r;
}
/*----------------------------------------------------------------------------*/
static inline malc_compressed_64 malc_get_compressed_u64 (bl_u64 v)
{
  malc_compressed_64 r;
  r.v              = v;
  r.format_nibble  = v ? bl_log2_floor_unsafe_u64 (v) / 8 : 0;
  return r;
}
/*----------------------------------------------------------------------------*/
static inline malc_compressed_64 malc_get_compressed_i64 (bl_i64 v)
{
  malc_compressed_64 r = malc_get_compressed_u64 ((bl_u64) (v < 0 ? ~v : v));
  r.format_nibble     |= (v < 0) << 3;
  return r;
}
/*----------------------------------------------------------------------------*/
#if BL_WORDSIZE == 64
/*----------------------------------------------------------------------------*/
static inline malc_compressed_ptr malc_get_compressed_ptr (void* v)
{
  return malc_get_compressed_u64 ((bl_u64) v);
}
/*----------------------------------------------------------------------------*/
#elif BL_WORDSIZE == 32
/*----------------------------------------------------------------------------*/
static inline malc_compressed_ptr malc_get_compressed_ptr (void* v)
{
  return malc_get_compressed_u32 ((bl_u32) v);
}
/*----------------------------------------------------------------------------*/
#else
  #error "Unsupported bl_word size or bad compiler detection"
#endif
/*----------------------------------------------------------------------------*/
typedef struct malc_serializer {
    bl_u8*   node_mem;
#if MALC_COMPRESSION == 1
    bl_u8*   compressed_header;
    bl_uword compressed_header_idx;
#endif
    bl_u8*   field_mem;
}
malc_serializer;
/*----------------------------------------------------------------------------*/
static inline void wrong (void) {}
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
  #define MALC_SERIALIZE(suffix) bl_pp_tokconcat(malc_serialize, suffix)
#else /* using function overloading on C++ */
  #define MALC_SERIALIZE(suffix) malc_serialize
#endif
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_u8) (malc_serializer* s, bl_u8 v)
{
  *s->field_mem = v;
  s->field_mem += 1;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_i8) (malc_serializer* s, bl_i8 v)
{
  *s->field_mem = (bl_u8) v;
  s->field_mem += 1;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_u16) (malc_serializer* s, bl_u16 v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_i16) (malc_serializer* s, bl_i16 v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_u32) (malc_serializer* s, bl_u32 v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_i32) (malc_serializer* s, bl_i32 v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_u64) (malc_serializer* s, bl_u64 v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_i64) (malc_serializer* s, bl_i64 v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_float)(malc_serializer* s, float v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_double)(
  malc_serializer* s, double v
  )
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_ptr) (malc_serializer* s, void* v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
/* lit comes as a compressed_ptr when MALC_PTR_COMPRESSION != 0 */
static inline void MALC_SERIALIZE(_lit)(
  malc_serializer* s, malc_lit v
  )
{
  return MALC_SERIALIZE(_ptr) (s, (void*) v.lit);
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_strcp)(
  malc_serializer* s, malc_strcp v
  )
{
  MALC_SERIALIZE(_u16) (s, v.len);
  memcpy (s->field_mem, v.str, v.len);
  s->field_mem += v.len;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_strref)(
  malc_serializer* s, malc_strref v
  )
{
  MALC_SERIALIZE(_u16) (s, v.len);
  MALC_SERIALIZE(_ptr) (s, (void*) v.str);
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_memcp)(
  malc_serializer* s, malc_memcp v
  )
{
  MALC_SERIALIZE(_u16) (s, v.size);
  memcpy (s->field_mem, v.mem, v.size);
  s->field_mem += v.size;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_memref)(
  malc_serializer* s, malc_memref v
  )
{
  MALC_SERIALIZE(_u16) (s, v.size);
  MALC_SERIALIZE(_ptr) (s, (void*) v.mem);
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_refdtor)(
  malc_serializer* s, malc_refdtor v
  )
{
  MALC_SERIALIZE(_ptr) (s, (void*) v.func);
  MALC_SERIALIZE(_ptr) (s, (void*) v.context);
}
#if MALC_COMPRESSION == 1 /*  */
/*----------------------------------------------------------------------------*/
/* inline ONLY to let the linker optimize without issuing warnings */
static inline void MALC_SERIALIZE(_comp32)(
  malc_serializer* s, malc_compressed_32 v
  )
{
  bl_uword size = malc_compressed_get_size (v.format_nibble);
  bl_assert (size <= sizeof (bl_u32));
  bl_u8* hdr   = s->compressed_header;
  bl_uword idx = s->compressed_header_idx;
  hdr[idx / 2] |= (bl_u8) v.format_nibble << ((idx & 1) * 4);
  ++s->compressed_header_idx;
  for (bl_uword i = 0; i < size; ++i) {
    *s->field_mem = (bl_u8) (v.v >> (i * 8));
    ++s->field_mem;
  }
}
/*----------------------------------------------------------------------------*/
/* inline ONLY to let the linker optimize without issuing warnings */
static inline void MALC_SERIALIZE(_comp64)(
  malc_serializer* s, malc_compressed_64 v
  )
{
  bl_uword size = malc_compressed_get_size (v.format_nibble);
  bl_u8* hdr    = s->compressed_header;
  bl_uword idx  = s->compressed_header_idx;
  hdr[idx / 2] |= (bl_u8) v.format_nibble << ((idx & 1) * 4);
  ++s->compressed_header_idx;
  for (bl_uword i = 0; i < size; ++i) {
    *s->field_mem = (bl_u8) (v.v >> (i * 8));
    ++s->field_mem;
  }
}
/*----------------------------------------------------------------------------*/
#if BL_WORDSIZE == 64
  static inline void malc_serialize_compressed_ptr(
    malc_serializer* s, malc_compressed_ptr v
    )
  {
    MALC_SERIALIZE(_comp64) (s, v);
  }
#else
  static inline void malc_serialize_compressed_ptr(
    malc_serializer* s, malc_compressed_ptr v
    )
  {
    MALC_SERIALIZE(_comp32) (s, v);
  }
#endif
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_compref)(
  malc_serializer* s, malc_compressed_ref v
  )
{
  MALC_SERIALIZE(_u16) (s, v.size);
  malc_serialize_compressed_ptr (s, v.ref);
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_comprefdtor)(
  malc_serializer* s, malc_compressed_refdtor v
  )
{
  malc_serialize_compressed_ptr (s, v.func);
  malc_serialize_compressed_ptr (s, v.context);
}
#endif /* #if MALC_COMPRESSION == 1*/
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus /* C++ uses function overload, not type generic macros */

#if MALC_COMPRESSION == 1
#define malc_serialize(s, val)\
  _Generic ((val),\
    bl_u8:                   malc_serialize_u8,\
    bl_i8:                   malc_serialize_i8,\
    bl_u16:                  malc_serialize_u16,\
    bl_i16:                  malc_serialize_i16,\
    bl_u32:                  malc_serialize_u32,\
    bl_i32:                  malc_serialize_i32,\
    bl_u64:                  malc_serialize_u64,\
    bl_i64:                  malc_serialize_i64,\
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
#else /* #if MALC_COMPRESSION == 1 */
  #define malc_serialize(s, val)\
  _Generic ((val),\
    bl_u8:                   malc_serialize_u8,\
    bl_i8:                   malc_serialize_i8,\
    bl_u16:                  malc_serialize_u16,\
    bl_i16:                  malc_serialize_i16,\
    bl_u32:                  malc_serialize_u32,\
    bl_i32:                  malc_serialize_i32,\
    bl_u64:                  malc_serialize_u64,\
    bl_i64:                  malc_serialize_i64,\
    double:                  malc_serialize_double,\
    float:                   malc_serialize_float,\
    void*:                   malc_serialize_ptr,\
    malc_lit:                malc_serialize_lit,\
    malc_strcp:              malc_serialize_strcp,\
    malc_strref:             malc_serialize_strref,\
    malc_memcp:              malc_serialize_memcp,\
    malc_memref:             malc_serialize_memref,\
    malc_refdtor:            malc_serialize_refdtor,\
    default:                 wrong\
    )\
  ((s), (val))

#endif /* #else #if MALC_COMPRESSION == 1  */
#endif /* _cplusplus */

#endif /* __MALC_PRIVATE_SERIALIZATION_H__ */

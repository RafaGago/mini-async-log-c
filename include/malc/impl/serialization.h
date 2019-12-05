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
#ifdef MALC_COMMON_NAMESPACED
namespace malcpp { namespace detail { namespace serialization {
#define MALC_CONSTEXPR constexpr
#else
#define MALC_CONSTEXPR
#endif
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
static inline MALC_CONSTEXPR bl_uword malc_compressed_get_size (bl_uword nibble)
{
  return (nibble & (bl_u_bit (3) - 1)) + 1;
}
/*----------------------------------------------------------------------------*/
static inline MALC_CONSTEXPR bool malc_compressed_is_negative (bl_uword nibble)
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
static inline malc_compressed_ptr malc_get_compressed_ptr (void const* v)
{
  return malc_get_compressed_u64 ((bl_u64) v);
}
/*----------------------------------------------------------------------------*/
#elif BL_WORDSIZE == 32
/*----------------------------------------------------------------------------*/
static inline malc_compressed_ptr malc_get_compressed_ptr (void const* v)
{
  return malc_get_compressed_u32 ((bl_u32) v);
}
/*----------------------------------------------------------------------------*/
#else
  #error "Unsupported bl_word size or bad compiler detection"
#endif
/*----------------------------------------------------------------------------*/
static inline void wrong (void) {}
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
  #define MALC_SERIALIZE(suffix) bl_pp_tokconcat (malc_serialize, suffix)
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
static inline void MALC_SERIALIZE(_ptr)(malc_serializer* s, void const* v)
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
    bl_u8:                     malc_serialize_u8,\
    bl_i8:                     malc_serialize_i8,\
    bl_u16:                    malc_serialize_u16,\
    bl_i16:                    malc_serialize_i16,\
    bl_u32:                    malc_serialize_u32,\
    bl_i32:                    malc_serialize_i32,\
    bl_u64:                    malc_serialize_u64,\
    bl_i64:                    malc_serialize_i64,\
    double:                    malc_serialize_double,\
    float:                     malc_serialize_float,\
    malc_tgen_cv_cases (void*, malc_serialize_ptr),\
    malc_compressed_32:        malc_serialize_comp32,\
    malc_compressed_64:        malc_serialize_comp64,\
    malc_compressed_ref:       malc_serialize_compref,\
    malc_compressed_refdtor:   malc_serialize_comprefdtor,\
    malc_lit:                  malc_serialize_lit,\
    malc_strcp:                malc_serialize_strcp,\
    malc_strref:               malc_serialize_strref,\
    malc_memcp:                malc_serialize_memcp,\
    malc_memref:               malc_serialize_memref,\
    malc_refdtor:              malc_serialize_refdtor,\
    default:                   wrong\
    )\
  ((s), (val))
#else /* #if MALC_COMPRESSION == 1 */
  #define malc_serialize(s, val)\
  _Generic ((val),\
    bl_u8:                     malc_serialize_u8,\
    bl_i8:                     malc_serialize_i8,\
    bl_u16:                    malc_serialize_u16,\
    bl_i16:                    malc_serialize_i16,\
    bl_u32:                    malc_serialize_u32,\
    bl_i32:                    malc_serialize_i32,\
    bl_u64:                    malc_serialize_u64,\
    bl_i64:                    malc_serialize_i64,\
    double:                    malc_serialize_double,\
    float:                     malc_serialize_float,\
    malc_tgen_cv_cases (void*, malc_serialize_ptr),\
    malc_lit:                  malc_serialize_lit,\
    malc_strcp:                malc_serialize_strcp,\
    malc_strref:               malc_serialize_strref,\
    malc_memcp:                malc_serialize_memcp,\
    malc_memref:               malc_serialize_memref,\
    malc_refdtor:              malc_serialize_refdtor,\
    default:                   wrong\
    )\
  ((s), (val))
#endif /* #else #if MALC_COMPRESSION == 1  */
#endif /* _cplusplus */
/*----------------------------------------------------------------------------*/
#define malc_tgen_cv_cases(type, expression)\
  type:                (expression),\
  const type:          (expression),\
  volatile type:       (expression),\
  const volatile type: (expression)

#define malc_get_type_code(expression)\
  _Generic ((expression),\
    malc_tgen_cv_cases (float,       (char) malc_type_float),\
    malc_tgen_cv_cases (double,      (char) malc_type_double),\
    malc_tgen_cv_cases (bl_i8,       (char) malc_type_i8),\
    malc_tgen_cv_cases (bl_u8,       (char) malc_type_u8),\
    malc_tgen_cv_cases (bl_i16,      (char) malc_type_i16),\
    malc_tgen_cv_cases (bl_u16,      (char) malc_type_u16),\
    malc_tgen_cv_cases (bl_i32,      (char) malc_type_i32),\
    malc_tgen_cv_cases (bl_u32,      (char) malc_type_u32),\
    malc_tgen_cv_cases (bl_i64,      (char) malc_type_i64),\
    malc_tgen_cv_cases (bl_u64,      (char) malc_type_u64),\
    malc_tgen_cv_cases (void*,       (char) malc_type_ptr),\
    malc_tgen_cv_cases (void* const, (char) malc_type_ptr),\
    malc_lit:                        (char) malc_type_lit,\
    malc_strcp:                      (char) malc_type_strcp,\
    malc_memcp:                      (char) malc_type_memcp,\
    malc_strref:                     (char) malc_type_strref,\
    malc_memref:                     (char) malc_type_memref,\
    malc_refdtor:                    (char) malc_type_refdtor,\
    default:                         (char) malc_type_error\
    )
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
  #define MALC_SIZE(suffix) bl_pp_tokconcat (malc_size, suffix)
#else /* using function overloading on C++ */
  #define MALC_SIZE(suffix) malc_size
#endif
/*----------------------------------------------------------------------------*/
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_float)  (float v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_double) (double v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_i8) (bl_i8 v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_u8) (bl_u8 v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_i16) (bl_i16 v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_u16) (bl_u16 v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_i32) (bl_i32 v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_u32) (bl_u32 v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_i64) (bl_i64 v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_u64) (bl_u64 v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_ptr) (void const* v)
{
  return sizeof (v);
}
#ifndef __cplusplus
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_ptrc) (const void* const v)
{
  return sizeof (v);
}
#endif
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_lit) (malc_lit v)
{
  return sizeof (v);
}

static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_strcp) (malc_strcp v)
{
  return bl_sizeof_member (malc_strcp, len) + v.len;
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_strref) (malc_strref v)
{
  return bl_sizeof_member (malc_strref, str) +
         bl_sizeof_member (malc_strref, len);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_memcp) (malc_memcp v)
{
  return bl_sizeof_member (malc_memcp, size) + v.size;
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_memref) (malc_memref v)
{
  return bl_sizeof_member (malc_memref, mem) +
         bl_sizeof_member (malc_memref, size);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_refdtor) (malc_refdtor v)
{
  return bl_sizeof_member (malc_refdtor, func) +
         bl_sizeof_member (malc_refdtor, context);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_comp32) (malc_compressed_32 v)
{
  return malc_compressed_get_size (v.format_nibble);
}
static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_comp64) (malc_compressed_64 v)
{
  return malc_compressed_get_size (v.format_nibble);
}

static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_compressed_ref) (malc_compressed_ref v)
{
  return bl_sizeof_member (malc_compressed_ref, size) +
         malc_compressed_get_size (v.ref.format_nibble);
}

static inline MALC_CONSTEXPR bl_uword MALC_SIZE(_compressed_refdtor)(
  malc_compressed_refdtor v
  )
{
  return malc_compressed_get_size (v.func.format_nibble) +
         malc_compressed_get_size (v.context.format_nibble);
}

/* generate errors saying something on "malc_type_size" for unknown types */
typedef struct malc_unknowntype_priv { char v; } malc_unknowntype_priv;
static inline bl_uword unknown_type_on_malc_type_size (malc_unknowntype_priv v)
{
  return 0;
}

#ifndef __cplusplus
#define malc_type_size(expression)\
  _Generic ((expression),\
    malc_tgen_cv_cases (float,       malc_size_float),\
    malc_tgen_cv_cases (double,      malc_size_double),\
    malc_tgen_cv_cases (bl_i8,       malc_size_i8),\
    malc_tgen_cv_cases (bl_u8,       malc_size_u8),\
    malc_tgen_cv_cases (bl_i16,      malc_size_i16),\
    malc_tgen_cv_cases (bl_u16,      malc_size_u16),\
    malc_tgen_cv_cases (bl_i32,      malc_size_i32),\
    malc_tgen_cv_cases (bl_u32,      malc_size_u32),\
    malc_tgen_cv_cases (bl_i64,      malc_size_i64),\
    malc_tgen_cv_cases (bl_u64,      malc_size_u64),\
    malc_tgen_cv_cases (void*,       malc_size_ptr),\
    malc_tgen_cv_cases (void* const, malc_size_ptrc),\
    malc_lit:                        malc_size_lit,\
    malc_strcp:                      malc_size_strcp,\
    malc_strref:                     malc_size_strref,\
    malc_memcp:                      malc_size_memcp,\
    malc_memref:                     malc_size_memref,\
    malc_refdtor:                    malc_size_refdtor,\
    malc_compressed_32:              malc_size_comp32,\
    malc_compressed_64:              malc_size_comp64,\
    malc_compressed_ref:             malc_size_compressed_ref,\
    malc_compressed_refdtor:         malc_size_compressed_refdtor,\
    default:                         unknown_type_on_malc_type_size\
    )\
  (expression)
#endif
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
  #define MALC_TRANSFORM(suffix) bl_pp_tokconcat(malc_transform, suffix)
#else /* using function overloading on C++ */
  #define MALC_TRANSFORM(suffix) malc_transform
#endif
/*----------------------------------------------------------------------------*/
static inline MALC_CONSTEXPR float MALC_TRANSFORM(_float) (float v)
{
  return v;
}
static inline MALC_CONSTEXPR double MALC_TRANSFORM(_double) (double v)
{
  return v;
}
static inline MALC_CONSTEXPR bl_i8 MALC_TRANSFORM(_i8) (bl_i8 v)
{
  return v;
}
static inline MALC_CONSTEXPR bl_u8 MALC_TRANSFORM(_u8) (bl_u8 v)
{
  return v;
}
static inline MALC_CONSTEXPR bl_i16 MALC_TRANSFORM(_i16) (bl_i16 v)
{
  return v;
}
static inline MALC_CONSTEXPR bl_u16 MALC_TRANSFORM(_u16) (bl_u16 v)
{
  return v;
}
static inline MALC_CONSTEXPR malc_strcp MALC_TRANSFORM(_strcp)  (malc_strcp v)
{
  return v;
}
static inline MALC_CONSTEXPR malc_memcp MALC_TRANSFORM(_memcp)  (malc_memcp v)
{
  return v;
}

#if MALC_BUILTIN_COMPRESSION == 0
  static inline MALC_CONSTEXPR bl_i32 MALC_TRANSFORM(_i32) (bl_i32 v)
  {
    return v;
  }
  static inline MALC_CONSTEXPR bl_u32 MALC_TRANSFORM(_u32) (bl_u32 v)
  {
    return v;
  }
  static inline MALC_CONSTEXPR bl_i64 MALC_TRANSFORM(_i64) (bl_i64 v)
  {
    return v;
  }
  static inline MALC_CONSTEXPR bl_u64 MALC_TRANSFORM(_u64) (bl_u64 v)
  {
    return v;
  }
#else
  static inline malc_compressed_32 MALC_TRANSFORM(_i32) (bl_i32 v)
  {
    return malc_get_compressed_i32 (v);
  }
  static inline malc_compressed_32 MALC_TRANSFORM(_u32) (bl_u32 v)
  {
    return malc_get_compressed_u32 (v);
  }
  static inline malc_compressed_64 MALC_TRANSFORM(_i64) (bl_i64 v)
  {
    return malc_get_compressed_i64 (v);
  }
  static inline malc_compressed_64 MALC_TRANSFORM(_u64) (bl_u64 v)
  {
    return malc_get_compressed_u64 (v);
  }
#endif

#if MALC_PTR_COMPRESSION == 0
  static inline MALC_CONSTEXPR void const* MALC_TRANSFORM(_ptr) (void const* v)
  {
    return v;
  }
#ifndef __cplusplus
  static inline MALC_CONSTEXPR void const* MALC_TRANSFORM(_ptrc) (void* const v)
  {
    return v;
  }
#endif
  static inline MALC_CONSTEXPR malc_lit MALC_TRANSFORM(_lit) (malc_lit v)
  {
    return v;
  }
  static inline MALC_CONSTEXPR
    malc_strref MALC_TRANSFORM(_strref) (malc_strref v)
  {
    return v;
  }
  static inline MALC_CONSTEXPR
    malc_memref MALC_TRANSFORM(_memref) (malc_memref v)
  {
    return v;
  }
  static inline MALC_CONSTEXPR
    malc_refdtor MALC_TRANSFORM(_refdtor)(malc_refdtor v)
  {
    return v;
  }
#else /* #if MALC_PTR_COMPRESSION == 0 */
  static inline malc_compressed_ptr MALC_TRANSFORM(_ptr) (void const* v)
  {
    return malc_get_compressed_ptr (v);
  }
#ifndef __cplusplus
  static inline malc_compressed_ptr MALC_TRANSFORM(_ptrc) (void* const v)
  {
    return malc_get_compressed_ptr ((void*) v);
  }
#endif
  static inline malc_compressed_ptr MALC_TRANSFORM(_lit)  (malc_lit v)
  {
    return malc_get_compressed_ptr ((void*) v.lit);
  }
  static inline malc_compressed_ref MALC_TRANSFORM(_strref) (malc_strref v)
  {
    malc_compressed_ref r;
    r.ref  = malc_get_compressed_ptr ((void*) v.str);
    r.size = v.len;
    return r;
  }
  static inline malc_compressed_ref MALC_TRANSFORM(_memref) (malc_memref v)
  {
    malc_compressed_ref r;
    r.ref  = malc_get_compressed_ptr ((void*) v.mem);
    r.size = v.size;
    return r;
  }
  static inline
    malc_compressed_refdtor MALC_TRANSFORM(_refdtor) (malc_refdtor v)
  {
    malc_compressed_refdtor r;
    r.func    = malc_get_compressed_ptr ((void*) v.func);
    r.context = malc_get_compressed_ptr ((void*) v.context);
    return r;
  }
#endif /* #else #if MALC_PTR_COMPRESSION == 0 */

/* generate some error on unknown types */
static inline void unknown_type_on_malc_type_transform(
  malc_unknowntype_priv v)
{}

#ifndef __cplusplus
#define malc_type_transform(expression)\
  _Generic ((expression),\
    malc_tgen_cv_cases (float,       malc_transform_float),\
    malc_tgen_cv_cases (double,      malc_transform_double),\
    malc_tgen_cv_cases (bl_i8,       malc_transform_i8),\
    malc_tgen_cv_cases (bl_u8,       malc_transform_u8),\
    malc_tgen_cv_cases (bl_i16,      malc_transform_i16),\
    malc_tgen_cv_cases (bl_u16,      malc_transform_u16),\
    malc_tgen_cv_cases (bl_i32,      malc_transform_i32),\
    malc_tgen_cv_cases (bl_u32,      malc_transform_u32),\
    malc_tgen_cv_cases (bl_i64,      malc_transform_i64),\
    malc_tgen_cv_cases (bl_u64,      malc_transform_u64),\
    malc_tgen_cv_cases (void*,       malc_transform_ptr),\
    malc_tgen_cv_cases (void* const, malc_transform_ptrc),\
    malc_lit:                        malc_transform_lit,\
    malc_strcp:                      malc_transform_strcp,\
    malc_memcp:                      malc_transform_memcp,\
    malc_strref:                     malc_transform_strref,\
    malc_memref:                     malc_transform_memref,\
    malc_refdtor:                    malc_transform_refdtor,\
    default:                         unknown_type_on_malc_type_transform\
    )\
  (expression)
#endif
/*----------------------------------------------------------------------------*/
#ifdef MALC_COMMON_NAMESPACED
}}} // namespace malcpp { namespace detail { namespace serialization {
#endif

#undef MALC_SERIALIZE
#undef MALC_SIZE
#undef MALC_TRANSFORM
#undef MALC_CONSTEXPR

#endif /* __MALC_PRIVATE_SERIALIZATION_H__ */

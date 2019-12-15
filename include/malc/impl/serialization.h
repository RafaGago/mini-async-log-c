#ifndef __MALC_PRIVATE_SERIALIZATION_H__
#define __MALC_PRIVATE_SERIALIZATION_H__

#include <string.h>

#include <bl/base/assert.h>
#include <bl/base/preprocessor_basic.h>
#include <bl/base/integer_manipulation.h>
#include <bl/base/integer_math.h>
#include <bl/base/static_assert.h>
#include <bl/base/utility.h>

#if MALC_PTR_MSB_BYTES_CUT_COUNT != 0
  #include <bl/base/endian.h>
  #define MALC_PTR_BYTE_COUNT (sizeof (void*) - MALC_PTR_MSB_BYTES_CUT_COUNT)
  bl_static_assert(
    sizeof (void*) > MALC_PTR_MSB_BYTES_CUT_COUNT,
    "MALC_PTR_MSB_BYTES_CUT_COUNT is bigger than sizeof (void*)"
    );
#else
  #define MALC_PTR_BYTE_COUNT (sizeof (void*))
#endif

#include <malc/impl/common.h>
/*----------------------------------------------------------------------------*/
/* A group of function families to do the producer-side serialization:

- malc_transform* = If required, they convert from an external type to log to
 another one  to be stored on the stack and suitable for serialization. The
 transformed type can contain processing on the value and additional attached
 data not present on the external data type itself.

- malc_size * = Get the wire-size of data type suitable for serialization.

- malc_serialize* = Serializes a data type.
*/
/*----------------------------------------------------------------------------*/
typedef struct malc_serializer {
    uint8_t* node_mem;
#if MALC_BUILTIN_COMPRESSION == 1
    uint8_t* compressed_header;
    unsigned compressed_header_idx;
#endif
    uint8_t* field_mem;
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
  uint32_t v;
  unsigned format_nibble; /*1 bit sign + 3 bit size (0-7)*/
}
malc_compressed_32;
/*----------------------------------------------------------------------------*/
typedef struct malc_compressed_64 {
  uint64_t v;
  unsigned format_nibble; /*1 bit sign + 3 bit size (0-7)*/
}
malc_compressed_64;
/*----------------------------------------------------------------------------*/
static inline MALC_CONSTEXPR unsigned malc_compressed_get_size (unsigned nibble)
{
  return (nibble & (bl_u_bit (3) - 1)) + 1;
}
/*----------------------------------------------------------------------------*/
static inline MALC_CONSTEXPR bool malc_compressed_is_negative (unsigned nibble)
{
  return (nibble >> 3) & 1;
}
/*----------------------------------------------------------------------------*/
static inline malc_compressed_32 malc_get_compressed_u32 (uint32_t v)
{
  malc_compressed_32 r;
  r.v              = v;
  r.format_nibble  = v ? bl_log2_floor_unsafe_u32 (v) / 8 : 0;
  return r;
}
/*----------------------------------------------------------------------------*/
static inline malc_compressed_32 malc_get_compressed_i32 (int32_t v)
{
  malc_compressed_32 r = malc_get_compressed_u32 ((uint32_t) (v < 0 ? ~v : v));
  r.format_nibble     |= (v < 0) << 3;
  return r;
}
/*----------------------------------------------------------------------------*/
static inline malc_compressed_64 malc_get_compressed_u64 (uint64_t v)
{
  malc_compressed_64 r;
  r.v              = v;
  r.format_nibble  = v ? bl_log2_floor_unsafe_u64 (v) / 8 : 0;
  return r;
}
/*----------------------------------------------------------------------------*/
static inline malc_compressed_64 malc_get_compressed_i64 (int64_t v)
{
  malc_compressed_64 r = malc_get_compressed_u64 ((uint64_t) (v < 0 ? ~v : v));
  r.format_nibble     |= (v < 0) << 3;
  return r;
}
/*----------------------------------------------------------------------------*/
static inline void wrong (void) {}
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
  #define MALC_SERIALIZE(suffix) bl_pp_tokconcat (malc_serialize, suffix)
#else /* using function overloading on C++ */
  #define MALC_SERIALIZE(suffix) malc_serialize
#endif
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_u8) (malc_serializer* s, uint8_t v)
{
  *s->field_mem = v;
  s->field_mem += 1;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_i8) (malc_serializer* s, int8_t v)
{
  *s->field_mem = (uint8_t) v;
  s->field_mem += 1;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_u16) (malc_serializer* s, uint16_t v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_i16) (malc_serializer* s, int16_t v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_u32) (malc_serializer* s, uint32_t v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_i32) (malc_serializer* s, int32_t v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_u64) (malc_serializer* s, uint64_t v)
{
  memcpy (s->field_mem, &v, sizeof v);
  s->field_mem += sizeof v;
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_i64) (malc_serializer* s, int64_t v)
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
#if MALC_PTR_MSB_BYTES_CUT_COUNT == 0
  memcpy (s->field_mem, &v, sizeof v);
#elif BL_ARCH_IS_LITTLE_ENDIAN
  bl_assert(
    ((uint8_t*) &v)[MALC_PTR_BYTE_COUNT] == 0 &&
    "misconfigured MALC_PTR_MSB_BYTES_CUT_COUNT."
    );
  memcpy (s->field_mem, &v, MALC_PTR_BYTE_COUNT);
#else
  bl_assert(
    ((uint8_t*) &v)[MALC_PTR_MSB_BYTES_CUT_COUNT - 1] == 0 &&
    "misconfigured MALC_PTR_MSB_BYTES_CUT_COUNT."
    );
  memcpy(
    s->field_mem,
    ((uint8_t*) &v) + MALC_PTR_MSB_BYTES_CUT_COUNT,
    MALC_PTR_BYTE_COUNT
    );
#endif
  s->field_mem += MALC_PTR_BYTE_COUNT;
}
/*----------------------------------------------------------------------------*/
/* lit comes as a compressed_ptr when MALC_PTR_COMPRESSION != 0 */
static inline void MALC_SERIALIZE(_lit)(
  malc_serializer* s, malc_lit v
  )
{
  return MALC_SERIALIZE(_ptr) (s, (void const*) v.lit);
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
  MALC_SERIALIZE(_ptr) (s, (void* const) v.str);
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
  MALC_SERIALIZE(_ptr) (s, (void const*) v.mem);
}
/*----------------------------------------------------------------------------*/
static inline void MALC_SERIALIZE(_refdtor)(
  malc_serializer* s, malc_refdtor v
  )
{
  MALC_SERIALIZE(_ptr) (s, (void const*) v.func);
  MALC_SERIALIZE(_ptr) (s, (void const*) v.context);
}
#if MALC_BUILTIN_COMPRESSION == 1 /*  */
/*----------------------------------------------------------------------------*/
/* inline ONLY to let the linker optimize without issuing warnings */
static inline void MALC_SERIALIZE(_comp32)(
  malc_serializer* s, malc_compressed_32 v
  )
{
  unsigned size = malc_compressed_get_size (v.format_nibble);
  bl_assert (size <= sizeof (uint32_t));
  uint8_t* hdr = s->compressed_header;
  unsigned idx = s->compressed_header_idx;
  hdr[idx / 2] |= (uint8_t) v.format_nibble << ((idx & 1) * 4);
  ++s->compressed_header_idx;
  for (unsigned i = 0; i < size; ++i) {
    *s->field_mem = (uint8_t) (v.v >> (i * 8));
    ++s->field_mem;
  }
}
/*----------------------------------------------------------------------------*/
/* inline ONLY to let the linker optimize without issuing warnings */
static inline void MALC_SERIALIZE(_comp64)(
  malc_serializer* s, malc_compressed_64 v
  )
{
  unsigned size = malc_compressed_get_size (v.format_nibble);
  uint8_t* hdr  = s->compressed_header;
  unsigned idx  = s->compressed_header_idx;
  hdr[idx / 2] |= (uint8_t) v.format_nibble << ((idx & 1) * 4);
  ++s->compressed_header_idx;
  for (unsigned i = 0; i < size; ++i) {
    *s->field_mem = (uint8_t) (v.v >> (i * 8));
    ++s->field_mem;
  }
}
#endif /* #if MALC_COMPRESSION == 1*/
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus /* C++ uses function overload, not type generic macros */

#if MALC_BUILTIN_COMPRESSION == 1
#define malc_serialize(s, val)\
  _Generic ((val),\
    uint8_t:                   malc_serialize_u8,\
    int8_t:                    malc_serialize_i8,\
    uint16_t:                  malc_serialize_u16,\
    int16_t:                   malc_serialize_i16,\
    uint32_t:                  malc_serialize_u32,\
    int32_t:                   malc_serialize_i32,\
    uint64_t:                  malc_serialize_u64,\
    int64_t:                   malc_serialize_i64,\
    double:                    malc_serialize_double,\
    float:                     malc_serialize_float,\
    malc_tgen_cv_cases (void*, malc_serialize_ptr),\
    malc_compressed_32:        malc_serialize_comp32,\
    malc_compressed_64:        malc_serialize_comp64,\
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
    uint8_t:                   malc_serialize_u8,\
    int8_t:                    malc_serialize_i8,\
    uint16_t:                  malc_serialize_u16,\
    int16_t:                   malc_serialize_i16,\
    uint32_t:                  malc_serialize_u32,\
    int32_t:                   malc_serialize_i32,\
    uint64_t:                  malc_serialize_u64,\
    int64_t:                   malc_serialize_i64,\
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
    malc_tgen_cv_cases (int8_t,      (char) malc_type_i8),\
    malc_tgen_cv_cases (uint8_t,     (char) malc_type_u8),\
    malc_tgen_cv_cases (int16_t,     (char) malc_type_i16),\
    malc_tgen_cv_cases (uint16_t,    (char) malc_type_u16),\
    malc_tgen_cv_cases (int32_t,     (char) malc_type_i32),\
    malc_tgen_cv_cases (uint32_t,    (char) malc_type_u32),\
    malc_tgen_cv_cases (int64_t,     (char) malc_type_i64),\
    malc_tgen_cv_cases (uint64_t,    (char) malc_type_u64),\
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
static inline MALC_CONSTEXPR size_t MALC_SIZE(_float)  (float v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_double) (double v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_i8) (int8_t v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_u8) (uint8_t v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_i16) (int16_t v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_u16) (uint16_t v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_i32) (int32_t v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_u32) (uint32_t v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_i64) (int64_t v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_u64) (uint64_t v)
{
  return sizeof (v);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_ptr) (void const* v)
{
  return MALC_PTR_BYTE_COUNT;
}
#ifndef __cplusplus
static inline MALC_CONSTEXPR size_t MALC_SIZE(_ptrc) (const void* const v)
{
  return MALC_PTR_BYTE_COUNT;
}
#endif
static inline MALC_CONSTEXPR size_t MALC_SIZE(_lit) (malc_lit v)
{
  return MALC_SIZE(_ptr) (v.lit);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_strcp) (malc_strcp v)
{
  return bl_sizeof_member (malc_strcp, len) + v.len;
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_strref) (malc_strref v)
{
  return MALC_SIZE(_ptr) (v.str) + bl_sizeof_member (malc_strref, len);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_memcp) (malc_memcp v)
{
  return bl_sizeof_member (malc_memcp, size) + v.size;
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_memref) (malc_memref v)
{
  return MALC_SIZE(_ptr) (v.mem) + bl_sizeof_member (malc_memref, size);
}
static inline size_t MALC_SIZE(_refdtor) (malc_refdtor v)
{
  /*function pointer casting to void* is undefined behavior on the standard. But
  for the relevant platforms it is OK*/
  bl_assert(
    (v.func == (malc_refdtor_fn) ((void const*) v.func)) &&
    "this platform's function pointers can't be stored on void pointers"
    );
  return MALC_SIZE(_ptr) ((void const*) v.func) + MALC_SIZE(_ptr) (v.context);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_comp32) (malc_compressed_32 v)
{
  return malc_compressed_get_size (v.format_nibble);
}
static inline MALC_CONSTEXPR size_t MALC_SIZE(_comp64) (malc_compressed_64 v)
{
  return malc_compressed_get_size (v.format_nibble);
}
/* generate errors saying something on "malc_type_size" for unknown types */
typedef struct malc_unknowntype_priv { char v; } malc_unknowntype_priv;
static inline size_t unknown_type_on_malc_type_size (malc_unknowntype_priv v)
{
  return 0;
}

#ifndef __cplusplus
#define malc_type_size(expression)\
  _Generic ((expression),\
    malc_tgen_cv_cases (float,       malc_size_float),\
    malc_tgen_cv_cases (double,      malc_size_double),\
    malc_tgen_cv_cases (int8_t,       malc_size_i8),\
    malc_tgen_cv_cases (uint8_t,       malc_size_u8),\
    malc_tgen_cv_cases (int16_t,      malc_size_i16),\
    malc_tgen_cv_cases (uint16_t,      malc_size_u16),\
    malc_tgen_cv_cases (int32_t,      malc_size_i32),\
    malc_tgen_cv_cases (uint32_t,      malc_size_u32),\
    malc_tgen_cv_cases (int64_t,      malc_size_i64),\
    malc_tgen_cv_cases (uint64_t,      malc_size_u64),\
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
static inline MALC_CONSTEXPR int8_t MALC_TRANSFORM(_i8) (int8_t v)
{
  return v;
}
static inline MALC_CONSTEXPR uint8_t MALC_TRANSFORM(_u8) (uint8_t v)
{
  return v;
}
static inline MALC_CONSTEXPR int16_t MALC_TRANSFORM(_i16) (int16_t v)
{
  return v;
}
static inline MALC_CONSTEXPR uint16_t MALC_TRANSFORM(_u16) (uint16_t v)
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
  static inline MALC_CONSTEXPR int32_t MALC_TRANSFORM(_i32) (int32_t v)
  {
    return v;
  }
  static inline MALC_CONSTEXPR uint32_t MALC_TRANSFORM(_u32) (uint32_t v)
  {
    return v;
  }
  static inline MALC_CONSTEXPR int64_t MALC_TRANSFORM(_i64) (int64_t v)
  {
    return v;
  }
  static inline MALC_CONSTEXPR uint64_t MALC_TRANSFORM(_u64) (uint64_t v)
  {
    return v;
  }
#else
  static inline malc_compressed_32 MALC_TRANSFORM(_i32) (int32_t v)
  {
    return malc_get_compressed_i32 (v);
  }
  static inline malc_compressed_32 MALC_TRANSFORM(_u32) (uint32_t v)
  {
    return malc_get_compressed_u32 (v);
  }
  static inline malc_compressed_64 MALC_TRANSFORM(_i64) (int64_t v)
  {
    return malc_get_compressed_i64 (v);
  }
  static inline malc_compressed_64 MALC_TRANSFORM(_u64) (uint64_t v)
  {
    return malc_get_compressed_u64 (v);
  }
#endif

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

/* generate some error on unknown types */
static inline void unknown_type_on_malc_type_transform(
  malc_unknowntype_priv v)
{}

#ifndef __cplusplus
#define malc_type_transform(expression)\
  _Generic ((expression),\
    malc_tgen_cv_cases (float,       malc_transform_float),\
    malc_tgen_cv_cases (double,      malc_transform_double),\
    malc_tgen_cv_cases (int8_t,      malc_transform_i8),\
    malc_tgen_cv_cases (uint8_t,     malc_transform_u8),\
    malc_tgen_cv_cases (int16_t,     malc_transform_i16),\
    malc_tgen_cv_cases (uint16_t,    malc_transform_u16),\
    malc_tgen_cv_cases (int32_t,     malc_transform_i32),\
    malc_tgen_cv_cases (uint32_t,    malc_transform_u32),\
    malc_tgen_cv_cases (int64_t,     malc_transform_i64),\
    malc_tgen_cv_cases (uint64_t,    malc_transform_u64),\
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
#if MALC_BUILTIN_COMPRESSION == 0
  #define malc_builtin_compress_count(x) 0
#else
  #define malc_builtin_compress_count(x)\
    ( \
    ((int) ((x) == malc_type_i32)) + \
    ((int) ((x) == malc_type_u32)) + \
    ((int) ((x) == malc_type_i64)) + \
    ((int) ((x) == malc_type_u64))  \
    )
#endif

#define malc_total_compress_count(type_id) \
  malc_builtin_compress_count (type_id)

/*----------------------------------------------------------------------------*/
#ifdef MALC_COMMON_NAMESPACED
}}} // namespace malcpp { namespace detail { namespace serialization {
#endif

#undef MALC_SERIALIZE
#undef MALC_SIZE
#undef MALC_TRANSFORM
#undef MALC_CONSTEXPR

#endif /* __MALC_PRIVATE_SERIALIZATION_H__ */

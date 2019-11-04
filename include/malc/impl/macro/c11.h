#ifndef __MALC_MACRO_C11_H__
#define __MALC_MACRO_C11_H__

/*
provides :
-malc_get_type_code
-malc_type_size
-malc_make_var_from_expression

By using C11
*/

#include <malc/impl/common.h>
#include <malc/impl/serialization.h>

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

static inline bl_uword malc_size_float  (float v)       { return sizeof (v); }
static inline bl_uword malc_size_double (double v)      { return sizeof (v); }
static inline bl_uword malc_size_i8     (bl_i8 v)       { return sizeof (v); }
static inline bl_uword malc_size_u8     (bl_u8 v)       { return sizeof (v); }
static inline bl_uword malc_size_i16    (bl_i16 v)      { return sizeof (v); }
static inline bl_uword malc_size_u16    (bl_u16 v)      { return sizeof (v); }
static inline bl_uword malc_size_i32    (bl_i32 v)      { return sizeof (v); }
static inline bl_uword malc_size_u32    (bl_u32 v)      { return sizeof (v); }
static inline bl_uword malc_size_i64    (bl_i64 v)      { return sizeof (v); }
static inline bl_uword malc_size_u64    (bl_u64 v)      { return sizeof (v); }
static inline bl_uword malc_size_ptr    (void* v)       { return sizeof (v); }
static inline bl_uword malc_size_ptrc   (void* const v) { return sizeof (v); }
static inline bl_uword malc_size_lit    (malc_lit v)    { return sizeof (v); }

static inline bl_uword malc_size_strcp (malc_strcp v)
{
  return bl_sizeof_member (malc_strcp, len) + v.len;
}
static inline bl_uword malc_size_strref (malc_strref v)
{
  return bl_sizeof_member (malc_strref, str) +
         bl_sizeof_member (malc_strref, len);
}
static inline bl_uword malc_size_memcp (malc_memcp v)
{
  return bl_sizeof_member (malc_memcp, size) + v.size;
}
static inline bl_uword malc_size_memref (malc_memref v)
{
  return bl_sizeof_member (malc_memref, mem) +
         bl_sizeof_member (malc_memref, size);
}
static inline bl_uword malc_size_refdtor (malc_refdtor v)
{
  return bl_sizeof_member (malc_refdtor, func) +
         bl_sizeof_member (malc_refdtor, context);
}
static inline bl_uword malc_size_comp32 (malc_compressed_32 v)
{
  return malc_compressed_get_size (v.format_nibble);
}
static inline bl_uword malc_size_comp64 (malc_compressed_64 v)
{
  return malc_compressed_get_size (v.format_nibble);
}

static inline bl_uword malc_size_compressed_ref (malc_compressed_ref v)
{
  return bl_sizeof_member (malc_compressed_ref, size) +
         malc_compressed_get_size (v.ref.format_nibble);
}

static inline bl_uword malc_size_compressed_refdtor (malc_compressed_refdtor v)
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


static inline float      malc_transform_float  (float v)      { return v; }
static inline double     malc_transform_double (double v)     { return v; }
static inline bl_i8      malc_transform_i8     (bl_i8 v)      { return v; }
static inline bl_u8      malc_transform_u8     (bl_u8 v)      { return v; }
static inline bl_i16     malc_transform_i16    (bl_i16 v)     { return v; }
static inline bl_u16     malc_transform_u16    (bl_u16 v)     { return v; }
static inline malc_strcp malc_transform_strcp  (malc_strcp v) { return v; }
static inline malc_memcp malc_transform_memcp  (malc_memcp v) { return v; }

#if MALC_BUILTIN_COMPRESSION == 0
  static inline bl_i32 malc_transform_i32 (bl_i32 v) { return v; }
  static inline bl_u32 malc_transform_u32 (bl_u32 v) { return v; }
  static inline bl_i64 malc_transform_i64 (bl_i64 v) { return v; }
  static inline bl_u64 malc_transform_u64 (bl_u64 v) { return v; }
#else
  static inline malc_compressed_32 malc_transform_i32 (bl_i32 v)
  {
    return malc_get_compressed_i32 (v);
  }
  static inline malc_compressed_32 malc_transform_u32 (bl_u32 v)
  {
    return malc_get_compressed_u32 (v);
  }
  static inline malc_compressed_64 malc_transform_i64 (bl_i64 v)
  {
    return malc_get_compressed_i64 (v);
  }
  static inline malc_compressed_64 malc_transform_u64 (bl_u64 v)
  {
    return malc_get_compressed_u64 (v);
  }
#endif

#if MALC_PTR_COMPRESSION == 0
  static inline void*       malc_transform_ptr    (void* v)       { return v; }
  static inline void* const malc_transform_ptrc   (void* const v) { return v; }
  static inline malc_lit    malc_transform_lit    (malc_lit v)    { return v; }
  static inline malc_strref malc_transform_strref (malc_strref v) { return v; }
  static inline malc_memref malc_transform_memref (malc_memref v) { return v; }
  static inline malc_refdtor malc_transform_refdtor (malc_refdtor v)
  {
    return v;
  }
#else /* #if MALC_PTR_COMPRESSION == 0 */
  static inline malc_compressed_ptr malc_transform_ptr (void* v)
  {
    return malc_get_compressed_ptr (v);
  }
  static inline malc_compressed_ptr malc_transform_ptrc (void* const v)
  {
    return malc_get_compressed_ptr ((void*) v);
  }
  static inline malc_compressed_ptr malc_transform_lit  (malc_lit v)
  {
    return malc_get_compressed_ptr ((void*) v.lit);
  }
  static inline malc_compressed_ref malc_transform_strref (malc_strref v)
  {
    malc_compressed_ref r;
    r.ref  = malc_get_compressed_ptr ((void*) v.str);
    r.size = v.len;
    return r;
  }
  static inline malc_compressed_ref malc_transform_memref (malc_memref v)
  {
    malc_compressed_ref r;
    r.ref  = malc_get_compressed_ptr ((void*) v.mem);
    r.size = v.size;
    return r;
  }
  static inline
    malc_compressed_refdtor malc_transform_refdtor (malc_refdtor v)
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

#define malc_make_var_from_expression(expression, name)\
  typeof (malc_type_transform (expression)) name = \
    malc_type_transform (expression);

#endif /* __MALC_MACRO_C11_H__ */

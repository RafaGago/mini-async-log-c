#ifndef __MALC_C11_H__
#define __MALC_C11_H__

/* Macro-based implementation for logging. C and old C++ */
/*----------------------------------------------------------------------------*/
#if !defined (__MALC_H__)
  #error "This file should be included through malc.h"
#endif

#include <bl/base/static_assert.h>
#include <bl/base/preprocessor.h>

#include <malc/impl/common.h>
#include <malc/impl/logging.h>
#include <malc/impl/serialization.h>


#include <malc/common.h>
#include <malc/impl/common.h>
#include <malc/impl/serialization.h>

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
/*----------------------------------------------------------------------------*/
#if MALC_PTR_COMPRESSION == 0
  #define malc_ptr_compress_count(x) 0
#else
  #define malc_ptr_compress_count(x)\
    (((int) malc_get_type_code ((x)) == malc_type_ptr) + \
     ((int) malc_get_type_code ((x)) == malc_type_lit) + \
     ((int) malc_get_type_code ((x)) == malc_type_strref) + \
     ((int) malc_get_type_code ((x)) == malc_type_memref) + \
     (((int) malc_get_type_code ((x)) == malc_type_refdtor) * 2) \
    )
#endif

#if MALC_BUILTIN_COMPRESSION == 0
  #define malc_builtin_compress_count(x) 0
#else
  #define malc_builtin_compress_count(x)\
    (((int) malc_get_type_code ((x)) == malc_type_i32) + \
     ((int) malc_get_type_code ((x)) == malc_type_u32) + \
     ((int) malc_get_type_code ((x)) == malc_type_i64) + \
     ((int) malc_get_type_code ((x)) == malc_type_u64) \
    )
#endif
/*----------------------------------------------------------------------------*/
#define malc_compress_count(x)\
  (malc_builtin_compress_count (x) + malc_ptr_compress_count (x))
/*----------------------------------------------------------------------------*/
/* used for testing, ignore */
#ifndef MALC_GET_MIN_SEVERITY_FNAME
  #define MALC_GET_MIN_SEVERITY_FNAME malc_get_min_severity
#endif

#ifndef MALC_LOG_ENTRY_PREPARE_FNAME
  #define MALC_LOG_ENTRY_PREPARE_FNAME malc_log_entry_prepare
#endif

#ifndef MALC_LOG_ENTRY_COMMIT_FNAME
  #define MALC_LOG_ENTRY_COMMIT_FNAME malc_log_entry_commit
#endif
/*----------------------------------------------------------------------------*/
#define MALC_LOG_CREATE_CONST_ENTRY(sev, ...) \
  static const char bl_pp_tokconcat(malc_const_info_, __LINE__)[] = { \
    (char) (sev), \
    /* this block builds the "info" string (the conditional is to skip*/ \
    /* the comma when empty */ \
    bl_pp_if (bl_pp_has_vargs (bl_pp_vargs_ignore_first (__VA_ARGS__)))( \
      bl_pp_apply( \
        malc_get_type_code, \
        bl_pp_comma, \
        bl_pp_vargs_ignore_first (__VA_ARGS__) \
        ) \
      bl_pp_comma() \
    ) /* endif */ \
    (char) malc_end \
  }; \
  static const malc_const_entry bl_pp_tokconcat(malc_const_entry_, __LINE__) = { \
    /* "" is prefixed to forbid compilation of non-literal format strings*/ \
    "" bl_pp_vargs_first (__VA_ARGS__), \
    bl_pp_tokconcat (malc_const_info_, __LINE__), \
    /* this block builds the compressed field count (0 if no vargs) */ \
    bl_pp_if_else (bl_pp_has_vargs (bl_pp_vargs_ignore_first (__VA_ARGS__)))( \
      bl_pp_apply( \
        malc_compress_count, bl_pp_plus, bl_pp_vargs_ignore_first (__VA_ARGS__) \
        ) \
    ,/* else */ \
      0 \
    )/* endif */\
  }
/*----------------------------------------------------------------------------*/
#define MALC_LOG_PASS_TMP_VARIABLES(...) \
  bl_pp_apply_wid (bl_pp_vargs_second, bl_pp_comma, __VA_ARGS__)

#define MALC_GET_TYPE_SIZE_VISITOR(expr, name)  malc_type_size (name)

#define MALC_GET_SERIALIZED_TYPES_SIZE(...) \
  bl_pp_apply_wid (MALC_GET_TYPE_SIZE_VISITOR, bl_pp_plus, __VA_ARGS__)

#define MALC_APPLY_SERIALIZER(expr, varname)\
  malc_serialize(&bl_pp_tokconcat(malc_serializer_, __LINE__), varname)

#define MALC_SERIALIZE_TMP_VALUES(...) \
  bl_pp_apply_wid (MALC_APPLY_SERIALIZER, bl_pp_semicolon, __VA_ARGS__)
/*----------------------------------------------------------------------------*/
#define MALC_LOG_DECLARE_TMP_VARIABLES(...) \
  bl_pp_apply_wid (malc_make_var_from_expression, bl_pp_empty, __VA_ARGS__)
/*----------------------------------------------------------------------------*/
#define MALC_IS_CLEANUP(x)\
  ((int) malc_get_type_code ((x)) == malc_type_refdtor)

#define MALC_IS_REF(x)\
  ((int) malc_get_type_code ((x)) == malc_type_strref ||\
         malc_get_type_code ((x)) == malc_type_memref)

#define MALC_GET_CLEANUP_COUNT(...) \
  bl_pp_apply (MALC_IS_CLEANUP, bl_pp_plus, __VA_ARGS__)

#define MALC_GET_REF_COUNT(...) \
  bl_pp_apply (MALC_IS_REF, bl_pp_plus, __VA_ARGS__)

#define MALC_LOG_VALIDATE_REF_AND_CLEANUP(...) \
  static_assert( \
    ((MALC_GET_CLEANUP_COUNT (__VA_ARGS__)) == 0 && \
     (MALC_GET_REF_COUNT (__VA_ARGS__)) == 0) || \
    ((MALC_GET_CLEANUP_COUNT (__VA_ARGS__)) == 1 && \
     (MALC_GET_REF_COUNT (__VA_ARGS__))) \
    bl_pp_comma() \
    "_logstrref_ and _logmemref_ require one (and only one) " \
    "_logrefcleanup_ function. _logrefcleanup_ can only be used in log "\
    "entries that contain either a _logstrref_ or a _logmemref_."  \
    ); \
  static_assert( \
    (MALC_GET_CLEANUP_COUNT (__VA_ARGS__)) == 0 || \
    malc_get_type_code(bl_pp_vargs_last (0, __VA_ARGS__)) \
      == malc_type_refdtor \
    bl_pp_comma()\
    "_logrefcleanup_ must be the last function call parameter."\
    );
/*----------------------------------------------------------------------------*/
static inline void malc_do_add_to_refarray(
  malc_ref* a, bl_uword* idx, malc_ref const* v
  )
{
  a[*idx] = *v;
  ++(*idx);
}

static inline void malc_do_run_refdtor(
  malc_refdtor* d, malc_ref const* refs, bl_uword refs_count
  )
{
  d->func (d->context, refs, refs_count);
}

#if MALC_PTR_COMPRESSION != 0
/*----------------------------------------------------------------------------*/
/* We need to convert back from the structs prepared for compression */
static inline void malc_do_add_to_refarray_compressed(
  malc_ref* a, bl_uword* idx, malc_compressed_ref const* v
  )
{
  malc_ref r;
  r.ref  = (void const*) v->ref.v;
  r.size = v->size;
  malc_do_add_to_refarray (a, idx, &r);
}

static inline void malc_do_run_refdtor_compressed(
  malc_compressed_refdtor* d, malc_ref const* refs, bl_uword refs_count
  )
{
  malc_refdtor rd;
  rd.func    = (malc_refdtor_fn) d->func.v;
  rd.context = (void*) d->context.v;
  malc_do_run_refdtor (&rd, refs, refs_count);
}
#endif
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
/*----------------------------------------------------------------------------*/
#if MALC_PTR_COMPRESSION == 0

static inline void malc_do_add_to_refarray_pass(
    malc_ref* a, bl_uword* idx, malc_ref const* v
    )
{}

#define MALC_LOG_FILL_REF_ARRAY_VAR_IF_REF(expr, name)\
  _Generic ((name), \
    malc_strref: malc_do_add_to_refarray, \
    malc_memref: malc_do_add_to_refarray, \
    default:     malc_do_add_to_refarray_pass \
    )( \
      bl_pp_tokconcat(malc_deallocrefs_, __LINE__), \
      &bl_pp_tokconcat(malc_deallocrefs_idx, __LINE__), \
      (malc_ref const*) &(name) \
    );

static inline void malc_do_run_refdtor_pass(
    malc_refdtor* d, malc_ref const* refs, bl_uword refs_count
    )
{}

#define MALC_LOG_REF_ARRAY_DEALLOC_SEARCH_EXEC(expr, name) \
  _Generic ((name), \
    malc_refdtor: malc_do_run_refdtor, \
    default:      malc_do_run_refdtor_pass \
    )( \
      (malc_refdtor*) &name, \
      bl_pp_tokconcat(malc_deallocrefs_, __LINE__), \
      bl_pp_tokconcat(malc_deallocrefs_idx, __LINE__) \
    );
/*----------------------------------------------------------------------------*/
#else /* MALC_PTR_COMPRESSION != 0 */

static inline void malc_do_add_to_refarray_pass(
    malc_ref* a, bl_uword* idx, malc_compressed_ref const* v
    )
{}

#define MALC_LOG_FILL_REF_ARRAY_VAR_IF_REF(expr, name)\
  _Generic ((name), \
    malc_compressed_ref: malc_do_add_to_refarray_compressed, \
    default:             malc_do_add_to_refarray_pass \
    )( \
      bl_pp_tokconcat(malc_deallocrefs_, __LINE__), \
      &bl_pp_tokconcat(malc_deallocrefs_idx, __LINE__), \
      (malc_compressed_ref const*) &(name) \
    );

static inline void malc_do_run_refdtor_pass(
    malc_compressed_refdtor* d, malc_ref const* refs, bl_uword refs_count
    )
{}

#define MALC_LOG_REF_ARRAY_DEALLOC_SEARCH_EXEC(expr, name) \
  _Generic ((name), \
    malc_compressed_refdtor: malc_do_run_refdtor_compressed, \
    default:                 malc_do_run_refdtor_pass \
    )( \
      (malc_compressed_refdtor*) &name, \
      bl_pp_tokconcat(malc_deallocrefs_, __LINE__), \
      bl_pp_tokconcat(malc_deallocrefs_idx, __LINE__) \
    );

#endif /* MALC_PTR_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#else /* __cplusplus is defined*/
/*----------------------------------------------------------------------------*/
template<typename T>
inline void malc_do_add_to_refarray_cpp(
  malc_ref* a, bl_uword* idx, T const* v
  )
{}

template <typename T>
inline void malc_do_run_refdtor_cpp(
  T* d, malc_ref const* refs, bl_uword refs_count
  )
{}

/*----------------------------------------------------------------------------*/
#if MALC_PTR_COMPRESSION == 0

template<>
inline void malc_do_add_to_refarray_cpp(
  malc_ref* a, bl_uword* idx, malc_strref const* v
  )
{
  malc_do_add_to_refarray (a, idx, (malc_ref const*) v);
}

template<>
inline void malc_do_add_to_refarray_cpp(
  malc_ref* a, bl_uword* idx, malc_memref const* v
  )
{
  malc_do_add_to_refarray (a, idx, (malc_ref const*) v);
}

template <>
inline void malc_do_run_refdtor_cpp(
  malc_refdtor* d, malc_ref const* refs, bl_uword refs_count
  )
{
  malc_do_run_refdtor (d, refs, refs_count);
}
#else /* MALC_PTR_COMPRESSION != 0 */

template<>
inline void malc_do_add_to_refarray_cpp(
  malc_ref* a, bl_uword* idx, malc_compressed_ref const* v
  )
{
  malc_do_add_to_refarray_compressed (a, idx, v);
}

template <>
inline void malc_do_run_refdtor_cpp(
  malc_compressed_refdtor* d, malc_ref const* refs, bl_uword refs_count
  )
{
  malc_do_run_refdtor_compressed (d, refs, refs_count);
}
#endif /* MALC_PTR_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
#define MALC_LOG_FILL_REF_ARRAY_VAR_IF_REF(expr, name)\
  malc_do_add_to_refarray_cpp( \
    bl_pp_tokconcat(malc_deallocrefs_, __LINE__), \
    &bl_pp_tokconcat(malc_deallocrefs_idx, __LINE__), \
    &(name) \
    );

#define MALC_LOG_REF_ARRAY_DEALLOC_SEARCH_EXEC(expr, name) \
  malc_do_run_refdtor_cpp( \
    &name, \
    bl_pp_tokconcat(malc_deallocrefs_, __LINE__), \
    bl_pp_tokconcat(malc_deallocrefs_idx, __LINE__) \
    );

#endif /* * __cplusplus */
/*----------------------------------------------------------------------------*/
#define MALC_LOG_FILL_REF_ARRAY(...) \
  /* Iterates all the variables (I, II, III, etc) and only adds the reference*/\
  /* type ones to the "malc_deallocrefs_LINE" array. It uses the variables by*/\
  /* name (I, II, III) to avoid calling expressions twice (passed arguments) */\
  bl_pp_apply_wid (MALC_LOG_FILL_REF_ARRAY_VAR_IF_REF, bl_pp_empty, __VA_ARGS__)

#define MALC_LOG_REF_ARRAY_DEALLOC(...) \
  /* Iterates all the variables (I, II, III, etc) until it finds the  */\
  /* ref_dtor. Then deallocates the ref array. It does use the variables by*/\
  /* name (I, II, III) to avoid calling expressions twice (passed arguments) */\
  bl_pp_apply_wid( \
    MALC_LOG_REF_ARRAY_DEALLOC_SEARCH_EXEC, bl_pp_empty, __VA_ARGS__ \
    )
/*----------------------------------------------------------------------------*/
#if !defined (__GNUC__) && !defined (__clang__)
  #error "MALC_LOG_IF_PRIVATE needs a compiler that allows macro statement expressions"
#endif
/*----------------------------------------------------------------------------*/
#define MALC_LOG_IF_PRIVATE(cond, malc_ptr, sev, ...) \
  /* Reminder: The first __VA_ARG__ is the format string */\
  ({\
  bl_err err = bl_mkok(); \
  bl_pp_if (bl_pp_has_vargs (bl_pp_vargs_ignore_first (__VA_ARGS__)))(\
    /* Validating that the functions containing refs have a ref destructor*/\
    /* as the last argument*/\
    MALC_LOG_VALIDATE_REF_AND_CLEANUP(\
      bl_pp_vargs_ignore_first (__VA_ARGS__)\
      )\
    /* The passed expressions (args) are stored into variables, this  */\
    /* is to keep function-like semantics (evaluating every expression */\
    /* only once) and to do some data compression (if configured to). */\
    /* A register optimizer will find unnecessary copies trivial to */\
    /* remove. Variables are called I, II, III, IIII, IIIII, etc... by */\
    /* the preprocessor library.*/ \
    MALC_LOG_DECLARE_TMP_VARIABLES (bl_pp_vargs_ignore_first (__VA_ARGS__))\
    /* In case we don't log we will have to deallocate in place the*/\
    /* dynamic entries: "malc_strref" and "malc_memref". This is a*/\
    /* boolean to trigger the deallocation.*/\
    bl_uword bl_pp_tokconcat(malc_do_deallocate_, __LINE__) = 0; \
  ) /*end if*/\
  do { \
    if ((cond) && ((sev) >= MALC_GET_MIN_SEVERITY_FNAME ((malc_ptr)))) { \
      /* Create a static const data holder that saves data about this  */\
      /* call: a pointer to the format literal, a string with the type  */\
      /* of each field on each char and the number of compressed fields*/\
      MALC_LOG_CREATE_CONST_ENTRY ((sev), __VA_ARGS__); \
      bl_pp_if_else (bl_pp_has_vargs (bl_pp_vargs_ignore_first (__VA_ARGS__)))(\
        malc_serializer bl_pp_tokconcat(malc_serializer_, __LINE__);\
        /* We prepare the serializer, MALC_GET_SERIALIZED_TYPES_SIZE */\
        /* will contain the size of the payload to be written. This */\
        /* function will make a single allocation that puts in a */\
        /* contiguous cache-friendly memory chunk:*/\
        \
        /* -an intrusive linked list node. */\
        /* -const info about the entry */\
        /* -extra free space to be able to write the payload */\
        \
        /* These things are wrapped on the serializer object.*/\
        err = MALC_LOG_ENTRY_PREPARE_FNAME(\
          (malc_ptr),\
          &bl_pp_tokconcat (malc_serializer_, __LINE__),\
          &bl_pp_tokconcat (malc_const_entry_, __LINE__),\
          MALC_GET_SERIALIZED_TYPES_SIZE( \
            bl_pp_vargs_ignore_first (__VA_ARGS__) \
            )\
          );\
        if (err.own) {\
          /*enable in-place deallocation of dynamic variables after err*/ \
          bl_pp_tokconcat(malc_do_deallocate_, __LINE__) = 1; \
          break;\
        }\
        /* passing all the variables on the serializer, which already */\
        /* knows the total size. */\
        MALC_SERIALIZE_TMP_VALUES (bl_pp_vargs_ignore_first (__VA_ARGS__));\
      , /*else: no args, just a plain format string*/\
        malc_serializer bl_pp_tokconcat(malc_serializer_, __LINE__);\
        /* We prepare the serializer wit no payload to be written. This */\
        /* function will make a single allocation that puts in a */\
        /* contiguous cache-friendly memory chunk:*/\
        \
        /* -an intrusive linked list node. */\
        /* -const info about the entry */\
        \
        /* These things are wrapped on the serializer object.*/\
        err = MALC_LOG_ENTRY_PREPARE_FNAME(\
          (malc_ptr),\
          &bl_pp_tokconcat (malc_serializer_, __LINE__),\
          &bl_pp_tokconcat (malc_const_entry_, __LINE__),\
          0\
          );\
        if (err.own) {\
          /*no args, don't need to care about in-place deallocation of */ \
          /* dynamic variables after err*/ \
          break;\
        }\
      ) /*end if*/\
      /* Once all the data is written we commit the data to the queue*/\
      MALC_LOG_ENTRY_COMMIT_FNAME(\
        (malc_ptr),\
        &bl_pp_tokconcat (malc_serializer_, __LINE__)\
        );\
    } \
    else { \
      (err) = bl_mkok(); \
      bl_pp_if (bl_pp_has_vargs (bl_pp_vargs_ignore_first (__VA_ARGS__)))(\
        /* enable in-place deallocation of dynamic variables after */\
        /* filtering out an entry*/ \
        bl_pp_tokconcat(malc_do_deallocate_, __LINE__) = 1; \
      ) /*end if*/\
    } \
  } \
  while (0); \
  bl_pp_if (bl_pp_has_vargs (bl_pp_vargs_ignore_first (__VA_ARGS__)))( \
    /* Dynamic entry deallocation code. Only generated when there are */ \
    /* arguments other than the format literal*/\
    \
    /* Notice that the conditional below will (hopefully) get optimized */ \
    /* away when "MALC_GET_REF_COUNT" is == 0, as it is a compile time */ \
    /* constant*/ \
    if (bl_pp_tokconcat(malc_do_deallocate_, __LINE__) == 1 && \
      (MALC_GET_REF_COUNT (bl_pp_vargs_ignore_first (__VA_ARGS__))) > 0 \
      ) { \
      /* We create an array with reoom for all the dynamic entries and a */ \
      /* counter*/ \
      malc_ref bl_pp_tokconcat(malc_deallocrefs_, __LINE__)[ \
        MALC_GET_REF_COUNT (bl_pp_vargs_ignore_first (__VA_ARGS__)) \
        ]; \
      bl_uword bl_pp_tokconcat(malc_deallocrefs_idx, __LINE__) = 0; \
      /* We populate the array */ \
      MALC_LOG_FILL_REF_ARRAY (bl_pp_vargs_ignore_first (__VA_ARGS__)) \
      /* run the destructor */ \
      MALC_LOG_REF_ARRAY_DEALLOC (bl_pp_vargs_ignore_first (__VA_ARGS__)) \
    } \
  ) /*end if*/\
  err; })

/*----------------------------------------------------------------------------*/
#define MALC_LOG_PRIVATE(malc_ptr, sev, ...) \
  MALC_LOG_IF_PRIVATE (1, (malc_ptr), (sev), __VA_ARGS__)
/*----------------------------------------------------------------------------*/

#endif /* __MALC_MACRO_IMPL_H__ */
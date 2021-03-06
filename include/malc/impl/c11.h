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
#define malc_compress_count(x) \
  malc_total_compress_count (malc_get_type_code ((x)))
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
#define malc_make_vars_assign_reftypes(expression, name)\
  typeof (malc_type_transform (expression)) name; \
  if (MALC_IS_REF (expression) || MALC_IS_CLEANUP (expression)) { \
    name =  malc_type_transform (expression); \
  }

#define malc_vars_assign_non_reftypes(expression, name)\
  if (!MALC_IS_REF (expression) && !MALC_IS_CLEANUP (expression)) { \
    name =  malc_type_transform (expression); \
  }

#define MALC_GET_TYPE_SIZE_VISITOR(expr, name)  malc_type_size (name)

#define MALC_GET_SERIALIZED_TYPES_SIZE(...) \
  bl_pp_apply_wid (MALC_GET_TYPE_SIZE_VISITOR, bl_pp_plus, __VA_ARGS__)

#define MALC_APPLY_SERIALIZER(expr, varname)\
  malc_serialize(&bl_pp_tokconcat(malc_serializer_, __LINE__), varname)

#define MALC_SERIALIZE_TMP_VALUES(...) \
  bl_pp_apply_wid (MALC_APPLY_SERIALIZER, bl_pp_semicolon, __VA_ARGS__)
/*----------------------------------------------------------------------------*/
#define MALC_LOG_DECLARE_TMP_VARIABLES_ASSIGN_REFTYPES(...) \
  bl_pp_apply_wid (malc_make_vars_assign_reftypes, bl_pp_empty, __VA_ARGS__)
/*----------------------------------------------------------------------------*/
#define MALC_LOG_TMP_VARIABLES_ASSIGN_NON_REFTYPES(...) \
  bl_pp_apply_wid (malc_vars_assign_non_reftypes, bl_pp_empty, __VA_ARGS__)
  /*----------------------------------------------------------------------------*/
#define MALC_IS_CLEANUP(x)\
  ((int) (malc_get_type_code ((x)) == malc_type_refdtor))

#define MALC_IS_REF(x)\
  ((int) (malc_get_type_code ((x)) == malc_type_strref ||\
          malc_get_type_code ((x)) == malc_type_memref))

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
    "_logrefdtor_ function. _logrefdtor_ can only be used in log "\
    "entries that contain either a _logstrref_ or a _logmemref_."  \
    ); \
  static_assert( \
    (MALC_GET_CLEANUP_COUNT (__VA_ARGS__)) == 0 || \
    malc_get_type_code(bl_pp_vargs_last (0, __VA_ARGS__)) \
      == malc_type_refdtor \
    bl_pp_comma()\
    "_logrefdtor_ must be the last function call parameter."\
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
/*----------------------------------------------------------------------------*/
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
    /* is to evaluate every expression only once. Variables are called I, */\
    /* II, III, IIII, IIIII, etc... by the preprocessor library. */\
    \
    /* Lazy argument evaluation is in place for non-reftype logging types, */\
    /* so only the reftypes are assigned to the variables at this point, */\
    /* the other types are assigned inside the conditional.*/\
    MALC_LOG_DECLARE_TMP_VARIABLES_ASSIGN_REFTYPES( \
      bl_pp_vargs_ignore_first (__VA_ARGS__) \
      )\
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
        /* lazy-evaluation, we assing the non-variable types after we */ \
        /* know that the types are going to be logged*/ \
        MALC_LOG_TMP_VARIABLES_ASSIGN_NON_REFTYPES( \
          bl_pp_vargs_ignore_first (__VA_ARGS__) \
          )\
        /* declare a serializer variable */\
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
      /* We create an array with room for all the dynamic entries and a */ \
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

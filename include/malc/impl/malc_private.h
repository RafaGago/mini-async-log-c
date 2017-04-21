#ifndef __MALC_PRIVATE_H__
#define __MALC_PRIVATE_H__

#if !defined (__MALC_H__)
  #error "This file should be included through malc.h"
#endif

#include <bl/base/integer.h>
#include <bl/base/integer_manipulation.h>
#include <bl/base/error.h>
#include <bl/base/preprocessor.h>
#include <malc/libexport.h>

/*----------------------------------------------------------------------------*/
typedef enum malc_encodings {
  malc_end          = 0,
  malc_type_float   = 'a',
  malc_type_double  = 'b',
  malc_type_i8      = 'c',
  malc_type_u8      = 'd',
  malc_type_i16     = 'e',
  malc_type_u16     = 'f',
  malc_type_i32     = 'g',
  malc_type_u32     = 'h',
  malc_type_i64     = 'i',
  malc_type_u64     = 'j',
  malc_type_vptr    = 'k',
  malc_type_lit     = 'l',
  malc_type_str     = 'm',
  malc_type_bytes   = 'n',
  malc_type_error   = 'o',
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
typedef struct malc_lit {
  char const* lit;
}
malc_lit;
/*----------------------------------------------------------------------------*/
typedef struct malc_mem {
  u8 const* mem;
  u16       size;
}
malc_mem;
/*----------------------------------------------------------------------------*/
#if !defined (__cplusplus)
#define malc_get_type_code(value)\
  _Generic ((value),\
    float:       (char) malc_type_float,\
    double:      (char) malc_type_double,\
    i8:          (char) malc_type_i8,\
    u8:          (char) malc_type_u8,\
    i16:         (char) malc_type_i16,\
    u16:         (char) malc_type_u16,\
    i32:         (char) malc_type_i32,\
    u32:         (char) malc_type_u32,\
    i64:         (char) malc_type_i64,\
    u64:         (char) malc_type_u64,\
    void*:       (char) malc_type_vptr,\
    malc_lit:    (char) malc_type_lit,\
    char const*: (char) malc_type_str,\
    malc_mem:    (char) malc_type_bytes,\
    default:     (char) malc_type_error\
    )

#define malc_get_type_min_size(value)\
  _Generic ((value),\
    float:       (uword) sizeof (float),\
    double:      (uword) sizeof (double),\
    i8:          (uword) sizeof (i8),\
    u8:          (uword) sizeof (u8),\
    i16:         (uword) sizeof (i16),\
    u16:         (uword) sizeof (u16),\
    i32:         (uword) 1,\
    u32:         (uword) 1,\
    i64:         (uword) 1,\
    u64:         (uword) 1,\
    void*:       (uword) sizeof (void*),\
    malc_lit:    (uword) sizeof (char const*),\
    char const*: (uword) sizeof (u16),\
    malc_mem:    (uword) sizeof (u16),\
    default:     (uword) 0\
    )

#define malc_get_type_max_size(value)\
  _Generic ((value),\
    float:       (uword) sizeof (float),\
    double:      (uword) sizeof (double),\
    i8:          (uword) sizeof (i8),\
    u8:          (uword) sizeof (u8),\
    i16:         (uword) sizeof (i16),\
    u16:         (uword) sizeof (u16),\
    i32:         (uword) sizeof (i32),\
    u32:         (uword) sizeof (u32),\
    i64:         (uword) sizeof (i64),\
    u64:         (uword) sizeof (u64),\
    void*:       (uword) sizeof (void*),\
    malc_lit:    (uword) sizeof (char const*),\
    char const*: (uword) (sizeof (u16) + utype_max (u16)),\
    malc_mem:    (uword) (sizeof (u16) + utype_max (u16)),\
    default:     (uword) 0\
    )
/*----------------------------------------------------------------------------*/
#else /* __cplusplus */
template<typename T> struct mal_type_traits {};

template<> struct mal_type_traits<float> {
  static const char  code = malc_type_float;
  static const uword min  = sizeof (float);
  static const uword max  = min;
};

template<> struct mal_type_traits<double> {
  static const char  code = malc_type_double;
  static const uword min  = sizeof (double);
  static const uword max  = min;
};

template<> struct mal_type_traits<i8> {
  static const char  code = malc_type_i8;
  static const uword min  = sizeof (i8);
  static const uword max  = min;
};

template<> struct mal_type_traits<u8> {
  static const char  code = malc_type_u8;
  static const uword min  = sizeof (u8);
  static const uword max  = min;
};

template<> struct mal_type_traits<i16> {
  static const char  code = malc_type_i16;
  static const uword min  = sizeof (i16);
  static const uword max  = min;
};

template<> struct mal_type_traits<u16> {
  static const char  code = malc_type_u16;
  static const uword min  = sizeof (u16);
  static const uword max  = min;
};

template<> struct mal_type_traits<i32> {
  static const char  code = malc_type_i32;
  static const uword min  = 1;
  static const uword max  = sizeof (i32);
};

template<> struct mal_type_traits<u32> {
  static const char  code = malc_type_u32;
  static const uword min  = 1;
  static const uword max  = sizeof (u32);
};

template<> struct mal_type_traits<i64> {
  static const char  code = malc_type_i64;
  static const uword min  = 1;
  static const uword max  = sizeof (i64);
};

template<> struct mal_type_traits<u64> {
  static const char  code = malc_type_u64;
  static const uword min  = 1;
  static const uword max  = sizeof (u64);
};

template<> struct mal_type_traits<void*> {
  static const char  code = malc_type_vptr;
  static const uword min  = sizeof (void*);
  static const uword max  = min;
};

template<> struct mal_type_traits<malc_lit> {
  static const char  code = malc_type_lit;
  static const uword min  = sizeof (char const*);
  static const uword max  = min;
};

template<> struct mal_type_traits<char const*> {
  static const char code  = malc_type_str;
  static const uword min  = sizeof (u16);
  static const uword max  = sizeof (u16) + utype_max (u16);
};

template<> struct mal_type_traits<malc_mem> {
  static const char  code = malc_type_bytes;
  static const uword min  = sizeof (u16);
  static const uword max  = sizeof (u16) + utype_max (u16);
};

#define malc_get_type_code(value)\
  mal_type_traits<decltype (value)>::code

#define malc_get_type_min_size(value)\
  mal_type_traits<decltype (value)>::min

#define malc_get_type_max_size(value)\
  mal_type_traits<decltype (value)>::max

#endif /*__cplusplus*/
/*----------------------------------------------------------------------------*/
#define malc_is_compressed(x) \
  ((int) (malc_get_type_code ((x)) >= malc_type_i32 && \
    malc_get_type_code ((x)) <= malc_type_u64))
/*----------------------------------------------------------------------------*/
typedef struct malc_const_entry {
  char const* format;
  char const* info;
  u16         compressed_count;
}
malc_const_entry;
/*----------------------------------------------------------------------------*/
extern  MALC_EXPORT bl_err malc_log(
  void* malc_ptr,
  const malc_const_entry* e,
  uword min_size,
  uword max_size,
  int   argc,
  ...
  );
/*----------------------------------------------------------------------------*/
#define MALC_LOG_PRIVATE_IMPL(malc_ptr, sev, ...) \
  malc_log( \
    (malc_ptr), \
    (malc_const_entry[]) {{ \
      /* "" is prefixed to forbid compilation of non-literal format strings*/ \
      ""pp_vargs_first (__VA_ARGS__), \
      (char[]) { \
        sev, \
        /* this block builds the "info" string (the conditional is to skip*/ \
        /* the comma when empty */ \
        pp_if (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))( \
          pp_apply( \
            malc_get_type_code, pp_comma, pp_vargs_ignore_first (__VA_ARGS__) \
            ) \
          pp_comma() \
        ) /* endif */ \
        malc_end \
      }, \
      /* this block builds the compressed field count (0 if no vargs) */ \
      pp_if_else (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))( \
        pp_apply( \
          malc_is_compressed, pp_plus, pp_vargs_ignore_first (__VA_ARGS__) \
          ) \
      ,/* else */ \
        0 \
      ) \
    }, }, \
    /* min_size (0 if no args) */ \
    pp_if_else (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))( \
      pp_apply( \
        malc_get_type_min_size, pp_plus, pp_vargs_ignore_first (__VA_ARGS__) \
        ) \
    ,/* else */ \
      0 \
    ), \
    /* max_size (0 if no args) */ \
    pp_if_else (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))( \
      pp_apply( \
        malc_get_type_max_size, pp_plus, pp_vargs_ignore_first (__VA_ARGS__) \
        ) \
    ,/* else */ \
      0 \
    ), \
    /* arg count */ \
    pp_vargs_count (pp_vargs_ignore_first (__VA_ARGS__)) \
    /* vargs (conditional to skip the trailing comma when ther are no vargs */ \
    pp_if (pp_has_vargs (pp_vargs_ignore_first (__VA_ARGS__)))( \
      pp_comma() \
        pp_apply( \
          pp_pass, pp_comma, pp_vargs_ignore_first (__VA_ARGS__) \
          ) \
    ) /* endif */ \
    )

#define MALC_LOG_PRIVATE(malc_ptr, sev, ...) \
  ((sev >= malc_get_sev ((malc_ptr))) ? \
      MALC_LOG_PRIVATE_IMPL ((malc_ptr), (sev), __VA_ARGS__) : bl_ok)

#define MALC_LOG_IF_PRIVATE(malc_ptr, condition, sev, ...) \
  ((condition) && (sev >= malc_get_sev ((malc_ptr))) ? \
      MALC_LOG_PRIVATE_IMPL ((malc_ptr), (sev), __VA_ARGS__) : bl_ok)
/*----------------------------------------------------------------------------*/
#if !defined (MALC_STRIP_LOG_FILELINE)
  #define MALC_TO_STR(s) #s
  #define MALC_CONCAT_FILELINE(file, line) "(" file ":" MALC_TO_STR (lin) ") "
  #define MALC_FILELINE MALC_CONCAT_FILELINE (__FILE__, __LINE__)
#else
  #define MALC_FILELINE
#endif
/*----------------------------------------------------------------------------*/
#if !defined (MAL_GET_LOGGER_INSTANCE_FUNCNAME)
    #define MAL_GET_LOGGER_INSTANCE_FUNC get_malc_logger_instance()
#else
    #define MAL_GET_LOGGER_INSTANCE_FUNC MAL_GET_LOGGER_INSTANCE_FUNCNAME()
#endif
/*----------------------------------------------------------------------------*/
#ifdef MALC_STRIP_LOG_DEBUG
  #ifndef MALC_STRIP_LOG_SEVERITY
    #define MALC_STRIP_LOG_SEVERITY 0
  #else
    #error "use either MALC_STRIP_LOG_SEVERITY or MALC_STRIP_LOG_DEBUG"
  #endif
#endif

#ifdef MALC_STRIP_LOG_TRACE
  #ifndef MALC_STRIP_LOG_SEVERITY
    #define MALC_STRIP_LOG_SEVERITY 1
  #else
    #error "use either MALC_STRIP_LOG_SEVERITY or MALC_STRIP_LOG_TRACE"
  #endif
#endif

#ifdef MALC_STRIP_LOG_NOTICE
  #ifndef MALC_STRIP_LOG_SEVERITY
    #define MALC_STRIP_LOG_SEVERITY 2
  #else
    #error "use either MALC_STRIP_LOG_SEVERITY or MALC_STRIP_LOG_NOTICE"
  #endif
#endif

#ifdef MALC_STRIP_LOG_WARNING
  #ifndef MALC_STRIP_LOG_SEVERITY
    #define MALC_STRIP_LOG_SEVERITY 3
  #else
    #error "use either MALC_STRIP_LOG_SEVERITY or MALC_STRIP_LOG_WARNING"
  #endif
#endif

#ifdef MALC_STRIP_LOG_ERROR
  #ifndef MALC_STRIP_LOG_SEVERITY
    #define MALC_STRIP_LOG_SEVERITY 4
  #else
    #error "use either MALC_STRIP_LOG_SEVERITY or MALC_STRIP_LOG_ERROR"
  #endif
#endif

#if defined (MALC_STRIP_LOG_CRITICAL) || defined (MALC_STRIP_ALL)
  #ifndef MALC_STRIP_LOG_SEVERITY
    #define MALC_STRIP_LOG_SEVERITY 5
  #else
    #error "use either MALC_STRIP_LOG_SEVERITY or MALC_STRIP_LOG_CRITICAL"
  #endif
#endif

#if defined (MALC_STRIP_LOG_SEVERITY) &&\
  MALC_STRIP_LOG_SEVERITY >= 0 &&\
  !defined (MALC_STRIP_LOG_DEBUG)
    #define MALC_STRIP_LOG_DEBUG
#endif

#if defined(MALC_STRIP_LOG_SEVERITY) &&\
  MALC_STRIP_LOG_SEVERITY >= 1 &&\
  !defined (MALC_STRIP_LOG_TRACE)
    #define MALC_STRIP_LOG_TRACE
#endif

#if defined(MALC_STRIP_LOG_SEVERITY) &&\
  MALC_STRIP_LOG_SEVERITY >= 2 &&\
  !defined (MALC_STRIP_LOG_NOTICE)
    #define MALC_STRIP_LOG_NOTICE
#endif

#if defined(MALC_STRIP_LOG_SEVERITY) &&\
  MALC_STRIP_LOG_SEVERITY >= 3 &&\
  !defined (MALC_STRIP_LOG_WARNING)
    #define MALC_STRIP_LOG_WARNING
#endif

#if defined(MALC_STRIP_LOG_SEVERITY) &&\
  MALC_STRIP_LOG_SEVERITY >= 4 &&\
  !defined (MALC_STRIP_LOG_ERROR)
    #define MALC_STRIP_LOG_ERROR
#endif

#if defined(MALC_STRIP_LOG_SEVERITY) &&\
  MALC_STRIP_LOG_SEVERITY >= 5 &&\
  !defined (MALC_STRIP_LOG_CRITICAL)
    #define MALC_STRIP_LOG_CRITICAL
#endif
/*----------------------------------------------------------------------------*/
static inline bl_error malc_warning_silencer() { return bl_ok; }
/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_DEBUG

#define MALC_DEBUG(...)\
  MALC_LOG_PRIVATE(MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_debug, __VA_ARGS__)

#define MALC_DEBUG_IF(condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    MALC_GET_LOGGER_INSTANCE_FUNC, (condition), malc_sev_debug, __VA_ARGS__\
    )

#define MALC_DEBUG_I(malc_ptr, ...)\
  MALC_LOG_PRIVATE ((malc_ptr), malc_sev_debug, __VA_ARGS__)

#define MALC_DEBUG_I_IF(malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (malc_ptr), (condition), malc_sev_debug, __VA_ARGS__\
    )

#else

#define MALC_DEBUG(...)                           malc_warning_silencer()
#define MALC_DEBUG_IF(condition, ...)             malc_warning_silencer()
#define MALC_DEBUG_I(malc_ptr, ...)               malc_warning_silencer()
#define MALC_DEBUG_I_IF(malc_ptr, condition, ...) malc_warning_silencer()

#endif

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_TRACE

#define MALC_TRACE(...)\
  MALC_LOG_PRIVATE(MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_trace, __VA_ARGS__)

#define MALC_TRACE_IF(condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    MALC_GET_LOGGER_INSTANCE_FUNC, (condition), malc_sev_trace, __VA_ARGS__\
    )

#define MALC_TRACE_I(malc_ptr, ...)\
  MALC_LOG_PRIVATE ((malc_ptr), malc_sev_trace, __VA_ARGS__)

#define MALC_TRACE_I_IF(malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (malc_ptr), (condition), malc_sev_trace, __VA_ARGS__\
    )

#else

#define MALC_TRACE(...)                           malc_warning_silencer()
#define MALC_TRACE_IF(condition, ...)             malc_warning_silencer()
#define MALC_TRACE_I(malc_ptr, ...)               malc_warning_silencer()
#define MALC_TRACE_I_IF(malc_ptr, condition, ...) malc_warning_silencer()

#endif

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_NOTICE

#define MALC_NOTICE(...)\
  MALC_LOG_PRIVATE (MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_note, __VA_ARGS__)

#define MALC_NOTICE_IF(condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    MALC_GET_LOGGER_INSTANCE_FUNC, (condition), malc_sev_note, __VA_ARGS__\
    )

#define MALC_NOTICE_I(malc_ptr, ...)\
  MALC_LOG_PRIVATE ((malc_ptr), malc_sev_note, __VA_ARGS__)

#define MALC_NOTICE_I_IF(malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (malc_ptr), (condition), malc_sev_note, __VA_ARGS__\
    )

#else

#define MALC_NOTICE(...)                           malc_warning_silencer()
#define MALC_NOTICE_IF(condition, ...)             malc_warning_silencer()
#define MALC_NOTICE_I(malc_ptr, ...)               malc_warning_silencer()
#define MALC_NOTICE_I_IF(malc_ptr, condition, ...) malc_warning_silencer()

#endif

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_WARNING

#define MALC_WARNING(...)\
  MALC_LOG_PRIVATE(\
    MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_warning, __VA_ARGS__\
    )

#define MALC_WARNING_IF(condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    MALC_GET_LOGGER_INSTANCE_FUNC, (condition), malc_sev_warning, __VA_ARGS__\
    )

#define MALC_WARNING_I(malc_ptr, ...)\
  MALC_LOG_PRIVATE ((malc_ptr), malc_sev_warning, __VA_ARGS__)

#define MALC_WARNING_I_IF(malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (malc_ptr), (condition), malc_sev_warning, __VA_ARGS__\
    )

#else

#define MALC_WARNING(...)                           malc_warning_silencer()
#define MALC_WARNING_IF(condition, ...)             malc_warning_silencer()
#define MALC_WARNING_I(malc_ptr, ...)               malc_warning_silencer()
#define MALC_WARNING_I_IF(malc_ptr, condition, ...) malc_warning_silencer()

#endif

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_ERROR

#define MALC_ERROR(...)\
  MALC_LOG_PRIVATE (MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_error, __VA_ARGS__)

#define MALC_ERROR_IF(condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    MALC_GET_LOGGER_INSTANCE_FUNC, (condition), malc_sev_error, __VA_ARGS__\
    )

#define MALC_ERROR_I(malc_ptr, ...)\
  MALC_LOG_PRIVATE ((malc_ptr), malc_sev_error, __VA_ARGS__)

#define MALC_ERROR_I_IF(malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (malc_ptr), (condition), malc_sev_error, __VA_ARGS__\
    )

#else

#define MALC_ERROR(...)                           malc_error_silencer()
#define MALC_ERROR_IF(condition, ...)             malc_error_silencer()
#define MALC_ERROR_I(malc_ptr, ...)               malc_error_silencer()
#define MALC_ERROR_I_IF(malc_ptr, condition, ...) malc_error_silencer()

#endif

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_CRITICAL

#define MALC_CRITICAL(...)\
  MALC_LOG_PRIVATE(\
    MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_critical, __VA_ARGS__\
    )

#define MALC_CRITICAL_IF(condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    MALC_GET_LOGGER_INSTANCE_FUNC, (condition), malc_sev_critical, __VA_ARGS__\
    )

#define MALC_CRITICAL_I(malc_ptr, ...)\
  MALC_LOG_PRIVATE ((malc_ptr), malc_sev_critical, __VA_ARGS__)

#define MALC_CRITICAL_I_IF(malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (malc_ptr), (condition), malc_sev_critical, __VA_ARGS__\
    )

#else

#define MALC_CRITICAL(...)                           malc_critical_silencer()
#define MALC_CRITICAL_IF(condition, ...)             malc_critical_silencer()
#define MALC_CRITICAL_I(malc_ptr, ...)               malc_critical_silencer()
#define MALC_CRITICAL_I_IF(malc_ptr, condition, ...) malc_critical_silencer()

#endif

#endif /*include guard*/
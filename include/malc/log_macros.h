#ifndef __MALC_USER_MACROS_H__
#define __MALC_USER_MACROS_H__

#include <bl/base/preprocessor_basic.h>
/*----------------------------------------------------------------------------*/
#ifndef __cplusplus
  #define MALC_SEV_WRAP(x) bl_pp_tokconcat (malc_sev_, x)
#else
  #define MALC_SEV_WRAP(x) bl_pp_tokconcat (malcpp::sev_, x)
#endif
/*----------------------------------------------------------------------------*/
/* malc user-facing log macros */
/*----------------------------------------------------------------------------*/
#if !defined (MALC_STRIP_LOG_FILELINE)
  #define MALC_TO_STR(s) #s
  #define MALC_CONCAT_FILELINE(file, line) "(" file ":" MALC_TO_STR (lin) ") "
  #define malc_fileline MALC_CONCAT_FILELINE (__FILE__, __LINE__)
#else
  #define malc_fileline
#endif
/*----------------------------------------------------------------------------*/
#if !defined (MALC_CUSTOM_LOGGER_INSTANCE_EXPRESSION)
    #define MALC_GET_LOGGER_INSTANCE_CALL get_malc_instance()
#else
    #define MALC_GET_LOGGER_INSTANCE_CALL MALC_CUSTOM_LOGGER_INSTANCE_EXPRESSION
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
static inline bl_err malc_stripped_entry()
{
  return bl_mkerr (bl_nothing_to_do);
}
/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_DEBUG

#define malc_debug(...)\
  MALC_LOG_PRIVATE( \
    MALC_GET_LOGGER_INSTANCE_CALL, MALC_SEV_WRAP (debug), __VA_ARGS__ \
    )

#define malc_debug_if(condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    MALC_GET_LOGGER_INSTANCE_CALL, \
    MALC_SEV_WRAP (debug), \
    __VA_ARGS__\
    )

/* _i = instance, where the instance is provided manually instead of by calling
MALC_GET_LOGGER_INSTANCE_CALL */
#define malc_debug_i(malcref, ...)\
  MALC_LOG_PRIVATE ((malcref), MALC_SEV_WRAP (debug), __VA_ARGS__)

#define malc_debug_i_if(condition, malcref, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (malcref), MALC_SEV_WRAP (debug), __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_DEBUG */

#define malc_debug(...)      malc_stripped_entry()
#define malc_debug_if(...)   malc_stripped_entry()
#define malc_debug_i(...)    malc_stripped_entry()
#define malc_debug_i_if(...) malc_stripped_entry()

#endif /* MALC_STRIP_LOG_DEBUG */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_TRACE

#define malc_trace(...)\
  MALC_LOG_PRIVATE( \
    MALC_GET_LOGGER_INSTANCE_CALL, MALC_SEV_WRAP (trace), __VA_ARGS__ \
    )

#define malc_trace_if(condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    MALC_GET_LOGGER_INSTANCE_CALL, \
    MALC_SEV_WRAP (trace), \
    __VA_ARGS__\
    )

/* _i = instance, where the instance is provided manually instead of by calling
MALC_GET_LOGGER_INSTANCE_CALL */
#define malc_trace_i(malcref, ...)\
  MALC_LOG_PRIVATE ((malcref), MALC_SEV_WRAP (trace), __VA_ARGS__)

#define malc_trace_i_if(condition, malcref, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (malcref), MALC_SEV_WRAP (trace), __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_TRACE */

#define malc_trace(...)      malc_stripped_entry()
#define malc_trace_if(...)   malc_stripped_entry()
#define malc_trace_i(...)    malc_stripped_entry()
#define malc_trace_i_if(...) malc_stripped_entry()

#endif /* MALC_STRIP_LOG_TRACE */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_NOTICE

#define malc_notice(...)\
  MALC_LOG_PRIVATE( \
    MALC_GET_LOGGER_INSTANCE_CALL, MALC_SEV_WRAP (note), __VA_ARGS__ \
    )

#define malc_notice_if(condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    MALC_GET_LOGGER_INSTANCE_CALL, \
    MALC_SEV_WRAP (note), \
    __VA_ARGS__\
    )

/* _i = instance, where the instance is provided manually instead of by calling
MALC_GET_LOGGER_INSTANCE_CALL */
#define malc_notice_i(malcref, ...)\
  MALC_LOG_PRIVATE ((malcref), MALC_SEV_WRAP (note), __VA_ARGS__)

#define malc_notice_i_if(condition, malcref, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (malcref), MALC_SEV_WRAP (note), __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_NOTICE */

#define malc_notice(...)      malc_stripped_entry()
#define malc_notice_if(...)   malc_stripped_entry()
#define malc_notice_i(...)    malc_stripped_entry()
#define malc_notice_i_if(...) malc_stripped_entry()

#endif /* MALC_STRIP_LOG_NOTICE */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_WARNING

#define malc_warning(...)\
  MALC_LOG_PRIVATE( \
    MALC_GET_LOGGER_INSTANCE_CALL, MALC_SEV_WRAP (warning), __VA_ARGS__ \
    )

#define malc_warning_if(condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    MALC_GET_LOGGER_INSTANCE_CALL, \
    MALC_SEV_WRAP (warning), \
    __VA_ARGS__\
    )

/* _i = instance, where the instance is provided manually instead of by calling
MALC_GET_LOGGER_INSTANCE_CALL */
#define malc_warning_i(malcref, ...)\
  MALC_LOG_PRIVATE ((malcref), MALC_SEV_WRAP (warning), __VA_ARGS__)

#define malc_warning_i_if(condition, malcref, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (malcref), MALC_SEV_WRAP (warning), __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_WARNING */

#define malc_warning(...)      malc_stripped_entry()
#define malc_warning_if(...)   malc_stripped_entry()
#define malc_warning_i(...)    malc_stripped_entry()
#define malc_warning_i_if(...) malc_stripped_entry()

#endif /* MALC_STRIP_LOG_WARNING */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_ERROR

#define malc_error(...)\
  MALC_LOG_PRIVATE( \
    MALC_GET_LOGGER_INSTANCE_CALL, MALC_SEV_WRAP (error), __VA_ARGS__ \
    )

#define malc_error_if(condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    MALC_GET_LOGGER_INSTANCE_CALL, \
    MALC_SEV_WRAP (error), \
    __VA_ARGS__\
    )

/* _i = instance, where the instance is provided manually instead of by calling
MALC_GET_LOGGER_INSTANCE_CALL */

#define malc_error_i(malcref, ...)\
  MALC_LOG_PRIVATE ((malcref), MALC_SEV_WRAP (error), __VA_ARGS__)

#define malc_error_i_if(condition, malcref, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (malcref), MALC_SEV_WRAP (error), __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_ERROR */

#define malc_error(...)      malc_stripped_entry()
#define malc_error_if(...)   malc_stripped_entry()
#define malc_error_i(...)    malc_stripped_entry()
#define malc_error_i_if(...) malc_stripped_entry()

#endif /* MALC_STRIP_LOG_ERROR */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_CRITICAL

#define malc_critical(...)\
  MALC_LOG_PRIVATE( \
    MALC_GET_LOGGER_INSTANCE_CALL, MALC_SEV_WRAP (critical), __VA_ARGS__ \
    )

#define malc_critical_if(condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    MALC_GET_LOGGER_INSTANCE_CALL, \
    MALC_SEV_WRAP (critical), \
    __VA_ARGS__\
    )

/* _i = instance, where the instance is provided manually instead of by calling
MALC_GET_LOGGER_INSTANCE_CALL */
#define malc_critical_i(malcref, ...)\
  MALC_LOG_PRIVATE ((malcref), MALC_SEV_WRAP (critical), __VA_ARGS__)

#define malc_critical_i_if(condition, malcref, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (malcref), MALC_SEV_WRAP (critical), __VA_ARGS__\
    )

#else /*MALC_STRIP_LOG_CRITICAL*/

#define malc_critical(...)      malc_stripped_entry()
#define malc_critical_if(...)   malc_stripped_entry()
#define malc_critical_i(...)    malc_stripped_entry()
#define malc_critical_i_if(...) malc_stripped_entry()

#endif /*MALC_STRIP_LOG_CRITICAL*/

/*----------------------------------------------------------------------------*/
#if !defined (MALC_NO_SHORT_LOG_MACROS)

#define log_debug(...)    malc_debug    (__VA_ARGS__)
#define log_trace(...)    malc_trace    (__VA_ARGS__)
#define log_notice(...)   malc_notice   (__VA_ARGS__)
#define log_warning(...)  malc_warning  (__VA_ARGS__)
#define log_error(...)    malc_error    (__VA_ARGS__)
#define log_critical(...) malc_critical (__VA_ARGS__)

#define log_debug_i(malcref, ...)\
  malc_debug_i    ((malcref), __VA_ARGS__)

#define log_trace_i(malcref, ...)\
  malc_trace_i    ((malcref), __VA_ARGS__)

#define log_notice_i(malcref, ...)\
  malc_notice_i   ((malcref), __VA_ARGS__)

#define log_warning_i(malcref, ...)\
  malc_warning_i  ((malcref), __VA_ARGS__)

#define log_error_i(malcref, ...)\
  malc_error_i    ((malcref), __VA_ARGS__)

#define log_critical_i(malcref, ...)\
  malc_critical_i ((malcref), __VA_ARGS__)

#define log_debug_if(condition, ...)\
  malc_debug_if    ((condition), __VA_ARGS__)

#define log_trace_if(condition, ...)\
  malc_trace_if    ((condition), __VA_ARGS__)

#define log_notice_if(condition, ...)\
  malc_notice_if   ((condition), __VA_ARGS__)

#define log_warning_if(condition, ...)\
  malc_warning_if  ((condition), __VA_ARGS__)

#define log_error_if(condition, ...)\
  malc_error_if    ((condition), __VA_ARGS__)

#define log_critical_if(condition, ...)\
  malc_critical_if ((condition), __VA_ARGS__)

#define log_debug_i_if(condition, malcref, ...)\
  malc_debug_i_if    ((condition), (malcref), __VA_ARGS__)

#define log_trace_i_if(condition, malcref, ...)\
  malc_trace_i_if    ((condition), (malcref), __VA_ARGS__)

#define log_notice_i_if(condition, malcref, ...)\
  malc_notice_i_if   ((condition), (malcref), __VA_ARGS__)

#define log_warning_i_if(condition, malcref, ...)\
  malc_warning_i_if  ((condition), (malcref), __VA_ARGS__)

#define log_error_i_if(condition, malcref, ...)\
  malc_error_i_if    ((condition), (malcref), __VA_ARGS__)

#define log_critical_i_if(condition, malcref, ...)\
  malc_critical_i_if ((condition), (malcref), __VA_ARGS__)

#define log_fileline malc_fileline

#endif /*#if !defined (MALC_NO_SHORT_MACROS)*/

#endif /* MALC_LOG_MACROS */

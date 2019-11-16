#ifndef __MALC_USER_MACROS_H__
#define __MALC_USER_MACROS_H__

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
#if !defined (MALC_GET_LOGGER_INSTANCE_FUNCNAME)
    #define MALC_GET_LOGGER_INSTANCE_FUNC get_malc_logger_instance()
#else
    #define MALC_GET_LOGGER_INSTANCE_FUNC MALC_GET_LOGGER_INSTANCE_FUNCNAME()
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
static inline bl_err malc_warning_silencer() { return bl_mkok(); }
/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_DEBUG

#define malc_debug(err, ...)\
  MALC_LOG_PRIVATE( \
    (err), MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_debug, __VA_ARGS__ \
    )

#define malc_debug_if(condition, err, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    (err), \
    MALC_GET_LOGGER_INSTANCE_FUNC, \
    malc_sev_debug, \
    __VA_ARGS__\
    )

#define malc_debug_i(err, malc_ptr, ...)\
  MALC_LOG_PRIVATE ((err), (malc_ptr), malc_sev_debug, __VA_ARGS__)

#define malc_debug_i_if(err, malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (err), (malc_ptr), malc_sev_debug, __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_DEBUG */

#define malc_debug(...)      malc_warning_silencer()
#define malc_debug_if(...)   malc_warning_silencer()
#define malc_debug_i(...)    malc_warning_silencer()
#define malc_debug_i_if(...) malc_warning_silencer()

#endif /* MALC_STRIP_LOG_DEBUG */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_TRACE

#define malc_trace(err, ...)\
  MALC_LOG_PRIVATE( \
    (err), MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_trace, __VA_ARGS__ \
    )

#define malc_trace_if(err, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    (err), \
    MALC_GET_LOGGER_INSTANCE_FUNC, \
    malc_sev_trace, \
    __VA_ARGS__\
    )

#define malc_trace_i(err, malc_ptr, ...)\
  MALC_LOG_PRIVATE ((err), (malc_ptr), malc_sev_trace, __VA_ARGS__)

#define malc_trace_i_if(err, malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (err), (malc_ptr), malc_sev_trace, __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_TRACE */

#define malc_trace(...)      malc_warning_silencer()
#define malc_trace_if(...)   malc_warning_silencer()
#define malc_trace_i(...)    malc_warning_silencer()
#define malc_trace_i_if(...) malc_warning_silencer()

#endif /* MALC_STRIP_LOG_TRACE */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_NOTICE

#define malc_notice(err, ...)\
  MALC_LOG_PRIVATE( \
    (err), MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_note, __VA_ARGS__ \
    )

#define malc_notice_if(err, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    (err), \
    MALC_GET_LOGGER_INSTANCE_FUNC, \
    malc_sev_note, \
    __VA_ARGS__\
    )

#define malc_notice_i(err, malc_ptr, ...)\
  MALC_LOG_PRIVATE ((err), (malc_ptr), malc_sev_note, __VA_ARGS__)

#define malc_notice_i_if(err, malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (err), (malc_ptr), malc_sev_note, __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_NOTICE */

#define malc_notice(...)      malc_warning_silencer()
#define malc_notice_if(...)   malc_warning_silencer()
#define malc_notice_i(...)    malc_warning_silencer()
#define malc_notice_i_if(...) malc_warning_silencer()

#endif /* MALC_STRIP_LOG_NOTICE */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_WARNING

#define malc_warning(err, ...)\
  MALC_LOG_PRIVATE( \
    (err), MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_warning, __VA_ARGS__ \
    )

#define malc_warning_if(err, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    (err), \
    MALC_GET_LOGGER_INSTANCE_FUNC, \
    malc_sev_warning, \
    __VA_ARGS__\
    )

#define malc_warning_i(err, malc_ptr, ...)\
  MALC_LOG_PRIVATE ((err), (malc_ptr), malc_sev_warning, __VA_ARGS__)

#define malc_warning_i_if(err, malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (err), (malc_ptr), malc_sev_warning, __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_WARNING */

#define malc_warning(...)      malc_warning_silencer()
#define malc_warning_if(...)   malc_warning_silencer()
#define malc_warning_i(...)    malc_warning_silencer()
#define malc_warning_i_if(...) malc_warning_silencer()

#endif /* MALC_STRIP_LOG_WARNING */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_ERROR

#define malc_error(err, ...)\
  MALC_LOG_PRIVATE( \
    (err), MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_error, __VA_ARGS__ \
    )

#define malc_error_if(err, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    (err), \
    MALC_GET_LOGGER_INSTANCE_FUNC, \
    malc_sev_error, \
    __VA_ARGS__\
    )

#define malc_error_i(err, malc_ptr, ...)\
  MALC_LOG_PRIVATE ((err), (malc_ptr), malc_sev_error, __VA_ARGS__)

#define malc_error_i_if(err, malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (err), (malc_ptr), malc_sev_error, __VA_ARGS__\
    )

#else /* MALC_STRIP_LOG_ERROR */

#define malc_error(...)      malc_warning_silencer()
#define malc_error_if(...)   malc_warning_silencer()
#define malc_error_i(...)    malc_warning_silencer()
#define malc_error_i_if(...) malc_warning_silencer()

#endif /* MALC_STRIP_LOG_ERROR */

/*----------------------------------------------------------------------------*/
#ifndef MALC_STRIP_LOG_CRITICAL

#define malc_critical(err, ...)\
  MALC_LOG_PRIVATE( \
    (err), MALC_GET_LOGGER_INSTANCE_FUNC, malc_sev_critical, __VA_ARGS__ \
    )

#define malc_critical_if(err, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), \
    (err), \
    MALC_GET_LOGGER_INSTANCE_FUNC, \
    malc_sev_critical, \
    __VA_ARGS__\
    )

#define malc_critical_i(err, malc_ptr, ...)\
  MALC_LOG_PRIVATE ((err), (malc_ptr), malc_sev_critical, __VA_ARGS__)

#define malc_critical_i_if(err, malc_ptr, condition, ...)\
  MALC_LOG_IF_PRIVATE(\
    (condition), (err), (malc_ptr), malc_sev_critical, __VA_ARGS__\
    )

#else /*MALC_STRIP_LOG_CRITICAL*/

#define malc_critical(...)      malc_warning_silencer()
#define malc_critical_if(...)   malc_warning_silencer()
#define malc_critical_i(...)    malc_warning_silencer()
#define malc_critical_i_if(...) malc_warning_silencer()

#endif /*MALC_STRIP_LOG_CRITICAL*/

/*----------------------------------------------------------------------------*/
#if !defined (MALC_NO_SHORT_LOG_MACROS)

#define log_debug(err, ...)    malc_debug    ((err), __VA_ARGS__)
#define log_trace(err, ...)    malc_trace    ((err), __VA_ARGS__)
#define log_notice(err, ...)   malc_notice   ((err), __VA_ARGS__)
#define log_warning(err, ...)  malc_warning  ((err), __VA_ARGS__)
#define log_error(err, ...)    malc_error    ((err), __VA_ARGS__)
#define log_critical(err, ...) malc_critical ((err), __VA_ARGS__)

#define log_debug_i(err, malc_ptr, ...)\
  malc_debug_i    ((err), (malc_ptr), __VA_ARGS__)

#define log_trace_i(err, malc_ptr, ...)\
  malc_trace_i    ((err), (malc_ptr), __VA_ARGS__)

#define log_notice_i(err, malc_ptr, ...)\
  malc_notice_i   ((err), (malc_ptr), __VA_ARGS__)

#define log_warning_i(err, malc_ptr, ...)\
  malc_warning_i  ((err), (malc_ptr), __VA_ARGS__)

#define log_error_i(err, malc_ptr, ...)\
  malc_error_i    ((err), (malc_ptr), __VA_ARGS__)

#define log_critical_i(err, malc_ptr, ...)\
  malc_critical_i ((err), (malc_ptr), __VA_ARGS__)

#define log_debug_if(condition, err, ...)\
  malc_debug_if    ((condition), (err), __VA_ARGS__)

#define log_trace_if(condition, err, ...)\
  malc_trace_if    ((condition), (err), __VA_ARGS__)

#define log_notice_if(condition, err, ...)\
  malc_notice_if   ((condition), (err), __VA_ARGS__)

#define log_warning_if(condition, err, ...)\
  malc_warning_if  ((condition), (err), __VA_ARGS__)

#define log_error_if(condition, err, ...)\
  malc_error_if    ((condition), (err), __VA_ARGS__)

#define log_critical_if(condition, err, ...)\
  malc_critical_if ((condition), (err), __VA_ARGS__)

#define log_debug_i_if(condition, err, malc_ptr, ...)\
  malc_debug_i_if    ((condition), (err), (malc_ptr), __VA_ARGS__)

#define log_trace_i_if(condition, err, malc_ptr, ...)\
  malc_trace_i_if    ((condition), (err), (malc_ptr), __VA_ARGS__)

#define log_notice_i_if(condition, err, malc_ptr, ...)\
  malc_notice_i_if   ((condition), (err), (malc_ptr), __VA_ARGS__)

#define log_warning_i_if(condition, err, malc_ptr, ...)\
  malc_warning_i_if  ((condition), (err), (malc_ptr), __VA_ARGS__)

#define log_error_i_if(condition, err, malc_ptr, ...)\
  malc_error_i_if    ((condition), (err), (malc_ptr), __VA_ARGS__)

#define log_critical_i_if(condition, err, malc_ptr, ...)\
  malc_critical_i_if ((condition), (err), (malc_ptr), __VA_ARGS__)

#define log_fileline malc_fileline

#endif /*#if !defined (MALC_NO_SHORT_MACROS)*/

#endif /* MALC_LOG_MACROS */

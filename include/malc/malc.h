#ifndef __MALC_H__
#define __MALC_H__

#include <bl/base/integer.h>
#include <bl/base/error.h>
#include <bl/base/assert.h>
#include <bl/base/alloc.h>
#include <bl/base/thread.h>

#include <malc/libexport.h>
#include <malc/cfg.h>
#include <malc/destination.h>
#include <malc/impl/malc_private.h>

struct malc;
typedef struct malc malc;

enum malc_destinations {
  malc_dst_file,
  malc_dst_console,
}
malc_destinations;

/*----------------------------------------------------------------------------*/
extern MALC_EXPORT uword malc_get_size (void);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_create (malc** l, alloc_tbl const* tbl);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_destroy (malc* l);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_get_cfg (malc* l, malc_cfg* cfg);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_init (malc* l, malc_cfg const* cfg);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_terminate (malc* l);
/*------------------------------------------------------------------------------
Each application controlled thread has to initialize its thread local storage
buffer explicitly, as there is no way in C to unexplicitly initialize thread local
resources from the logger when a thread is launched. The good thing is that
each thread can have a different buffer size.
------------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_producer_thread_local_init (malc* l, u32 bytes);
/*------------------------------------------------------------------------------
timeout_us: timeout to block before returning. 0 just runs one iteration.

returns bl_ok:            Consumer not in idle-state.
        bl_nothing_to_do: Consumer not on idle-state.
        bl_locked:        Terminated.
------------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_run_consume_task (malc* l, uword timeout_us);
/*------------------------------------------------------------------------------
force: forces running the idle task even if it wasn't its time.

returns bl_ok:
        bl_nothing_to_do: Consumer not on idle-state.
        bl_locked:        Terminated.
------------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_run_idle_task (malc* l, bool force);
/*------------------------------------------------------------------------------
log destination can only be added before initializing
------------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_add_destination(
  malc* l, u32* dst_id, malc_dst const* dst
  );
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_get_destination(
  malc* l, u32 dst_id, malc_dst* dst, void* instance
  );
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_set_destination_severity(
  malc* l, u32 dst_id, u8 severity
  );
/*------------------------------------------------------------------------------
Passes a literal to mal log. By using this function you are saying to the logger
that this string will outlive the data logger. These is to be mainly used to
pass literals by pointer (no deep copy) but it can be used for dynamic strings
that will outlive the data logger too.
------------------------------------------------------------------------------*/
static inline malc_lit loglit (char const* literal)
{
  bl_assert (literal);
  malc_lit l = { literal };
  return l;
}
/*------------------------------------------------------------------------------
Passes a string by value (deep copy) to mal log.
------------------------------------------------------------------------------*/
static inline malc_mem logstr (char const* str, u16 len)
{
  bl_assert ((str && len) || len == 0);
  malc_mem s = { str, len };
  return s;
}
/*------------------------------------------------------------------------------
Passes a memory area by value (deep copy) to mal log. It will be printed as hex.
------------------------------------------------------------------------------*/
static inline malc_mem logmem (u8 const* mem, u16 size)
{
  bl_assert ((mem && size) || size == 0);
  malc_mem b = { mem, size };
  return b;
}
/*----------------------------------------------------------------------------*/
#if !defined (MALC_NO_SHORT_MACROS)

#define log_debug(...)    MALC_DEBUG    (__VA_ARGS__)
#define log_trace(...)    MALC_TRACE    (__VA_ARGS__)
#define log_notice(...)   MALC_NOTICE   (__VA_ARGS__)
#define log_warning(...)  MALC_WARNING  (__VA_ARGS__)
#define log_error(...)    MALC_ERROR    (__VA_ARGS__)
#define log_critical(...) MALC_CRITICAL (__VA_ARGS__)

#define log_debug_i(malc_ptr, ...)    MALC_DEBUG_I    ((malc_ptr), __VA_ARGS__)
#define log_trace_i(malc_ptr, ...)    MALC_TRACE_I    ((malc_ptr), __VA_ARGS__)
#define log_notice_i(malc_ptr, ...)   MALC_NOTICE_I   ((malc_ptr), __VA_ARGS__)
#define log_warning_i(malc_ptr, ...)  MALC_WARNING_I  ((malc_ptr), __VA_ARGS__)
#define log_error_i(malc_ptr, ...)    MALC_ERROR_I    ((malc_ptr), __VA_ARGS__)
#define log_critical_i(malc_ptr, ...) MALC_CRITICAL_I ((malc_ptr), __VA_ARGS__)

#define log_debug_if(condition, ...)\
  MALC_DEBUG_IF    ((condition), __VA_ARGS__)

#define log_trace_if(condition, ...)\
  MALC_TRACE_IF    ((condition), __VA_ARGS__)

#define log_notice_if(condition, ...)\
  MALC_NOTICE_IF   ((condition), __VA_ARGS__)

#define log_warning_if(condition, ...)\
  MALC_WARNING_IF  ((condition), __VA_ARGS__)

#define log_error_if(condition, ...)\
  MALC_ERROR_IF    ((condition), __VA_ARGS__)

#define log_critical_if(condition, ...)\
  MALC_CRITICAL_IF ((condition), __VA_ARGS__)

#define log_debug_i_if(malc_ptr, condition, ...)\
  MALC_DEBUG_I_IF    ((malc_ptr), (condition), __VA_ARGS__)

#define log_trace_i_if(malc_ptr, condition, ...)\
  MALC_TRACE_I_IF    ((malc_ptr), (condition), __VA_ARGS__)

#define log_notice_i_if(malc_ptr, condition, ...)\
  MALC_NOTICE_I_IF   ((malc_ptr), (condition), __VA_ARGS__)

#define log_warning_i_if(malc_ptr, condition, ...)\
  MALC_WARNING_I_IF  ((malc_ptr), (condition), __VA_ARGS__)

#define log_error_i_if(malc_ptr, condition, ...)\
  MALC_ERROR_I_IF    ((malc_ptr), (condition), __VA_ARGS__)

#define log_critical_i_if(malc_ptr, condition, ...)\
  MALC_CRITICAL_I_IF ((malc_ptr), (condition), __VA_ARGS__)

#endif /*#if !defined (MALC_NO_SHORT_MACROS)*/

#endif /* __MALC_H__ */

#ifndef __MALC_H__
#define __MALC_H__

#include <string.h>

#include <bl/base/integer.h>
#include <bl/base/error.h>
#include <bl/base/assert.h>
#include <bl/base/allocator.h>
#include <bl/base/thread.h>

#ifndef MALC_LIBRARY_COMPILATION
  #include <malc/config.h>
#endif

/* has to be the first, defines preprocessor macros */
#include <malc/impl/malc_private.h>
#include <malc/libexport.h>
#include <malc/cfg.h>
#include <malc/destination.h>
#include <malc/impl/serializer.h>

struct malc;
typedef struct malc malc;

enum malc_destinations {
  malc_dst_file,
  malc_dst_console,
}
malc_destinations;

/* Usage:
#if MALC_VERSION <= MALC_VERSION_GET (1, 0, 0)
  Do something for versions LE than 1.0.0
#endif
*/
#define MALC_VERSION_GET(major,minor,rev) \
  (major * 1000000 + minor * 1000 + rev)

#define MALC_VERSION \
  MALC_VERSION_GET(MALC_VERSION_MAJOR, MALC_VERSION_MINOR, MALC_VERSION_REV)

#ifdef __cplusplus
  extern "C" {
#endif
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT uword malc_get_size (void);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_create (malc* l, alloc_tbl const* tbl);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_destroy (malc* l);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_get_cfg (malc const* l, malc_cfg* cfg);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_init (malc* l, malc_cfg const* cfg);
/*------------------------------------------------------------------------------
Sends a flush command message to the logger queue and waits until it is dequeued
and processed (all destinations have been flushed). As this call is blocking,
when this call unblocks all the messages logged from this thread have already
been processed by the consumer task.

If the consume task is not being called for some reason (bug in user code) it
will block forever.
------------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_flush (malc* l);
/*------------------------------------------------------------------------------
Prepares the logger for termination. After this is invoked the consumer task
("malc_run_consume_task") will start the necessary steps to cleanly shutdown the
logger and will eventually return "bl_preconditions" when the logger instance is
in the terminated state.

After calling this function no further log messages can be enqueued by any
thread.

The "is_consume_task_thread" bool parameter needs to be set to "true" when the
thread doing the termination is the same one that runs "malc_run_consume_task".
This is because in such case the command can't block until the consumer task
processes the termination command (it would deadlock). In this case the
termination is complete when "malc_run_consume_task" returns "bl_preconditions".

If "is_consume_task_thread" is "false" this call will block until the
termination command has been processed by the consume task.

 "malc_destroy" will only succeed when the consumer task is in the terminated
 state.
------------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_terminate (malc* l, bool is_consume_task_thread);
/*------------------------------------------------------------------------------
Activates the thread local buffer for the caller thread. Using this buffer makes
the logger the fastest it can be. Logging becomes wait-free (as long as there
is room on the queue).

Each thread has to initialize its thread local storage buffer manually, this is
done for three reasons:

1.When writing a library using this logger, it may be unnecessary and wasteful
  to place thread local resources on threads that are not going to use it.

2.Threads activating the thread local buffer can _never_ outlive the data
  logger, they have to be "join"ed before calling "malc_terminate"(*): when a
  thread goes out of scope its TLS/TSS destructor automatically sends the
  buffer memory to the consumer task as a deallocation command. Its memory is
  deallocated from the consume task thread to ensure that the TLS buffer isn't
  deallocated before all the entries of the now-dying thread have been
  processed.

  It is very likely that the user that manually calls this function has full
  control of the thread and can comply with this requirement. If not there are
  other log entry allocation methods available.

  (*) Note that the thread that runs the "malc_terminate" function  doesn't need
  to be joined. The "malc_terminate" function deallocates the thread local
  storage buffer manually.

3.There is no straightforward way to do it on C (which is a good thing, I
  would't do it anyways for the two reasons above).

Note that when using thread local storage (per-thread global variables) it's
only possible to use one library instance on each thread. A thread logging to
two memory instances will corrupt both loggers. This is unfixable as it is a
limitation of Thread Local Storage.
------------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_producer_thread_local_init (malc* l, u32 bytes);
/*------------------------------------------------------------------------------
timeout_us: timeout to block before returning. 0 just runs one iteration.

returns bl_ok:            Consumer not in idle-state.
        bl_nothing_to_do: Consumer in idle-state.
        bl_preconditions: Malc library not ready to run (no init, terminated...)
------------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_run_consume_task (malc* l, uword timeout_us);
/*------------------------------------------------------------------------------
Adds a destination. Can only be added before initializing (calling "malc_init").

If run-time modifications are done to the instance/object, keep in mind thread
safety issues of variables used by the functions pointed by the "malc_dst"
table.
------------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_add_destination(
  malc* l, u32* dest_id, malc_dst const* dst
  );
/*------------------------------------------------------------------------------
Gets the allocated and initialized instance of a logger destination. E.g:

  #include <malc/malc.h>
  #include <malc/destinations/stdouterr.h>

  malc* l = ...;
  u32   id;
  (void) malc_add_destination (l, &id, &malc_stdouterr_dst_tbl);
  malc_stdouterr_dst* dst;
  (void) malc_get_destination_instance (l (void**) &dst, id);
  malc_stdouterr_set_stderr_severity (dst, malc_sev_warning);

Keep in mind that adding more destinations can make this destination instance to
be reallocated elsewhere, so storing the pointer value is dangerous. It
is safe to separately store the pointer when all the destinations have been
added.
------------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_get_destination_instance(
  malc const* l, void** instance, u32 dest_id
  );
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_get_destination_cfg(
  malc const* l, malc_dst_cfg* cfg, u32 dest_id
  );
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_set_destination_cfg(
  malc* l, malc_dst_cfg const* cfg, u32 dest_id
  );
/*------------------------------------------------------------------------------
Passes a literal to malc. By using this function you tell the logger that
this string will outlive the data logger and doesn't need to be copied, it can
be taken by reference/pointer.

This is mainly intended to be used for passing string literals, but it can be
used (with caution) for non-literal strings that are known to outlive the data
logger unmodified.
------------------------------------------------------------------------------*/
static inline malc_lit loglit (char const* literal)
{
  bl_assert (literal);
  malc_lit l = { literal };
  return l;
}
/*------------------------------------------------------------------------------
Passes a string by value (deep copy) to malc.
------------------------------------------------------------------------------*/
static inline malc_strcp logstrcpy (char const* str, u16 len)
{
  bl_assert ((str && len) || len == 0);
  malc_strcp s = { str, len };
  return s;
}
/*----------------------------------------------------------------------------*/
static inline malc_strcp logstrcpyl (char const* str)
{
  uword len = strlen (str);
  return logstrcpy (str, (u16) (len < 65536 ? len : 65535));
}
/*------------------------------------------------------------------------------
Passes a memory area by value (deep copy) to malc. It will be printed as hex.
------------------------------------------------------------------------------*/
static inline malc_memcp logmemcpy (void const* mem, u16 size)
{
  bl_assert ((mem && size) || size == 0);
  malc_memcp b = { (u8 const*) mem, size };
  return b;
}
/*------------------------------------------------------------------------------
Passes a string by reference to malc. Needs that you provide a destructor for
this dynamic string as the last parameter on the call (see "logrefdtor"). To be
used to avoid copying big chunks of data.
------------------------------------------------------------------------------*/
static inline malc_strref logstrref (char const* str, u16 len)
{
  bl_assert ((str && len) || len == 0);
  malc_strref s = { str, len };
  return s;
}
/*----------------------------------------------------------------------------*/
static inline malc_strref logstrrefl (char const* str)
{
  uword len = strlen (str);
  return logstrref (str, (u16) (len < 65536 ? len : 65535));
}
/*------------------------------------------------------------------------------
Passes a memory area by reference to malc. Needs that you provide a destructor
for this memory area as the last parameter on the call (see "logrefdtor"), To be
used to avoid copying big chunks of data.
------------------------------------------------------------------------------*/
static inline malc_memref logmemref (void const* mem, u16 size)
{
  bl_assert ((mem && size) || size == 0);
  malc_memref b = { (u8 const*) mem, size };
  return b;
}
/*------------------------------------------------------------------------------
typedef void (*malc_refdtor_fn)(
  void* context, malc_ref const* refs, uword refs_count
  );

Passed by reference parameter destructor.

This is mandatory to be used as the last parameter on log calls that contain a
parameter passed by reference ("logstrref" or "logmemref"). It does make the
consumer (logger) thread to invoke a callback that can be used to do memory
cleanup/recycling/reference decreasing etc.

This means that blocking on the callback will block the logger consumer thread,
so callbacks should be kept short, ideally doing an atomic reference count
decrement, a deallocation or something similar. Thread safety issues should be
considered too.

Note that in case of error or a filtered out severity the destructor will be run
in-place by the calling thread. This can make the feature unusable for some use
cases, e.g. when the deallocation is costly and blocking and the calling thread
has to progress fast.

The copy by value functions don't add any extra operations on failure or
filtering out, so they can be considered an alternative for when the
deallocation behavior described above is a problem.
------------------------------------------------------------------------------*/
static inline malc_refdtor logrefdtor (malc_refdtor_fn func, void* context)
{
  malc_refdtor r = { func, context };
  return r;
}
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

#ifdef __cplusplus
  } /* extern "C" { */
#endif

#endif /* __MALC_H__ */

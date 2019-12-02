/* this header is used to include structures in both C and C++ */

#if (!defined (__MALC_COMMON_H__) && !defined (MALC_COMMON_NAMESPACED)) || \
    (!defined (__MALC_COMMON_CPP_H__) && defined (MALC_COMMON_NAMESPACED))

#ifdef MALC_COMMON_NAMESPACED
#define __MALC_COMMON_CPP_H__
#else
#define __MALC_COMMON_H__
#endif

/* user-facing macros and data types */

#include <bl/base/integer.h>
#include <bl/base/error.h>
#include <bl/base/allocator.h>

#ifdef MALC_COMMON_NAMESPACED
namespace malcpp {
#endif

/*----------------------------------------------------------------------------*/
/* Version */
/*----------------------------------------------------------------------------*/
/* Usage:
#if MALC_VERSION <= MALC_VERSION_GET (1, 0, 0)
  Do something for versions LE than 1.0.0
#endif
*/
/*----------------------------------------------------------------------------*/
#define MALC_VERSION_GET(major,minor,rev) \
  (major * 1000000 + minor * 1000 + rev)

#define MALC_VERSION \
  MALC_VERSION_GET(MALC_VERSION_MAJOR, MALC_VERSION_MINOR, MALC_VERSION_REV)
/*----------------------------------------------------------------------------*/
enum malc_severities {
  malc_sev_debug    = '3',
  malc_sev_trace    = '4',
  malc_sev_note     = '5',
  malc_sev_warning  = '6',
  malc_sev_error    = '7',
  malc_sev_critical = '8',
  malc_sev_off      = '9',
};
/*----------------------------------------------------------------------------*/
static inline bool malc_is_valid_severity (bl_uword sev)
{
  return (sev >= malc_sev_debug) && (sev <= malc_sev_critical);
}
/*----------------------------------------------------------------------------*/
/* MALC configuration C structs */
/*------------------------------------------------------------------------------
idle_task_period_us:

  The IDLE task (both internal and on all the destinations) will be run with
  this periodicity. The IDLE task is used to do maintenance operations that can
  be done outside of the fast path.

backoff_max_us:

  Internal fine-tuning. Leave as default or read the sources before setting
  this.

start_own_thread:

  When this is set "malc" will launch and manage a dedicated thread for the
  consumer. If this is unset the logger's consume task is run manually from a
  user thread by using "malc_run_consume_task".
------------------------------------------------------------------------------*/
typedef struct malc_consumer_cfg {
  bl_u32 idle_task_period_us;
  bl_u32 backoff_max_us;
  bool   start_own_thread;
}
malc_consumer_cfg;

/*------------------------------------------------------------------------------
timestamp:

  Timestamp at the producer side. It's slower but more precise. In general if
  you can't tolerate ~10ms jitter on the logging timestamp you should set this
  at the expense of performance.
------------------------------------------------------------------------------*/
typedef struct malc_producer_cfg {
  bool timestamp;
}
malc_producer_cfg;
/*------------------------------------------------------------------------------
msg_allocator:

  Each producer may have an own memory buffer in TLS (Thread Local Storage) and
  a fixed-size shared memory buffer between all the threads, when both are
  exhausted (or disabled) the pointed allocator will be used. Note that this
  allocator is only used to enqueue log entries, the  memory required by the
  logger's internals is taken from the allocator passed on "malc_init".

slot_size:

  All allocations for the producer's consumer queue are rounded to the ceiling
  against this value. Usually this is intended to be set as the size in bytes
  of your machine's cache line.

fixed_allocator_bytes:

  The minimal amount of bytes that each fixed allocator will have. This number
  might be rounded up to cache line size or to the next power of 2. 0 disables
  the fixed allocator.

fixed_allocator_max_slots:

  The maximum amount of slots that a single call to the fixed allocator can
  have. The slot size is unspecified so this parameter is a bit vague. Consider
  a slot size equal or very near the cache line size.

  Note that the current fixed allocator implementation has unavoidable
  unfairness; when in contention smaller slot sizes have more probabilities to
  win the contention, starving bigger slot size allocations.

fixed_allocator_per_cpu:

  Create one fixed allocator for each CPU core. This is an optimization (or
  pessimization: measure your performance) to alleviate false sharing. Note that
  "fixed_allocator_bytes" is not divided by the number of CPUs when this setting
  this active, it's still the size of each allocator.

------------------------------------------------------------------------------*/
typedef struct malc_alloc_cfg {
  bl_alloc_tbl const* msg_allocator;
  bl_u32              slot_size;
  bl_u32              fixed_allocator_bytes;
  bl_u32              fixed_allocator_max_slots;
  bool                fixed_allocator_per_cpu;
}
malc_alloc_cfg;
/*------------------------------------------------------------------------------
sanitize_log_entries:

  The log entries are removed from any character that may make them to be
  confused with another log line, e.g. newline, so log injection is harder.

log_rate_filter_watch_count:

  Controls how many different log entries can be watched simultaneously for
  repeated log messages by the log rate filter. This number may be limited to be
  very low (64), as calculating the data rate of all the previous entries has
  performance implications. 0 disables the filter.

log_rate_filter_min_severity:

  Controls the lowest severity affected by the log_rate_filter. This is intended
  to let the user decide if the lowest severitites should be filtered or not, as
  these can be used for debugging and stripped from the release executable.

  Both this and is "log_rate_filter_watch_count" are to be used together with
  the per-destination (in struct "malc_dst_cfg") "log_rate_filter_time_ns"
  parameter.

------------------------------------------------------------------------------*/
typedef struct malc_security {
  bool   sanitize_log_entries;
  bl_u32 log_rate_filter_watch_count;
  bl_u32 log_rate_filter_min_severity;
}
malc_security;
/*----------------------------------------------------------------------------*/
typedef struct malc_cfg {
  malc_consumer_cfg consumer;
  malc_producer_cfg producer;
  malc_security     sec;
  malc_alloc_cfg    alloc;
}
malc_cfg;
/*----------------------------------------------------------------------------*/
/* Destination(sink) C structures */
/*------------------------------------------------------------------------------
timestamp:     timestamp string (from a monotonic clock).
timestamp_len: timestamp "strlen". Can be zero.
sev:           severity string
sev_len:       severity "strlen". Can be zero.
text:          log entry text. It won't contain an added trailing newline.
text_len   :   log entry "strlen". Can't be zero.
------------------------------------------------------------------------------*/
typedef struct malc_log_strings {
  char const* timestamp; /* from a monotonic clock */
  bl_uword    timestamp_len;
  char const* sev;
  bl_uword    sev_len;
  char const* text;
  bl_uword    text_len;
}
malc_log_strings;
/*------------------------------------------------------------------------------
log_rate_filter_time_ns:

  Rate filter period. The same log entry arriving before this time will be
  discarded. 0 = disabled.

  It protects against malicious or involuntary loops logging the same entry (or
  set of entries) with a very high rate. Which can force the logs to rotate and
  can lead to a potential loss of information, allowing an attacker to cover
  tracks.

  This is to be used in with the global "log_rate_filter_watch_count" and
  "log_rate_filter_min_severity" on the "malc_security_cfg" struct.

severity_file_path:

  null or a file (probably on a RAM filesystem) to read the severity from. If
  not null the logger will read that file periodically (on the IDLE taks) to
  update the logger severity. The file should contain one of the next values:

  -debug
  -trace
  -note
  -warning
  -error
  -critical

------------------------------------------------------------------------------*/
typedef struct malc_dst_cfg {
  bl_u64      log_rate_filter_time_ns;
  bool        show_timestamp;
  bool        show_severity;
  bl_u8       severity;
  char const* severity_file_path;
}
malc_dst_cfg;
/*------------------------------------------------------------------------------
malc_dst:

  Resource table of a malc destination. To be used to implement malc
  destinations (sinks) in C. There are wrappers on the C++ implemenmtations.

  All the functions here will be called non-concurrently (in a single
  threaded fashion) with guaranteed memory visibility (the thead may be swapped
  under the hood though). The field description follows.

size_of:

  "sizeof" the struct containing an instance of this destination. Malc will
  own (allocate and deallocate) the instance, so it needs to know the size of
  the memory to allocate.

init:

  Initialization: The instance parameter will contain a raw memory chunk of
  "size_of" bytes. "alloc" is a provided-by-malc memory allocator in case it's
  necessary for the instance to allocate more memory on the initialization
  phase. (it will be the same instance passed to malc on "malc_create"). The
  allocator can be stored by instance implementations for later usage. This
  function pointer can be set to null if there is no initialization to do.

terminate:

  Termination before memory deallocation. Its call will be triggered by
  "malc_terminate" (or "malc_destroy" when no "malc_terminate" call is made)
  Memory synchronization is placed afterwars by malc, so all the actions did
  inside the function will be visible to thread that calls "malc_terminate"
  or "malc_destroy"). This function pointer can be set to null if there is no
  termination to do.

flush:

  Flush all data. To be used in case of the "write" function being buffered.
  Will always be trigered by "malc_flush" and can be called in some other
  implementation defined states too (e.g. idle). This function pointer can be
  set to null if there is no flushing to do on this destination.

idle_task:

  Will be invoked when the logger is idle, e.g. to do low priority tasks. This
  function pointer can be set to null if there is no idle task.

write:

  Log write. The only mandatory function to implement. Writes data somewhere.

    nsec:     current monotonic clock timestamp, can be used internally.
    sev_val:  severity.
    strs: log strings.
------------------------------------------------------------------------------*/
typedef struct malc_dst {
  bl_uword size_of;
  bl_err (*init)      (void* instance, bl_alloc_tbl const* alloc);
  void   (*terminate) (void* instance);
  bl_err (*flush)     (void* instance);
  bl_err (*idle_task) (void* instance);
  bl_err (*write)(
    void* instance, bl_u64 nsec, bl_uword sev_val, malc_log_strings const* strs
    );
}
malc_dst;
/*------------------------------------------------------------------------------
Configuration struct for MALC's inbuilt file destination

prefix:
suffix:
time_based_name:

  The file names have this form when "time_based_name" is "true":
    "prefix"_"(system clock)"_"(monotonic clock)""suffix"

  Or this one when "time_based_name" is "false":
    "prefix"_"(number)""suffix"

  "prefix" can include a folder, but the folder won't be created, it has to
  exist before.

  "system clock" is the value of the system clock in nanoseconds coded as a
  64bit zero-filled hexadecimal number.

  "monotonic clock" is the value of a clock that always goes forward, is not
  affected by DST or wall clock time changes and starts at some arbitrary point
  of time. It is in nanoseconds units too, coded as a 64bit zero-filled
  hexadecimal number.

  All the log entries inside a file are relative to the monotonic clock.
  Expressed in seconds as floating point numbers.

  For a demo on how to convert the dates to calendar, see
  "scripts/malc-overview-date-converter.sh"

max_file_size:

  The approximate maximum log file size in KB. It will be rounded to contain
  full log entries. This size will never be surpassed except for files with a
  single log entry bigger than this size. 0 disables splitting log files in
  chunks.

max_log_files:

  The maximum number of log files to keep. Once this number is reached the log
  files will start rotating. Requires a non-zero "file_size". 0 disables log
  rotation.

can_remove_old_data_on_full_disk:

  If the disk is full the logger starts erasing either the oldest log files
  (when "max_log_files" is non zero) or it deletes the current log file when
  "max_log_files" is zero.
------------------------------------------------------------------------------*/
typedef struct malc_file_cfg {
  char const* prefix;
  char const* suffix;
  bool        time_based_name;
  bool        can_remove_old_data_on_full_disk;
  bl_uword    max_file_size;
  bl_uword    max_log_files;
}
malc_file_cfg;
/*------------------------------------------------------------------------------
type returned on "malc_refdtor_fn" when using reference types on the logger. It
is just a passed pointer of the memory address with the size that was passed.

"ref" is not const because free migth be called on it.
------------------------------------------------------------------------------*/
typedef struct malc_ref {
  void*  ref;
  bl_u16 size; /* !!! for string this is equal to "strlen", not "strlen + 1" */
}
malc_ref;
/*------------------------------------------------------------------------------
type returned on "malc_refdtor_fn" when using reference types on the logger. It
is just a passed pointer of the memory address with the size that was passed.
------------------------------------------------------------------------------*/
typedef void (*malc_refdtor_fn)(
  void* context, malc_ref const* refs, bl_uword refs_count
  );
/*------------------------------------------------------------------------------
functions for objects

Malc allows serializations of "objects" up to "MALC_OBJ_MAX_ALIGN" and
"MALC_OBJ_MAX_SIZE".
------------------------------------------------------------------------------*/
#define MALC_OBJ_MAX_ALIGN 256
#define MALC_OBJ_MAX_SIZE \
  ((1ull << bl_sizeof_member (malc_obj, obj_sizeof) * 8) - 1)
/*------------------------------------------------------------------------------
This is passed by the log functions. On the log function you select a callback
and if you log with context.
------------------------------------------------------------------------------*/
typedef struct malc_obj_ref {
  void* obj;
  union {
    void* context;
    bl_u8 flag;
  } extra;
}
malc_obj_ref;
/*------------------------------------------------------------------------------
types to return on "malc_obj_log_data.data.builtin.type" below.
------------------------------------------------------------------------------*/
typedef enum malc_obj_type_id {
  malc_obj_u8,
  malc_obj_u16,
  malc_obj_u32,
  malc_obj_u64,
  malc_obj_i8,
  malc_obj_i16,
  malc_obj_i32,
  malc_obj_i64,
  malc_obj_float,
  malc_obj_double,
}
malc_obj_type_id;
/*------------------------------------------------------------------------------
Returned from "malc_obj_get_data_fn", it returns a contiguous chunk that can
either be a string or a memory block (to be printed as hex).
------------------------------------------------------------------------------*/
typedef struct malc_obj_log_data {
  union {
    struct {
      char const* ptr;
      bl_u16      len;
    }
    str;
    struct {
      void const* ptr;
      bl_u8       count;
      bl_u8       type; /* malc_obj_i8 ... malc_obj_double ( ints + floats) */
    }
    builtin;
  }
  data;
  bl_u8 is_str;
}
malc_obj_log_data;
/*------------------------------------------------------------------------------
The function that returns data to print from the object.

It will be called in a loop for as long as "*iter_context" is not NULL.

The content of "*iter_context" will never modified by malc, so it can be used
to pass dynamically allocated data that has to be persistent between calls or
just be set to a non NULL value to signal malc to continue iterating.

If an internal malc error happens and "iter_context" is non-NULL,
"malc_obj_get_data_fn" will be called with "out" set to NULL to give the
function an oportunity to deallocate resources on "iter_context".

The "fmt" string contains the passed printf modifiers.
------------------------------------------------------------------------------*/
typedef void (*malc_obj_get_data_fn) (
  malc_obj_ref*      obj,
  malc_obj_log_data* out,
  void**             iter_context,
  char const*        fmt
  );
/*------------------------------------------------------------------------------
Object destructor
------------------------------------------------------------------------------*/
typedef void (*malc_obj_destroy_fn) (malc_obj_ref* obj);

#ifdef MALC_COMMON_NAMESPACED
} //namespace malcpp {
#endif

#endif

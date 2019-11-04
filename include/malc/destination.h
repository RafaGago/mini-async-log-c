#ifndef __MALC_DESTINATION_H__
#define __MALC_DESTINATION_H__

#include <bl/base/platform.h>
#include <bl/base/error.h>
#include <bl/base/integer.h>
#include <bl/base/allocator.h>
#include <bl/base/time.h>

#ifdef __cplusplus
  extern "C" {
#endif

/*------------------------------------------------------------------------------
timestamp:     timestamp string (from a monotonic clock).
timestamp_len: timestamp string length. Can be zero depending on the configuration.
sev:        severity string
sev_len:    severity string length. Can be zero depending on the configuration.
text:       log entry text. It won't contain an added trailing newline.
text_len:   log entry text length. Can't be zero.
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
log_rate_filter_time:

  Rate filter period. The same log entry arriving before this time will be
  discarded. 0 = disabled.

  It protects against malicious or involuntary loops logging the same entry (or
  set of entries) with a very high rate. Which can force the logs to rotate and
  can lead to a potential loss of information, allowing an attacker to cover
  tracks.

  This is to be used in with the global "log_rate_filter_watch_count" and
  "log_rate_filter_min_severity" on the "malc_security_cfg" struct.

  e.g. If you want to block entries that arrive with a frequency higher than
  100000 msg/sec you have to calculate the period (1/0.00001 = 10 microseconds)
  and set it by using "bl_usec_to_timept64 (10)"

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
  bl_timept64 log_rate_filter_time; /* TODO: make easy for the user and set it in ns*/
  bool        show_timestamp;
  bool        show_severity;
  bl_u8       severity;
  char const* severity_file_path;
}
malc_dst_cfg;
/*------------------------------------------------------------------------------
malc_dst:

  Resource table of a malc destination. All the functions here will be called
  non-concurrently (in a single threaded fashion) with guaranteed memory
  visibility (the thead may be swapped under the hood though).

size_of:

  "sizeof" the struct containing the implementation of an instance of this
   destination.

init:

  Initialization: instance will contain a raw memory chunk of "size_of"
  bytes. "alloc" is a provided-by-malc memory allocator in case it's necessary
  for the instance to allocate more memory on the initialization phase. (it will
  be the same instance passed to malc on "malc_create"). The allocator can be
  stored by instance implementations for later usage. This value can be set to
  null if there is no initialization to do.

terminate:

  Termination before memory deallocation. can be set to null if there is no
  termination to do. Its call will be triggered by "malc_terminate" (or
  "malc_destroy" when no "malc_terminate" call is made). Memory synchronization
  is placed afterwars by malc, so all the actions did inside the function will
  be visible to thread that calls "malc_terminate" (or "malc_destroy").

flush:

  Flush all data. To be used in case of the "write" function being buffered.
  Will always be trigered by "malc_flush" and can be called in some other
  implementation defined states too (e.g. idle). It can be set to null if there
  is no flush.

idle_task:

  Will be invoked when the logger is idle, e.g. to do low priority tasks. It
  can be set to null if there is no idle task.

write:

  Log write. Mandatory.
    now:      current timestamp, can be used internally.
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
    void* instance, bl_timept64 now, bl_uword sev_val, malc_log_strings const* strs
    );
}
malc_dst;
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
  }
#endif

#endif /* __MALC_DESTINATION_H__ */

#ifndef __MALC_DESTINATION_H__
#define __MALC_DESTINATION_H__

#include <bl/base/platform.h>
#include <bl/base/error.h>
#include <bl/base/integer.h>
#include <bl/base/allocator.h>
#include <bl/base/time.h>

/*------------------------------------------------------------------------------
log_rate_filter_time:
  Rate filter period. The same log entry arriving before this time will be
  discarded. 0 = disabled.

  It protects against malicious or involuntary loops logging the same entry (or
  set of entries) with a very high rate. Which can force the logs to rotate and
  can lead to a potential lose of important information.

  This is to be used in with the global "log_rate_filter_watch_count" and
  "log_rate_filter_min_severity" on the "malc_security_cfg" struct.

  e.g. If you want to block entries that arrive with a frequency higher than
  100000 msg/sec you have to calculate the period (1/0.00001 = 10 microseconds)
  and set it by using "bl_usec_to_tstamp (10)"

severity_file_path:
  null or a file to read the severity from.
------------------------------------------------------------------------------*/
typedef struct malc_dst_cfg {
  tstamp      log_rate_filter_time;
  bool        show_timestamp;
  bool        show_severity;
  u8          severity;
  char const* severity_file_path;
}
malc_dst_cfg;
/*------------------------------------------------------------------------------
malc_dst: Data table of a malc destination

size_of:
  sizeof the struct, will be used by malc to allocate the correct size.

init:
  Initialization: instance will contain a raw memory chunk of "size_of"
  bytes. "alloc" is a provided memory allocator in case it's necessary (it
  will be the same one passed to malc on "malc_create"). The allocator can
  be stored for later usage. It can be set to null if nothing relevant
  happens on initialization.

terminate:
  Termination before memory deallocation. can be set to null if there is no
  termination required.

flush:
  Flush all data in case of the "write" function being buffered. It can be
  set to null if there is no flush.

idle_task:
  Will be invoked when the logger is idle, e.g. to do low priority tasks. It
  can be set to null if there is no idle task.

write:
  Log write. Mandatory.

    now:           current timestamp, can be used for timeouts
    timestamp:     timestamp string
    timestamp_len: timestamp string length. can be zero
    severity:      severity string
    severity_len:  severity string length. can be zero
    text:          log entry text
    text_len:      log entry text length. can't be zero
------------------------------------------------------------------------------*/
typedef struct malc_dst {
  uword size_of;
  bl_err (*init)      (void* instance, alloc_tbl const* alloc);
  void   (*terminate) (void* instance);
  bl_err (*flush)     (void* instance);
  bl_err (*idle_task) (void* instance);
  bl_err (*write)(
    void*       instance,
    tstamp      now,
    char const* timestamp,
    uword       timestamp_len,
    char const* severity,
    uword       severity_len,
    char const* text,
    uword       text_len
    );
}
malc_dst;
/*----------------------------------------------------------------------------*/

#endif /* __MALC_DESTINATION_H__ */

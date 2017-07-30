#ifndef __MALC_CFG_H__
#define __MALC_CFG_H__

#include <bl/base/platform.h>
#include <bl/base/integer.h>
#include <bl/base/allocator.h>

/*------------------------------------------------------------------------------
idle_task_period_us:

  The IDLE task (both internal and on all the destinations) will be run with
  this periodicity.

backoff_max_us:

  Internal fine-tuning. Leave as default or read the sources before setting
  this.

start_own_thread:

  When this is set "malc" will launch and manage a dedicated thread for the
  consumer. If this is unset the logger's consume task is run manually from a
  user thread by using "malc_run_consume_task".
------------------------------------------------------------------------------*/
typedef struct malc_consumer_cfg {
  u32  idle_task_period_us;
  u32  backoff_max_us;
  bool start_own_thread;
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

  Each producer may have a buffer in TLS and a fixed buffer for all the threads,
  when both are exhausted (or disabled) the pointed heap allocator will be used.
  Note that this allocator is only used to enqueue single log entries from the
  heap. All the other dynamic memory is taken from the allocator passed on
  "malc_init".

fixed_allocator_bytes:

  The minimal amount of bytes that each fixed allocator will have. This number
  might be rounded up to cache line size or to the next power of 2. 0 disables
  the fixed allocator.

fixed_allocator_max_slots:

  The maximum amount of slots that a single call to the fixed allocator can
  have. The slot size is unspecified so this parameter is a bit vague. Consider
  a slot size equal or very near the cache line size.

fixed_allocator_per_cpu:

  Create one fixed allocator for each CPU core. This is an optimization (or
  pessimization: measure your performance) to alleviate false sharing. Note that
  "fixed_allocator_bytes" is not divided by the number CPUs when this setting is
  active.
------------------------------------------------------------------------------*/
typedef struct malc_alloc_cfg {
  alloc_tbl const* msg_allocator;
  u32              fixed_allocator_bytes;
  u32              fixed_allocator_max_slots;
  bool             fixed_allocator_per_cpu;
}
malc_alloc_cfg;
/*------------------------------------------------------------------------------
sanitize_log_entries:

  The log entries are removed from any character that may make them to be
  confused with another log line, e.g. newline, so log injection isn't possible.

log_rate_filter_watch_count:

  Controls how many different entries log entries can be watched simultaneosly
  by the log rate filter. This number may be limited to be very low (64), as
  calculating the data rate of all the previous entries has performance
  implications. 0 disables the filter.

log_rate_filter_min_severity:

  Controls the lowest severity affected by the log_rate_filter. This is intended
  to let the user decide if the lowest severitites should be filtered or not, as
  these can be used for debugging and stripped from the release executable.

  Both this and is "log_rate_filter_watch_count" are be used together with
  the per-destination (in struct "malc_dst_cfg") "log_rate_filter_time".

log_rate_filter_cutoff_eps:

  Controls the maximum rate that a given log entry can have. The units are
  entries per second.
------------------------------------------------------------------------------*/
typedef struct malc_security {
  bool sanitize_log_entries;
  u32  log_rate_filter_watch_count;
  u32  log_rate_filter_min_severity;
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

#endif /* __MALC_CFG_H__ */

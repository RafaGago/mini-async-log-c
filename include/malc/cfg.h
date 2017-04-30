#ifndef __MALC_CFG_H__
#define __MALC_CFG_H__

#include <bl/base/platform.h>
#include <bl/base/integer.h>

/*------------------------------------------------------------------------------
idle_task_period_us:
  The IDLE task will be run with this periodicity. This means that calls to
  "malc_run_idle_task" won't do nothing until the period is expired.
backoff_max_us:
  Read the sources before setting this.
start_own_thread:
  When this is set "malc" will launch and own a dedicated thread for the
  consumer. If this is unset the logger is run from an user thread  by using
  "malc_run_consume_task" and "malc_run_idle_task".
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

heap_allocator:
  Each producer may have a buffer in TLS, when it's exhausted and this variable
  is not null the pointed heap allocator is used until the TLS buffers start
  to be available again. If there is no TLS buffer the heap_allocator is the
  second used choice. Note that this allocator is only used to enqueue log
  entries from the heap. The TLS allocator is the one passed on "malc_init".

------------------------------------------------------------------------------*/
typedef struct malc_alloc_cfg {
  alloc_tbl const* heap_allocator;
}
malc_alloc_cfg;
/*------------------------------------------------------------------------------
sanitize_log_entries:
  The log entries are removed from any character that may make them to be
  confused with another log line, e.g. newline, so log injection isn't possible.

log_rate_filter_time_us:
log_rate_filter_max:
log_rate_filter_watch_count:
  These three parameters are better explained all in once.

  A hacked application may try to erase data logs by running a legitimate piece
  of code of the application in a loop, either to make the server disk to be
  full (no rotation in place) or to erase attack traces. These parameters try to
  limit messages with an excessive data rate.

  A buffer with the last log entries and its timestamps is kept. If one of these
  entries repeats too often it will be silently dropped.

  "log_rate_filter_time_us" and "log_rate_filter_max" control how many log
  entries can happen in a unit of time before the protection being activated.

  "log_rate_filter_watch_count" controls how many of the last log entries are
  watched (circular buffer).
------------------------------------------------------------------------------*/
typedef struct malc_security {
  bool sanitize_log_entries;
  u32  log_rate_filter_time_us;
  u32  log_rate_filter_max;
  u32  log_rate_filter_watch_count;
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

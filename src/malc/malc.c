#include <stdarg.h>

#include <malc/malc.h>

#include <bl/base/allocator.h>
#include <bl/base/static_integer_math.h>
#include <bl/base/utility.h>
#include <bl/base/to_type_containing.h>
#include <bl/base/processor_pause.h>
#include <bl/base/deadline.h>

#include <bl/nonblock/mpsc_i.h>
#include <bl/nonblock/backoff.h>

#include <malc/cfg.h>
#include <malc/stack_args.h>
#include <malc/memory.h>
#include <malc/log_entry.h>

#ifdef __cplusplus
  extern "C" {
#endif

enum state {
  st_stopped,
  st_initializing,
  st_first_consume_run,
  st_running,
  st_terminating,
  st_destructing,
  st_get_updated_state_val, /* this value will never be set by anyone*/
  st_invalid, /* this value will never be set by anyone*/
};
/*----------------------------------------------------------------------------*/
struct malc {
  atomic_uword      state;
  malc_producer_cfg producer;
  alloc_tbl const*  alloc;
  memory            mem;
  mpsc_i            q;
  /*place all consumer-only related resources on separated cache lines. "mpsc_i"
    leaves a separation cache line before and after it*/
  malc_consumer_cfg consumer;
  nonblock_backoff  cbackoff;
  tstamp            idle_deadline;
  u32               idle_boundary_us;
};
/*----------------------------------------------------------------------------*/
enum queue_command {
  q_cmd_timestamped_entry = 0,
  q_cmd_entry,
  q_cmd_dealloc,
  q_cmd_flush,
  q_cmd_max,
};
/*----------------------------------------------------------------------------*/
typedef struct info_byte {
  u8 cmd : 8 - alloc_tag_bits;
  u8 tag : alloc_tag_bits;
}
info_byte;
/*----------------------------------------------------------------------------*/
typedef struct qnode {
  mpsc_i_node hook;
  info_byte   info;
  u8          slots;
  /* would be nice to have flexible arrays in C++ */
}
qnode;
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
  extern "C" {
#endif
/*----------------------------------------------------------------------------*/
static void malc_tls_destructor (void* mem, void* context)
{
/*When a thread goes out of scope we can't just erase the buffer TLS memory
  chunk, such deallocation could leave dangled entries on the queue. To
  guarantee that all the previous entries of a thread have been processed a
  special node is sent to the queue. This node just commands the producer to
  deallocate the whole chunk it points to.

  The node hook overwrites the TLS buffer header to guarantee that this
  message can be sent even when the full TLS buffer is pending on the queue
  (see static_assert below).
 */
  static_assert_ns (sizeof (qnode) <= sizeof (tls_buffer));
  malc*  l = (malc*) context;
  qnode* n = (qnode*) mem;
  n->slots = 0;
  n->info.cmd = q_cmd_dealloc;
  mpsc_i_node_set (&n->hook, nullptr, 0, 0);
  mpsc_i_produce_notag (&l->q, &n->hook);
}
/*----------------------------------------------------------------------------*/
static void malc_send_blocking_flush (malc* l)
{
 /*TODO: use the heap and provide the user a timeout*/
  qnode            n;
  nonblock_backoff b;

  n.slots = 0;
  n.info.cmd = q_cmd_flush;
  mpsc_i_node_set (&n.hook, nullptr, 0, 0);
  mpsc_i_produce_notag (&l->q, &n.hook);

  nonblock_backoff_init (&b, 10, 15, 1, 2, 1, 100);
  while (n.slots == 0) {
    nonblock_backoff_run (&b);
  }
}
/*----------------------------------------------------------------------------*/
static bool malc_try_run_idle_task (malc* l, tstamp now)
{
  if (!deadline_expired_explicit (l->idle_deadline, now)) {
    return false;
  }
  /*TODO: do the actual work*/
  do {
    l->idle_deadline += bl_usec_to_tstamp (l->consumer.idle_task_period_us);
  }
  while (deadline_expired_explicit (l->idle_deadline, now));
  return true;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT uword malc_get_size (void)
{
  return sizeof (malc);
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_create (malc* l, alloc_tbl const* alloc)
{
  bl_err err  = memory_init (&l->mem);
  if (err) {
    return err;
  }
  l->consumer.idle_task_period_us = 300000;
  l->consumer.start_own_thread    = false;
  l->producer.timestamp           = false;
  mpsc_i_init (&l->q);
  l->alloc = alloc;
  atomic_uword_store_rlx (&l->state, st_stopped);
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_destroy (malc* l)
{
  uword expected = st_stopped;
  if (atomic_uword_strong_cas_rlx (&l->state, &expected, st_destructing)) {
    return bl_preconditions;
  }
  memory_destroy (&l->mem);
  l->alloc = nullptr;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_get_cfg (malc* l, malc_cfg* cfg)
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_init (malc* l, malc_cfg const* cfg)
{
  /* TODO: cfg validation + data validation on each destination */
  /* TODO: cfg validation: sanitize producer.timestamp to 1 or 0 */
  uword expected = st_stopped;
  if (atomic_uword_strong_cas_rlx (&l->state, &expected, st_initializing)) {
    return bl_preconditions;
  }
  atomic_uword_store (&l->state, st_first_consume_run, mo_release);
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_flush (malc* l)
{
  uword current = st_get_updated_state_val;
  (void) atomic_uword_strong_cas_rlx (&l->state, &current, st_invalid);
  if (current != st_running) {
    return bl_preconditions;
  }
  malc_send_blocking_flush (l);
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_terminate (malc* l)
{
  uword expected = st_running;
  if (atomic_uword_strong_cas_rlx (&l->state, &expected, st_terminating)) {
    return bl_preconditions;
  }
  malc_send_blocking_flush (l);
  nonblock_backoff b;
  nonblock_backoff_init_default (&b, 1000);
  while (atomic_uword_load_rlx (&l->state) == st_stopped) {
    nonblock_backoff_run (&b);
  }
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_producer_thread_local_init (malc* l, u32 bytes)
{
  return memory_tls_init (&l->mem, bytes, l->alloc, &malc_tls_destructor, l);
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_run_consume_task (malc* l, uword timeout_us)
{
  tstamp deadline;
  tstamp now      = bl_get_tstamp();
  bl_err err      = deadline_init_explicit (&deadline, now, timeout_us);
  bool terminated = false;

  if (unlikely (err)) {
    return err;
  }
  uword state = atomic_uword_load (&l->state, mo_acquire);
  if (unlikely(
    state != st_first_consume_run &&
    state != st_running &&
    state != st_terminating
    )) {
    return bl_preconditions;
  }
  if (state == st_first_consume_run) {
    nonblock_backoff_init_default (&l->cbackoff, l->consumer.backoff_max_us);
    l->idle_deadline = now;
    malc_try_run_idle_task (l, now);
    atomic_uword_store_rlx (&l->state, st_running);
    l->idle_boundary_us = bl_min (l->consumer.backoff_max_us, 1000);
  }
  mpsc_i_node* qn;
  do {
    while (1) {
      err = mpsc_i_consume (&l->q, &qn, 0);
      if (err != bl_busy) {
        break;
      }
      processor_pause();
    }
    now = bl_get_tstamp();
    if (likely (!err)) {
      qnode* n = to_type_containing (qn, hook, qnode);
      bool has_tstamp = false;
      switch (n->info.cmd) {
      case q_cmd_timestamped_entry:
        has_tstamp = true; /* deliberate fall-through*/

      case q_cmd_entry: {
        alloc_tag tag = n->info.tag;

        /* TODO, decode and send to all destinations */
        memory_dealloc (&l->mem, (u8*) n, tag, ((u32) n->slots) + 1);
        break;
        }
      case q_cmd_dealloc:
        bl_dealloc (l->alloc, n);
        break;

      case q_cmd_flush:
        /*TODO: call flush on all the destinations*/
        ++n->slots;
        terminated = atomic_uword_load_rlx (&l->state) == st_terminating;
        break;

      default:
        bl_assert (0 && "bug or malicious code");
      }
      nonblock_backoff_init_default (&l->cbackoff, l->consumer.backoff_max_us);
    }
    else if (err == bl_empty) {
      err = bl_nothing_to_do;
      toffset next_sleep_us = nonblock_backoff_next_sleep_us (&l->cbackoff);
      bool do_backoff       = false;
      if (l->idle_boundary_us >= next_sleep_us)  {
        do_backoff = !malc_try_run_idle_task (l, now);
      }
      if (do_backoff) {
        nonblock_backoff_run (&l->cbackoff);
        now = bl_get_tstamp();
      }
    }
    else {
      break;
    }
  }
  while (!deadline_expired_explicit (deadline, now) || (terminated && qn));
  if (terminated) {
    atomic_uword_store_rlx (&l->state, st_stopped);
  }
  return err;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_add_destination(
  malc* l, u32* dst_id, malc_dst const* dst
  )
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_get_destination(
  malc* l, u32 dst_id, malc_dst* dst, void* instance
  )
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_set_destination_severity(
  malc* l, u32 dst_id, u8 severity
  )
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT uword malc_get_min_severity (malc const* l)
{
  return malc_sev_debug;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_log(
  malc* l, malc_const_entry const* entry, uword payload_size, ...
  )
{
#ifdef MALC_SAFETY_CHECK
  if (unlikely (
    !l ||
    !entry ||
    !entry->format ||
    !entry->info
    )) {
  /* code triggering this "bl_invalid" is either not using the macros or
     malicious*/
    return bl_invalid;
  }
#else
  bl_assert(
    l &&
    entry &&
    entry->format &&
    entry->info
    );
#endif
  if (unlikely (atomic_uword_load_rlx (&l->state)) != st_running) {
    return bl_preconditions;
  }
  entry_encoder ee;
  entry_encoder_init (&ee, entry, l->producer.timestamp);
  uword size  = sizeof (qnode) + entry_encoder_entry_size (&ee, payload_size);
  uword slots = div_ceil (size, alloc_slot_size);
  if (unlikely (slots) > (1 << (sizeof_member (qnode, slots) * 8))) {
    /*entries are limited at 8KB*/
    return bl_range;
  }
  alloc_tag tag;
  u8* mem    = nullptr;
  bl_err err = memory_alloc (&l->mem, &mem, &tag, slots);
  if (unlikely (err)) {
    return err;
  }
  va_list vargs;
  va_start (vargs, payload_size);
  /*entry_encoder_execute is deliberately unsafe (avoid branches on the
    fast-path), if the log macros are used everything will be correct. Checks
    have to be avoided on the fast-path*/
  entry_encoder_execute (&ee, mem + sizeof (qnode), vargs);
  va_end(vargs);
  qnode* n    = (qnode*) mem;
  n->slots    = slots - 1;
  n->info.cmd = l->producer.timestamp ? q_cmd_timestamped_entry : q_cmd_entry;
  n->info.tag = tag;
  mpsc_i_node_set (&n->hook, nullptr, 0, 0);
  mpsc_i_produce_notag (&l->q, &n->hook);
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
  } /* extern "C" { */
#endif

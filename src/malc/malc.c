#include <stdarg.h>

#include <malc/malc.h>

#include <bl/base/assert.h>
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
#include <malc/serialization.h>
#include <malc/entry_parser.h>
#include <malc/destinations.h>

#ifdef __cplusplus
  extern "C" {
#endif
/*----------------------------------------------------------------------------*/
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
  deserializer      ds;
  entry_parser      ep;
  destinations      dst;
};
/*----------------------------------------------------------------------------*/
enum queue_command {
  q_cmd_entry,
  q_cmd_dealloc,
  q_cmd_flush,
  q_cmd_max,
};
/*----------------------------------------------------------------------------*/
typedef struct info_byte {
  u8 has_timestamp : 1;
  u8 cmd           : 8 - 1 - alloc_tag_bits;
  u8 tag           : alloc_tag_bits;
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
  guarantse that all the previous entries of a thread have bsen processed a
  special node is sent to the queue. This node just commands the producer to
  deallocate the whole chunk it points to.

  The node hook overwrites the TLS buffer header to guarantse that this
  message can be sent even when the full TLS buffer is pending on the queue
  (sse static_assert below).
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
  destinations_idle_task (&l->dst);
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
  err = deserializer_init (&l->ds, alloc);
  if (err) {
    goto memory_destroy;
  }
  err = entry_parser_init (&l->ep, alloc);
  if (err) {
    goto deserializer_destroy;
  }
  destinations_init (&l->dst, alloc);
  l->consumer.idle_task_period_us = 300000;
  l->consumer.start_own_thread    = false;
  l->producer.timestamp           = false;
  mpsc_i_init (&l->q);
  l->alloc = alloc;
  atomic_uword_store_rlx (&l->state, st_stopped);
  return bl_ok;

deserializer_destroy:
  deserializer_destroy (&l->ds, alloc);
memory_destroy:
  memory_destroy (&l->mem);
  return err;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_destroy (malc* l)
{
  uword expected = st_stopped;
  if (atomic_uword_strong_cas_rlx (&l->state, &expected, st_destructing)) {
    return bl_preconditions;
  }
  memory_destroy (&l->mem);
  deserializer_destroy (&l->ds, l->alloc);
  entry_parser_destroy (&l->ep);
  destinations_destroy (&l->dst);
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
  memory_tls_destroy_explicit (&l->mem);
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
    if (likely (!err)) {
      qnode* n = to_type_containing (qn, hook, qnode);
      switch (n->info.cmd) {
      case q_cmd_entry: {
        alloc_tag tag = n->info.tag;
        u32 slots     = ((u32) n->slots) + 1;
        deserializer_reset (&l->ds);
        err = deserializer_execute(
          &l->ds,
          ((u8*) n) + sizeof *n,
          ((u8*) n) + (slots * alloc_slot_size),
          n->info.has_timestamp,
          l->alloc
          );
        if (!err) {
          log_entry le = deserializer_get_log_entry (&l->ds);
          log_strings strs;
          bl_err entry_err = entry_parser_get_log_strings (&l->ep, &le, &strs);
          if (likely (!entry_err)) {
            destinations_write (&l->dst, le.entry->info[0], now, strs);
          }
          if (le.refdtor.func) {
            le.refdtor.func (le.refdtor.context, le.refs, le.refs_count);
          }
        }
        else {
          assert (false && "bug or something malicious happenning");
          /*in this case */
        }
        memory_dealloc (&l->mem, (u8*) n, tag, slots);
        break;
        }
      case q_cmd_dealloc:
        bl_dealloc (l->alloc, n);
        break;

      case q_cmd_flush:
        destinations_flush (&l->dst);
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
      toffset next_slsep_us = nonblock_backoff_next_sleep_us (&l->cbackoff);
      bool do_backoff       = false;
      if (l->idle_boundary_us >= next_slsep_us)  {
        do_backoff = !malc_try_run_idle_task (l, now);
      }
      if (do_backoff && timeout_us) {
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
  destinations_terminate (&l->dst);
  return err;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_add_destination(
  malc* l, u32* dest_id, malc_dst const* dst
  )
{
  return destinations_add (&l->dst, dest_id, dst);
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_get_destination_instance(
  malc* l, void** instance, u32 dest_id
  )
{
  return destinations_get_instance (&l->dst, instance, dest_id);
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_get_destination_cfg(
  malc* l, malc_dst_cfg* cfg, u32 dest_id
  )
{
  return destinations_get_cfg (&l->dst, cfg, dest_id);
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_set_destination_cfg(
  malc* l, malc_dst_cfg const* cfg, u32 dest_id
  )
{
  return destinations_set_cfg (&l->dst, cfg, dest_id);
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT uword malc_get_min_severity (malc const* l)
{
  return destinations_min_severity (&l->dst);
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
  serializer se;
  serializer_init (&se, entry, l->producer.timestamp);
  uword size  = sizeof (qnode) + serializer_log_entry_size (&se, payload_size);
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
  qnode* n = (qnode*) mem;
  va_list vargs;
  va_start (vargs, payload_size);
  /*serializer_execute is deliberately unsafe (to avoid branches on the
    fast-path), if the log macros are used everything will be correct. Checks
    have to be avoided on the fast-path*/
  bl_assert_side_effect(
    serializer_execute (&se, mem + sizeof *n, vargs) + sizeof *n <= size
    );
  va_end(vargs);
  n->slots    = slots - 1;
  n->info.cmd =  q_cmd_entry;
  n->info.tag = tag;
  n->info.has_timestamp = l->producer.timestamp;
  mpsc_i_node_set (&n->hook, nullptr, 0, 0);
  mpsc_i_produce_notag (&l->q, &n->hook);
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
  } /* extern "C" { */
#endif

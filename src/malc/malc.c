#include <stdarg.h>

#include <malc/malc.h>

#include <bl/base/assert.h>
#include <bl/base/allocator.h>
#include <bl/base/static_integer_math.h>
#include <bl/base/utility.h>
#include <bl/base/to_type_containing.h>
#include <bl/base/processor_pause.h>
#include <bl/base/deadline.h>
#include <bl/base/cache.h>
#include <bl/base/static_assert.h>

#include <bl/nonblock/mpsc_i.h>
#include <bl/nonblock/backoff.h>

#include <malc/cfg.h>
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
  st_running,
  st_terminating,
  st_destructing,
  st_get_updated_state_val, /* this value will never be set by anyone*/
  st_invalid,               /* this value will never be set by anyone*/
};
/*----------------------------------------------------------------------------*/
struct malc {
  declare_cache_pad_member;
  atomic_uword      state;
  malc_producer_cfg producer;
  alloc_tbl const*  alloc;
  memory            mem;
  mpsc_i            q;
  /*place all consumer-only related resources on separated cache lines. "mpsc_i"
    leaves a separation cache line before and after it*/
  bl_thread         thread;
  malc_consumer_cfg consumer;
  malc_security     sec;
  nonblock_backoff  cbackoff;
  tstamp            idle_deadline;
  u32               idle_boundary_us;
  deserializer      ds;
  entry_parser      ep;
  destinations      dst;
  declare_cache_pad_member;
};
/*----------------------------------------------------------------------------*/
enum queue_command {
  q_cmd_entry,
  q_cmd_tls_register,
  q_cmd_tls_dealloc_deregister,
  q_cmd_flush,
  q_cmd_terminate,
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
static_assert_outside_func_ns (pow2_u (8 - 1 - alloc_tag_bits) >= q_cmd_max);
/*----------------------------------------------------------------------------*/
typedef struct qnode {
  mpsc_i_node hook;
  info_byte   info;
  u8          slots;
  /* would be nice to have flexible arrays in C++ */
}
qnode;
/*----------------------------------------------------------------------------*/
typedef struct qnode_tls_alloc {
  qnode n;
  void* mem;
}
qnode_tls_alloc;
/*----------------------------------------------------------------------------*/
static void malc_tls_destructor (void* mem, void* context)
{
/*When a thread goes out of scope we can't just deallocate its TLS memory
  chunk/buffer; an immediate deallocation could leave dangled entries on the
  queue. To guarantee that all the previous entries of a thread have been
  processed before deallocating the TLS chunk the deallocation is done by
  sending a deallocation command to the consumer thread using the same log
  queue. This guarantees that all log entries belonging to this memory chunk
  have been processed before. */

  static_assert_ns (sizeof (qnode) <= sizeof (tls_buffer));
  malc*  l = (malc*) context;
  qnode* n = (qnode*) mem;
  n->slots = 0;
  n->info.cmd = q_cmd_tls_dealloc_deregister;
  mpsc_i_node_set (&n->hook, nullptr, 0, 0);
  mpsc_i_produce_notag (&l->q, &n->hook);
}
/*----------------------------------------------------------------------------*/
static void malc_send_blocking_flush (malc* l)
{
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
  destinations_idle_task (&l->dst, now);
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
  if (!alloc) {
    return bl_mkerr (bl_invalid);
  }
  bl_err err  = memory_init (&l->mem, alloc);
  if (err.bl) {
    return err;
  }
  err = deserializer_init (&l->ds, alloc);
  if (err.bl) {
    goto memory_destroy;
  }
  err = entry_parser_init (&l->ep, alloc);
  if (err.bl) {
    goto deserializer_destroy;
  }
  destinations_init (&l->dst, alloc);

  /*Set all producer/consumer default settings*/
  l->consumer.idle_task_period_us = 300000;
  l->consumer.backoff_max_us      = 2000;
  l->consumer.start_own_thread    = false;
  l->producer.timestamp           = false;

  mpsc_i_init (&l->q);
  l->alloc = alloc;
  atomic_uword_store_rlx (&l->state, st_stopped);
  return bl_mkok();

deserializer_destroy:
  deserializer_destroy (&l->ds, alloc);
memory_destroy:
  memory_destroy (&l->mem, alloc);
  return err;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_destroy (malc* l)
{
  uword expected = st_stopped;
  if (!atomic_uword_strong_cas_rlx (&l->state, &expected, st_destructing)) {
    return bl_mkerr (bl_preconditions);
  }
  memory_destroy (&l->mem, l->alloc);
  deserializer_destroy (&l->ds, l->alloc);
  entry_parser_destroy (&l->ep);
  destinations_destroy (&l->dst);
  l->alloc = nullptr;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_get_cfg (malc const* l, malc_cfg* cfg)
{
  cfg->consumer = l->consumer;
  cfg->producer = l->producer;
  cfg->alloc    = l->mem.cfg;
  cfg->sec.sanitize_log_entries = l->ep.sanitize_log_entries;
  destinations_get_rate_limit_settings (&l->dst, &cfg->sec);
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static void malc_producer_thread_env_init(malc* l)
{
    tstamp now = bl_get_tstamp();
    nonblock_backoff_init_default (&l->cbackoff, l->consumer.backoff_max_us);
    l->idle_deadline = now;
    malc_try_run_idle_task (l, now);
    l->idle_boundary_us = bl_min (l->consumer.backoff_max_us, 1000);
    atomic_uword_store (&l->state, st_running, mo_release);
}
/*----------------------------------------------------------------------------*/
static int malc_thread (void* d)
{
  malc_producer_thread_env_init ((malc*) d);
  bl_err err;
  do {
    err = malc_run_consume_task ((malc*) d, 200000);
  }
  while (!err.bl || err.bl == bl_nothing_to_do);
  return 0;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_init (malc* l, malc_cfg const* cfg_readonly)
{
  /* validation */
  malc_cfg cfg;
  if (cfg_readonly) {
    cfg = *cfg_readonly;
  }
  else {
    malc_get_cfg (l, &cfg);
  }
  if (cfg.consumer.backoff_max_us == 0) {
    return bl_mkerr (bl_invalid);
  }
  bl_err err = destinations_validate_rate_limit_settings (&l->dst, &cfg.sec);
  if (err.bl) {
    return err;
  }
  /* booleanization */
  cfg.consumer.start_own_thread = !!cfg.consumer.start_own_thread;
  cfg.producer.timestamp        = !!cfg.producer.timestamp;
  cfg.sec.sanitize_log_entries  = !!cfg.sec.sanitize_log_entries;

  /* initialization */
  uword expected = st_stopped;
  if (!atomic_uword_strong_cas_rlx (&l->state, &expected, st_initializing)) {
    return bl_mkerr (bl_preconditions);
  }
  l->mem.cfg  = cfg.alloc;
  l->consumer = cfg.consumer;
  l->producer = cfg.producer;
  l->ep.sanitize_log_entries = cfg.sec.sanitize_log_entries;
  err = destinations_set_rate_limit_settings (&l->dst, &cfg.sec);
  if (err.bl) {
    goto finish;
  }
  err = memory_bounded_buffer_init (&l->mem, l->alloc);
  if (err.bl) {
    goto finish;
  }
  if (l->consumer.start_own_thread) {
    err = bl_thread_init (&l->thread, malc_thread, l);
    if (!err.bl) {
      while (atomic_uword_load_rlx (&l->state) == st_initializing) {
        bl_thread_yield();
      }
    }
  }
  else {
    malc_producer_thread_env_init (l);
  }
finish:
  if (err.bl) {
    atomic_uword_store_rlx (&l->state, expected);
  }
  return err;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_flush (malc* l)
{
  uword current = st_get_updated_state_val;
  (void) atomic_uword_strong_cas_rlx (&l->state, &current, st_invalid);
  if (current != st_running) {
    return bl_mkerr (bl_preconditions);
  }
  malc_send_blocking_flush (l);
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_terminate (malc* l, bool is_consume_task_thread)
{
  qnode* n = bl_alloc (l->alloc, sizeof *n);
  if (!n) {
    return bl_mkerr (bl_alloc);
  }
  /* force the the thread local storage destructor to run now (if any) instead
     of doing it when the thread goes out of scope */
  bl_err err = memory_tls_try_run_destructor (&l->mem);
  if (err.bl) {
    bl_dealloc (l->alloc, n);
    return err;
  }
  uword expected = st_running;
  if (!atomic_uword_strong_cas(
    &l->state, &expected, st_terminating, mo_release, mo_relaxed
    )) {
    bl_dealloc (l->alloc, n);
    return bl_mkerr (bl_preconditions);
  }
  n->slots = 0;
  n->info.cmd = q_cmd_terminate;
  mpsc_i_node_set (&n->hook, nullptr, 0, 0);
  mpsc_i_produce_notag (&l->q, &n->hook);

  if (!is_consume_task_thread) {
    if (l->consumer.start_own_thread) {
      bl_thread_join (&l->thread);
    }
    else {
      nonblock_backoff b;
      nonblock_backoff_init_default (&b, 1000);
      while (atomic_uword_load (&l->state, mo_acquire) != st_stopped) {
        nonblock_backoff_run (&b);
      }
    }
  }
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_producer_thread_local_init (malc* l, u32 bytes)
{
  qnode_tls_alloc* n;
  n = (qnode_tls_alloc*) bl_alloc (l->alloc, sizeof *n);
  if (!n) {
    return bl_mkerr (bl_alloc);
  }
  n->n.slots = 0;
  n->n.info.cmd = q_cmd_tls_register;
  bl_err err = memory_tls_init_unregistered(
    &l->mem, bytes, l->alloc, &malc_tls_destructor, l, &n->mem
    );
  if (err.bl) {
    bl_dealloc (l->alloc, n);
    return err;
  }
  mpsc_i_node_set (&n->n.hook, nullptr, 0, 0);
  mpsc_i_produce_notag (&l->q, &n->n.hook);
  return err;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_run_consume_task (malc* l, uword timeout_us)
{
  tstamp deadline;
  tstamp now = bl_get_tstamp();
  bl_err err = deadline_init_explicit (&deadline, now, timeout_us);

  if (unlikely (err.bl)) {
    return err;
  }
  uword state = atomic_uword_load (&l->state, mo_acquire);
  if (unlikely (state != st_running && state != st_terminating)) {
    return bl_mkerr (bl_preconditions);
  }
  mpsc_i_node* qn;
  uword count = 0;
  do {
    while (1) {
      qn = nullptr;
      err = mpsc_i_consume (&l->q, &qn, 0);
      if (err.bl != bl_busy) {
        break;
      }
      processor_pause();
    }
    if (likely (!err.bl)) {
      ++count;
      qnode* n = to_type_containing (qn, hook, qnode);
      switch (n->info.cmd) {
      case q_cmd_entry: {
        alloc_tag tag = n->info.tag;
        u32 slots     = ((u32) n->slots) + 1;
        deserializer_reset (&l->ds);
        err = deserializer_execute(
          &l->ds,
          ((u8*) n) + sizeof *n,
          ((u8*) n) + (slots * l->mem.cfg.slot_size),
          n->info.has_timestamp,
          l->alloc
          );
        if (!err.bl) {
          log_entry le = deserializer_get_log_entry (&l->ds);
          malc_log_strings strs;
          bl_err entry_err = entry_parser_get_log_strings (&l->ep, &le, &strs);
          if (likely (!entry_err.bl)) {
            /*NOTE: Possible problem when using the rate_filter:

            The format string pointer (to a constant) is used raw as an entry
            id/hash. This can potentially lead to id/hash collisions on the
            rate_filter if some entries have the same format string. (e.g. {})
            and the linker optimizes them away (it should).

            If I had to improve this, my preferred way would be to always
            concatenate __LINE__ to the format string to decrease the chances
            of the linker optimizing a given string. Then __LINE__ would
            be just ignored by the entry_parser. This method decreases the
            collision chance a lot withouth needing to bloat the binaries by
            forcing the use of __FILE__.

            Note that log lines that prefix the file and line are not affected
            by this. */
            destinations_write(
              &l->dst, (uword) le.entry->format, now, le.entry->info[0], &strs
              );
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
      case q_cmd_tls_register:
        /* a list will all the created TLS buffers is maintained. The
        registered TLS buffers are serialized through the event loop, so they
        avoid mutexes. This is done like this because they are also unregistered
        by this function/thread. */
        memory_tls_register (&l->mem, ((qnode_tls_alloc*) n)->mem, l->alloc);
        bl_dealloc (l->alloc, n);
        break;

      case q_cmd_tls_dealloc_deregister:
        /* when a thread goes out of scope the TLS destructor runs. This
        destructor calls "malc_tls_destructor", which sends the whole TLS buffer
        memory chunk as a queue node, so it is deallocated from this (consumer)
        thread. This is to guarantee that all pending entries originated on each
        TLS buffer are consumed before deallocating the buffer itself. See
        "malc_tls_destructor". */
        bl_assert_side_effect(
          memory_tls_destroy (&l->mem, (void*) n, l->alloc)
          );
        break;

      case q_cmd_flush:
        destinations_flush (&l->dst);
        ++n->slots; /* poor-man's signalling back to the caller */
        break;

      case q_cmd_terminate:
        bl_assert(
          atomic_uword_load_rlx (&l->state) == st_terminating && "Malc BUG"
          );
        bl_assert(
          mpsc_i_consume (&l->q, &qn, 0).bl == bl_empty &&
          "client code BUG: sending messages after termination. May leak."
          );
        bl_dealloc (l->alloc, n);
        destinations_terminate (&l->dst);
        /*Destroy all registered TLS buffers. From now on all thread local
        buffers from an hypothetical thread outliving the logger
        ("bl_thread_local malc_tls") will be dangled. The docs say that threads
        using TLS can't outlive the logger (reason on the next paragraph) so the
        TLS buffers are dangled on purpose, forcing the user to investigate the
        segfaults that doing this will (hopefully) trigger.

        The reason why the threads with TLS buffers can't outlive the logger is
        that a "thread local" variable can't be set from another thread (this
        one), so there is no way to force a clean and safe shutdown sequence
        (that I can think of) from inside the library (setting the pointer to
        null + deallocating) without using heavyweight locking, which defeats
        the purpose of this library. The user is forced to only use TLS on
        threads that he owns, which is a good side effect IMO.*/
        memory_tls_destroy_all (&l->mem, l->alloc);
        /* release fence here to ensure that all the actions done on
        "destinations_terminate" are visibile to the thread that called
        "malc_terminate" */
        atomic_uword_store (&l->state, st_stopped, mo_release);
        return bl_mkok();

      default:
        bl_assert (0 && "bug or malicious code");
      }
      nonblock_backoff_init_default (&l->cbackoff, l->consumer.backoff_max_us);
    }
    else if (err.bl == bl_empty) {
      toffset next_sleep_us = nonblock_backoff_next_sleep_us (&l->cbackoff);
      bool do_backoff       = true;
      if (l->idle_boundary_us < next_sleep_us)  {
        do_backoff = !malc_try_run_idle_task (l, now);
      }
      if (do_backoff) {
        nonblock_backoff_run (&l->cbackoff);
        /* no timer updates on spin/yield phase of the backoff (expensive?) */
        now = next_sleep_us ? bl_get_tstamp() : now;
      }
    }
    else {
      break;
    }
  }
  while (!deadline_expired_explicit (deadline, now));
  return bl_mkerr (count ? bl_ok : bl_nothing_to_do);
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_add_destination(
  malc* l, u32* dest_id, malc_dst const* dst
  )
{
  uword state = atomic_uword_fetch_add_rlx (&l->state, 0);
  if (state > st_initializing) {
    return bl_mkerr (bl_preconditions);
  }
  return destinations_add (&l->dst, dest_id, dst);
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_get_destination_instance(
  malc const* l, void** instance, u32 dest_id
  )
{
  return destinations_get_instance (&l->dst, instance, dest_id);
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_get_destination_cfg(
  malc const* l, malc_dst_cfg* cfg, u32 dest_id
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
MALC_EXPORT bl_err malc_log_entry_prepare(
  malc*                   l,
  malc_serializer*        ext_ser,
  malc_const_entry const* entry,
  uword                   payload_size
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
    return bl_mkerr (bl_invalid);
  }
#else
  bl_assert(
    l &&
    entry &&
    entry->format &&
    entry->info
    );
#endif
  uword state = atomic_uword_load_rlx (&l->state);
  if (unlikely (state != st_running)) {
    return bl_mkerr (bl_preconditions);
  }
  serializer se;
  serializer_init (&se, entry, l->producer.timestamp);
  uword size  = sizeof (qnode) + serializer_log_entry_size (&se, payload_size);
  uword slots = div_ceil (size, l->mem.cfg.slot_size);
  if (unlikely (slots) > (1 << (sizeof_member (qnode, slots) * 8))) {
    /*entries are limited at 8KB*/
    return bl_mkerr (bl_range);
  }
  alloc_tag tag;
  u8* mem    = nullptr;
  bl_err err = memory_alloc (&l->mem, &mem, &tag, slots);
  if (unlikely (err.bl)) {
    return err;
  }
  qnode* n = (qnode*) mem;
  n->slots    = slots - 1;
  n->info.cmd =  q_cmd_entry;
  n->info.tag = tag;
  n->info.has_timestamp = l->producer.timestamp;
  mpsc_i_node_set (&n->hook, nullptr, 0, 0);
  *ext_ser = serializer_prepare_external_serializer (&se, mem, mem + sizeof *n);
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT void malc_log_entry_commit(
  malc* l, malc_serializer const* ext_ser
  )
{
  mpsc_i_produce_notag (&l->q, &((qnode*) ext_ser->node_mem)->hook);
}
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
  } /* extern "C" { */
#endif

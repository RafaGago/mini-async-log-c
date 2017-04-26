#include <malc/malc.h>

#include <bl/base/allocator.h>
#include <bl/base/static_integer_math.h>
#include <bl/base/utility.h>
#include <bl/base/to_type_containing.h>
#include <bl/base/processor_pause.h>
#include <bl/nonblock/mpsc_i.h>

#include <malc/cfg.h>
#include <malc/stack_args.h>
#include <malc/memory.h>

#ifdef __cplusplus
  extern "C" {
#endif

/*----------------------------------------------------------------------------*/
struct malc {
  malc_producer_cfg producer;
  memory            mem;
  mpsc_i            q;
  alloc_tbl const*  alloc;
};
/*----------------------------------------------------------------------------*/
enum queue_command {
  q_cmd_dealloc = 0,
  q_cmd_max,
};
/*----------------------------------------------------------------------------*/
typedef struct qnode {
  mpsc_i_node hook;
  uword  data;
  u8     slots;
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
  n->data  = q_cmd_dealloc;
  mpsc_i_node_set (&n->hook, nullptr, 0, 0);
  mpsc_i_produce_notag (&l->q, &n->hook);
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
  mpsc_i_init (&l->q);
  l->alloc = alloc;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_destroy (malc* l)
{
  memory_destroy (&l->mem);
  /*TODO: no destroy on mpsc_i mpsc_i_destroy (&l->q);*/
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
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_terminate (malc* l)
{
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
  /* INCOMPLETE */
  mpsc_i_node* qn;
  bl_err err;
  while (1) {
    err = mpsc_i_consume (&l->q, &qn, 0);
    if (err != bl_busy) {
      break;
    }
    processor_pause();
  }
  if (likely (!err)) {
    qnode* n = to_type_containing (qn, hook, qnode);
    if (n->data != q_cmd_dealloc) {
      alloc_tag               tag;
      tag = n->data & alloc_tag_mask;
      /* unused warning
      malc_const_entry const* ent;
      ent = (malc_const_entry const*) (n->data & ~alloc_tag_mask);
      */
      /* TODO, decode and send to all destinations */
      memory_dealloc (&l->mem, (u8*) n, tag, ((u32) n->slots) + 1);
    }
    else {
      bl_assert (n->data == q_cmd_dealloc);
      bl_dealloc (l->alloc, n);
    }
    return bl_ok;
  }
  return (err == bl_empty) ? bl_nothing_to_do : err;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_run_idle_task (malc* l, bool force)
{
  return bl_ok;
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
  malc*                   l,
  malc_const_entry const* e,
  uword                   va_min_size,
  uword                   va_max_size,
  int                     argc,
  ...
  )
{
#ifdef MALC_SAFETY_CHECK
  if (unlikely (
    !l ||
    !e ||
    !e->format ||
    !e->info ||
    e->compressed_count > argc ||
    va_min_size > va_max_size
    )) {
  /* code triggering this "bl_invalid" is either not using the macros or
     malicious*/
    return bl_invalid;
  }
#else
  bl_assert(
    l &&
    e &&
    e->format &&
    e->info &&
    e->compressed_count <= argc &&
    va_min_size <= va_max_size
    );
#endif
  /* TODO: this is very preliminary, */
  alloc_tag tag;
  uword  size  = va_max_size += sizeof (*e);
  u8*    mem   = nullptr;
  uword  slots = div_ceil (size, alloc_slot_size);
  bl_err err   = memory_tls_alloc (&l->mem, &mem, &tag, slots);
  /* a failure here will do va_max_size - va_min_size, and if the difference is
     just one alloc_slot it will just overallocate, the block count will be
     saved on the first byte. (256 slots will be the maximum size)*/
  /* reminder: when expand fails the tls has to be deallocated */
  if (unlikely (err)) {
    /* TODO: now just testing the TLS */
    /* err = memory_alloc (&l->m, &mem, &tag, size); */
    if (unlikely (err)) {
      return err;
    }
  }
  qnode* n = (qnode*) mem;
  n->slots = 0; /*TODO*/
  n->data  = (uword) e;
  bl_assert ((n->data & ~alloc_tag_mask) == n->data);
  n->data |= tag & alloc_tag_mask;
  mpsc_i_node_set (&n->hook, nullptr, 0, 0);
  mpsc_i_produce_notag (&l->q, &n->hook);
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
  } /* extern "C" { */
#endif

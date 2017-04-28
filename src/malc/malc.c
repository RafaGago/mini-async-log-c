#include <string.h>
#include <stdarg.h>

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
static void malc_encode (u8* mem, va_list vargs, malc_const_entry const* e)
{
  char const* partype = &e->info[1];

  uword compressed_header_bytes = (e->compressed_count + 1) & ~u_lsb_set (1);
  for (uword i = 0; i < compressed_header_bytes; ++i) {
    mem[i] = 0;
  }
  uword compress_idx = 0;
  u8* wptr = mem + compressed_header_bytes;
  /* the compiler will remove the fixed-size memcpy calls*/
  while (*partype) {
    switch (*partype) {
    case malc_type_i8:
    case malc_type_u8: {
      u8 v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v, sizeof v);
      wptr += sizeof v;
      break;
      }
    case malc_type_i16:
    case malc_type_u16: {
      u16 v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v, sizeof v);
      wptr += sizeof v;
      break;
      }
#ifdef MALC_NO_BUILTIN_COMPRESSION
    case malc_type_i32:
    case malc_type_u32: {
      u32 v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v, sizeof v);
      wptr += sizeof v;
      break;
      }
#else
    case malc_type_i32:
    case malc_type_u32: {
      malc_compressed_32 v;
      v = malc_get_va_arg (vargs, v);
      uword size = (v.format_nibble & ((1 << 3) - 1)) + 1;
      bl_assert (size <= sizeof (u32));
      mem[compress_idx / 2] |= (u8) v.format_nibble << ((compress_idx & 1) * 4);
      ++compress_idx;
      for (uword i = 0; i < size; ++i) {
        *wptr = (u8) (v.v >> (i * 8));
        ++wptr;
      }
      break;
      }
#endif
    case malc_type_float: {
      float v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v, sizeof v);
      wptr += sizeof v;
      break;
      }
#ifdef MALC_NO_BUILTIN_COMPRESSION
    case malc_type_i64:
    case malc_type_u64: {
      u64 v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v, sizeof v);
      wptr += sizeof v;
      break;
      }
#else
    case malc_type_i64:
    case malc_type_u64: {
      malc_compressed_64 v;
      v = malc_get_va_arg (vargs, v);
      uword size = (v.format_nibble & ((1 << 3) - 1)) + 1;
      bl_assert (size <= sizeof (u64));
      mem[compress_idx / 2] |= (u8) v.format_nibble << ((compress_idx & 1) * 4);
      ++compress_idx;
      for (uword i = 0; i < size; ++i) {
        *wptr = (u8) (v.v >> (i * 8));
        ++wptr;
      }
      break;
    }
#endif
    case malc_type_double: {
      double v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v, sizeof v);
      wptr += sizeof v;
      break;
      }
    case malc_type_ptr: {
      void* v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v, sizeof v);
      wptr += sizeof v;
      break;
      }
    case malc_type_lit: {
      malc_lit v = malc_get_va_arg (vargs, v);
      memcpy (wptr, &v.lit, sizeof v);
      wptr += sizeof v;
      break;
      }
    case malc_type_str:
    case malc_type_bytes: {
      malc_mem v = malc_get_va_arg (vargs, v);
      if (unlikely (!v.mem)) {
        v.size = 0;
      }
      memcpy (wptr, &v.size, sizeof v.size);
      wptr += sizeof v.size;
      memcpy (wptr, v.mem, v.size);
      wptr += v.size;
      break;
      }
    default: {
      bl_assert (0 && "bug");
      }
    }
    ++partype;
  }
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_log(
  malc* l, malc_const_entry const* e, uword payload_size, ...
  )
{
#ifdef MALC_SAFETY_CHECK
  if (unlikely (
    !l ||
    !e ||
    !e->format ||
    !e->info
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
    e->info
    );
#endif
  alloc_tag tag;
  uword size  = sizeof (qnode) + payload_size;
  size       += (e->compressed_count + 1) & ~u_lsb_set (1);
  u8* mem     = nullptr;
  uword slots = div_ceil (size, alloc_slot_size);
  if (unlikely (slots) > (1 << (sizeof_member (qnode, slots) * 8))) {
    /*entries are limited at 8KB*/
    return bl_range;
  }
  bl_err err = memory_alloc (&l->mem, &mem, &tag, slots);
  if (unlikely (err)) {
    return err;
  }
  va_list vargs;
  va_start (vargs, payload_size);
  /*malc_encode  is deliberately unsafe, if the log macros are used everything
    will be correct. Checks have to be avoided on the fast-path*/
  malc_encode (mem + sizeof (qnode), vargs, e);
  va_end(vargs);
  qnode* n = (qnode*) mem;
  n->slots = slots - 1;
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

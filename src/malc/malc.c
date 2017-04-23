#include <malc/malc.h>

#include <bl/base/allocator.h>
#include <bl/base/static_integer_math.h>
#include <bl/base/utility.h>
#include <bl/nonblock/mpsc_i.h>

#include <malc/cfg.h>
#include <malc/memory.h>
/*----------------------------------------------------------------------------*/
struct malc {
  malc_producer_cfg producer;
  memory            mem;
  mpsc_i            q;
  alloc_tbl const*  alloc;
};
/*----------------------------------------------------------------------------*/
typedef struct qnode {
  mpsc_i_node hook;
  u8          slots;
}
qnode;
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
  return memory_tls_init (&l->mem, bytes, l->alloc);
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_run_consume_task (malc* l, uword timeout_us)
{
  return bl_ok;
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
  /* TODO: this is very preliminary, */
  bl_assert (e && e->format && e->info);
  uword     size  = va_max_size += sizeof (*e);
  u8*       mem   = nullptr;
  uword     slots = div_ceil (size, alloc_slot_size);
  alloc_tag tag   = alloc_tag_tls;
  bl_err    err   = memory_tls_alloc (&l->mem, &mem, slots);
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
  mpsc_i_node_set (&n->hook, nullptr, tag, alloc_tag_bits);
  mpsc_i_produce (&l->q, &n->hook, alloc_tag_bits);

#if 0
  va_list vargs;
  va_start (vargs, argc);
  char const* partype = &e->info[1];

  while (*partype) {
    switch (*partype) {
    case malc_type_float: {
      float v = (float) va_arg (vargs, double);
      printf ("float: %f\n", v);
      break;
      }
    case malc_type_double: {
      double v = (double) va_arg (vargs, double);
      printf ("double: %f\n", v);
      break;
      }
    case malc_type_i8: {
      i8 v = (i8) va_arg (vargs, int);
      printf ("i8: %"PRId8"\n", v);
      break;
      }
    case malc_type_u8: {
      u8 v = (u8) va_arg (vargs, int);
      printf ("u8: %"PRIu8"\n", v);
      break;
      }
    case malc_type_i16: {
      i16 v = (i16) va_arg (vargs, int);
      printf ("i16: %"PRId16"\n", v);
      break;
      }
    case malc_type_u16: {
      u16 v = (u16) va_arg (vargs, int);
      printf ("u16: %"PRIu16"\n", v);
      break;
      }
#ifdef BL_32
    case malc_type_i32: {
      i32 v = (i32) va_arg (vargs, i32);
      printf ("i32: %"PRId32"\n", v);
      break;
      }
    case malc_type_u32: {
      u32 v = (u32)  va_arg (vargs, u32);
      printf ("u32: %"PRIu32"\n", v);
      break;
      }
#else
      case malc_type_i32: {
      i32 v = (i32) va_arg (vargs, int);
      printf ("i32: %"PRId32"\n", v);
      break;
      }
    case malc_type_u32: {
      u32 v = (u32) va_arg (vargs, int);
      printf ("u32: %"PRIu32"\n", v);
      break;
      }
#endif
      case malc_type_i64: {
      i64 v = (i64) va_arg (vargs, i64);
      printf ("i64: %"PRId64"\n", v);
      break;
      }
    case malc_type_u64: {
      u64 v = (u64) va_arg (vargs, u64);
      printf ("u64: %"PRIu64"\n", v);
      break;
      }
    case malc_type_vptr: {
      void* v = va_arg (vargs, void*);
      printf ("vptr: 0x%08lx\n", (u64) v);
      break;
      }
    case malc_type_lit: {
      malc_lit v = va_arg (vargs, malc_lit);
      printf ("string literal: %s\n", v.lit);
      break;
      }
    case malc_type_str: {
      malc_str v = va_arg (vargs, malc_str);
      printf ("string: len: %d, %s\n", (int) v.len, v.str);
      break;
      }
    case malc_type_bytes: {
      malc_mem  v = va_arg (vargs, malc_mem);
      printf ("mem: len: %d, ptr: 0x%08lx\n", v.size, (u64) v.mem);
      break;
      }
    default: {
      printf ("invalid\n");
      err = bl_invalid;
      goto end_process_loop;
      }
    }
    ++partype;
  }
end_process_loop:
  va_end (vargs);
  return err;
  #endif
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
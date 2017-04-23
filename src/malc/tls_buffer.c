#ifndef __BL_TSS_BUFFER_H__
#define __BL_TSS_BUFFER_H__

#include <bl/base/error.h>
#include <bl/base/assert.h>
#include <bl/base/utility.h>
#include <bl/base/atomic.h>
#include <bl/base/static_integer_math.h>

#include <malc/tls_buffer.h>

/*----------------------------------------------------------------------------*/
static bl_err tls_buffer_alloc_priv(
  tls_buffer* t, u8** mem, u8* old, u32 expand, uword old_slots, bool realloc
  )
{
  bl_assert (expand != 0 && mem && t);
  /* this trivial SPSC algorithm relies on the fact that zero is a forbidden
     value on the first word of each allocated entry */
  bool copy_old = false;
  uword slots  = old_slots + expand;
  if (unlikely (slots > t->slot_count)) {
    return bl_invalid;
  }
  u8* slot_start = old;
  u8* slot_end   = old + (slots * t->slot_size);
  if (unlikely (slot_end >= t->mem_end)) {
    /* memory region wrapping */
    slot_start = mem;
    slot_end   = mem + (slots * t->slot_size);
    copy_old   = realloc;
  }
  u8* it = old + (old_slots * t->slot_size);
  while (it < slot_end) {
    if (unlikely (atomic_uword_load_rlx ((atomic_uword*) it) != 0)) {
      return bl_would_oveflow;
    }
    it += t->slot_size;
  }
  t->slot = slot_end;
  *mem     = slot_start;
  if (unlikely (copy_old)) {
    memcpy (*mem, old, old_slots * t->slot_size);
    tls_buffer_dealloc (old, old_slots, t->slot_size);
  }
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
bl_err tls_buffer_init(
  tls_buffer**     t,
  u32              slot_size_and_align,
  u32              slot_count,
  alloc_tbl const* alloc
  )
{
  bl_assert (is_pow2 (slot_size_and_align));
  uword allocsize = sizeof (tls_buffer) + slot_size_and_align;
  allocsize      += slot_size_and_align * slot_count;

  *t = (tls_buffer*) bl_alloc (alloc, allocsize);
  if (!*t) {
    return bl_alloc;
  }
  t->alloc       = alloc;
  t->slot_size  = slot_size_and_align;
  t->slot_count = slot_count;

  uword mem  = ((uword) *t) + sizeof (tls_buffer) + t->slotsize;
  mem       &= ~(((uword) t->slotsize) - 1);
  t->mem     = (u8*) mem;
  t->mem_end = t->mem + (slot_count * t->slot_size);
  t->slot    = t->mem;
  tls_buffer_dealloc (t->mem, slots, t->slot_size);
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
void bl_tss_dtor_callconv tls_buffer_destroy (void* opaque)
{
  if (opaque) {
    alloc_tbl const* alloc = ((tls_buffer*) opaque)->alloc;
    bl_dealloc (alloc, opaque);
  }
}
/*----------------------------------------------------------------------------*/
bl_err tls_buffer_alloc (tls_buffer* t, u8** mem, u32 slots)
{
  return tls_buffer_alloc_priv (r, mem, old, t->slot, slots, 0, false);
}
/*----------------------------------------------------------------------------*/
bl_err tls_buffer_expand(
  tls_buffer* t, u8** mem, u8* old, u32 expand_slots
  )
{
  return tls_buffer_alloc_priv(
    r, mem, old, expand_slots, ((uword) (t->slot - old)) / t->slot_size, true
    );
}
/*----------------------------------------------------------------------------*/
void tls_buffer_dealloc (void* mem, u32 slots, u32 slot_size)
{
  /* check right alignment */
  bl_assert ((((uword) mem) / slot_size) * slot_size == (uword) mem);
  u8* slot = (u8*) mem;
  u8* end   = slot + (slot_size * slots);
  for (; slot < end; slot += slot_size) {
    atomic_uword_store_rlx ((atomic_uword*) slot, 0);
  }
}
/*----------------------------------------------------------------------------*/
#endif
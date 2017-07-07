#ifndef __BL_TSS_BUFFER_H__
#define __BL_TSS_BUFFER_H__

#include <string.h>

#include <bl/base/error.h>
#include <bl/base/assert.h>
#include <bl/base/utility.h>
#include <bl/base/atomic.h>
#include <bl/base/static_integer_math.h>

#include <malc/tls_buffer.h>

/*----------------------------------------------------------------------------*/
static bl_thread_local void* malc_tls = nullptr;
/*----------------------------------------------------------------------------*/
void tls_buffer_thread_local_set (void* mem)
{
  /* Some GDB versions sefault on TLS var access, set breakpoints afterwards*/
  malc_tls = mem;
}
/*----------------------------------------------------------------------------*/
void* tls_buffer_thread_local_get (void)
{
  /* Some GDB versions sefault on TLS var access, set breakpoints afterwards*/
  return malc_tls;
}
/*----------------------------------------------------------------------------*/
bl_err tls_buffer_init(
  tls_buffer**     out,
  u32              slot_size_and_align,
  u32              slot_count,
  alloc_tbl const* alloc,
  tls_destructor   destructor_fn,
  void*            destructor_context
  )
{
  bl_assert (is_pow2 (slot_size_and_align));
  uword allocsize = sizeof (tls_buffer) + slot_size_and_align;
  allocsize      += slot_size_and_align * slot_count;

  tls_buffer* t = (tls_buffer*) bl_alloc (alloc, allocsize);
  if (!t) {
    return bl_alloc;
  }
  t->destructor_fn      = destructor_fn;
  t->destructor_context = destructor_context;
  t->slot_size          = slot_size_and_align;
  t->slot_count         = slot_count;

  uword mem  = ((uword) t) + sizeof (*t) + t->slot_size;
  mem       &= ~(((uword) t->slot_size) - 1);
  t->mem     = (u8*) mem;
  t->mem_end = t->mem + (slot_count * t->slot_size);
  t->slot    = t->mem;
  tls_buffer_dealloc (t->mem, slot_count, t->slot_size);
  *out = t;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
void bl_tss_dtor_callconv tls_buffer_out_of_scope_destroy (void* opaque)
{
  /* Some GDB versions sefault on TLS var access, set breakpoints afterwards*/
  if (malc_tls && (void*) malc_tls == opaque) {
    malc_tls      = nullptr;
    tls_buffer* t = (tls_buffer*) opaque;
    if (t && t->destructor_fn) {
      t->destructor_fn (opaque, t->destructor_context);
    }
  }
  /* Some GDB versions sefault on TLS var access, set breakpoints afterwards*/
  else if (malc_tls && (void*) malc_tls != opaque) {
    bl_assert (0 && "bad destruction (logger wrongly reinitialized)");
  }
}
/*----------------------------------------------------------------------------*/
bl_err tls_buffer_alloc (u8** mem, u32 slots)
{
  /* Some GDB versions sefault on TLS var access, set breakpoints afterwards*/
  tls_buffer* t = malc_tls;
  if (unlikely (!t)) {
    return bl_alloc;
  }
  bl_assert (slots != 0 && mem && t);
  if (unlikely (slots > t->slot_count)) {
    return bl_alloc;
  }
  /* Segfaults here are most likely caused by a thread enqueueing after the
  termination function has been called, which is forbidded but not enforced
  (enforcing it would require heavyweight synchronization on the fast-path) */
  u8* slot_start = t->slot;
  u8* slot_end   = t->slot + (slots * t->slot_size);

  if (unlikely (slot_end > t->mem_end)) {
    /* memory region wrapping */
    slot_start = t->mem;
    slot_end   = t->mem + (slots * t->slot_size);
  }
  u8* check_iter = slot_start;
  while (check_iter < slot_end) {
    uword first_word = atomic_uword_load_rlx ((atomic_uword*) check_iter);
    if (unlikely (first_word != TLS_BUFFER_FREE_UWORD)) {
      return bl_alloc;
    }
    check_iter += t->slot_size;
  }
  t->slot = slot_end;
  *mem    = slot_start;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
void tls_buffer_dealloc (void* mem, u32 slots, u32 slot_size)
{
  /* check right alignment */
  bl_assert ((((uword) mem) / slot_size) * slot_size == (uword) mem);
  u8* slot = (u8*) mem;
  u8* end   = slot + (slot_size * slots);
  for (; slot < end; slot += slot_size) {
    atomic_uword_store_rlx ((atomic_uword*) slot, TLS_BUFFER_FREE_UWORD);
  }
}
/*----------------------------------------------------------------------------*/
#endif
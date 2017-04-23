#ifndef __MALC_MEMORY__
#define __MALC_MEMORY__

#include <bl/base/thread.h>
#include <bl/base/assert.h>
#include <bl/base/utility.h>
#include <bl/base/integer_manipulation.h>
#include <bl/base/default_allocator.h>

#include <malc/cfg.h>
#include <malc/memory.h>
#include <malc/tls_buffer.h>

/* TODO: bounded queue. Needs a custom spmc with variable size pop */
/*----------------------------------------------------------------------------*/
bl_thread_local void* malc_tls = nullptr;
/*----------------------------------------------------------------------------*/
static bl_err memory_init (memory* m)
{
  bl_err err = bl_tss_init (&m->tss_key, &tls_buffer_destroy)
  if (err) {
    return err;
  }
  m->default_allocator  = get_default_alloc();
  m->cfg.heap_allocator = &m->default_allocator;
}
/*----------------------------------------------------------------------------*/
static void memory_destroy (memory* m)
{
  bl_tss_destroy (m->key);
}
/*----------------------------------------------------------------------------*/
static bl_err memory_tls_alloc (memory* m, u8** mem, u32 slots)
{
  if (unlikely (!malc_tls)) {
    return bl_invalid;
  }
  tls_buffer* t = (tls_buffer*) malc_tls;
  return tls_buffer_alloc (t, mem, slots);
}
/*----------------------------------------------------------------------------*/
static bl_err memory_tls_expand(
  memory* m, u8** mem, u8* oldmem, u32 expand_slots
  )
{
  bl_assert (malc_tls);
  tls_buffer* t = (tls_buffer*) malc_tls;
  return tls_buffer_expand (t, u8** mem, u8* oldmem, expand_slots);
}
/*----------------------------------------------------------------------------*/
static bl_err memory_tls_init (memory* m, u32 bytes, alloc_tbl const* alloc)
{
  if (malc_tls) {
    return bl_locked;
  }
  tls_buffer* t;
  u32 slots  = round_to_next_multiple (bytes, (u32) alloc_slot_size);
  bl_err err = tls_buffer_init (&t, alloc_slot_size, slots, alloc);
  if (err) {
    return err;
  }
  err = bl_tss_set (m->tss_key, t);
  if (err) {
    tls_buffer_destroy (t);
    return err;
  }
  malc_tls = t;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
static void memory_tls_dealloc (memory* m, u8* mem, u32 slots)
{
  tls_buffer_dealloc (mem, slots, alloc_slot_size);
}
/*----------------------------------------------------------------------------*/
static bl_err memory_alloc (memory* m, u8** mem, alloc_tag* tag, u32 bytes)
{
  if (m->cfg.heap_allocator) {
    *mem = bl_alloc (m->cfg.heap_allocator, bytes);
    *tag = alloc_tag_heap;
    return *mem ? bl_ok : bl_alloc;
  }
  return bl_would_overflow;
}
/*----------------------------------------------------------------------------*/
static void memory_dealloc (memory* m, u8* mem, alloc_tag tag, u32 bytes)
{
  bl_assert (tag == alloc_tag_heap);
  bl_dealloc (m->cfg.heap_allocator, mem);
}
/*----------------------------------------------------------------------------*/

#endif /* __MALC_MEMORY__ */
#ifndef __MALC_MEMORY__
#define __MALC_MEMORY__

#include <bl/base/thread.h>
#include <bl/base/assert.h>
#include <bl/base/utility.h>
#include <bl/base/integer_manipulation.h>

#include <malc/cfg.h>
#include <malc/memory.h>
#include <malc/tls_buffer.h>

declare_dynarray_funcs (mem_array, void*)
/*----------------------------------------------------------------------------*/
bl_err memory_init (memory* m, alloc_tbl const* alloc)
{
  bl_err err = bl_tss_init (&m->tss_key, &tls_buffer_out_of_scope_destroy);
  if (err) {
    return err;
  }
  m->cfg.msg_allocator = alloc;
  boundedb_init (&m->bb);
  mem_array_init_empty (&m->tss_list);
  malc_tls = nullptr; /* resetting TLS var, mostly done for the smoke tests */
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
void memory_destroy (memory* m, alloc_tbl const* alloc)
{
  bl_tss_destroy (m->tss_key);
  boundedb_destroy (&m->bb, alloc);
  mem_array_destroy (&m->tss_list, alloc);
}
/*----------------------------------------------------------------------------*/
bl_err memory_tls_init_unregistered(
  memory*          m,
  u32              bytes,
  alloc_tbl const* alloc,
  tls_destructor   destructor_fn,
  void*            destructor_context,
  void**           tls_buffer_addr
  )
{
  if (malc_tls) {
    return bl_locked;
  }
  tls_buffer* t;
  u32 slots  = round_to_next_multiple (bytes, (u32) alloc_slot_size);
  bl_err err = tls_buffer_init(
    &t, alloc_slot_size, slots, alloc, destructor_fn, destructor_context
    );
  if (err) {
    return err;
  }
  err = bl_tss_set (m->tss_key, t);
  if (err) {
    bl_dealloc (alloc, t);
    return err;
  }
  *tls_buffer_addr = (void*) t;
  malc_tls = t;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
bl_err memory_bounded_buffer_init (memory* m, alloc_tbl const* alloc)
{
  return boundedb_reset(
    &m->bb,
    alloc,
    m->cfg.fixed_allocator_bytes,
    alloc_slot_size,
    m->cfg.fixed_allocator_max_slots,
    m->cfg.fixed_allocator_per_cpu
    );
}
/*----------------------------------------------------------------------------*/
bl_err memory_tls_register (memory* m, void* mem, alloc_tbl const* alloc)
{
  dynarray_foreach (mem_array, void*, &m->tss_list, it) {
    if (*it == nullptr) {
      *it = mem;
      return bl_ok;
    }
  }
  bl_err err = mem_array_grow (&m->tss_list, 1, alloc);
  if (err) {
    return err;
  }
  *mem_array_last (&m->tss_list) = mem;
  return err;
}
/*----------------------------------------------------------------------------*/
bool memory_tls_destroy (memory* m, void* mem, alloc_tbl const* alloc)
{
  dynarray_foreach (mem_array, void*, &m->tss_list, it) {
    if (*it == mem) {
      bl_dealloc (alloc, mem);
      *it = nullptr;
      return true;
    }
  }
  return false;
}
/*----------------------------------------------------------------------------*/
void memory_tls_destroy_all (memory* m, alloc_tbl const* alloc)
{
  dynarray_foreach (mem_array, void*, &m->tss_list, it) {
    if (*it != nullptr) {
      bl_dealloc (alloc, *it);
      *it = nullptr;
    }
  }
}
/*----------------------------------------------------------------------------*/
bl_err memory_alloc (memory* m, u8** mem, alloc_tag* tag, u32 slots)
{
  bl_assert (m && mem && tag && slots);
  if (likely (malc_tls)) {
    tls_buffer* t = (tls_buffer*) malc_tls;
    if (likely (tls_buffer_alloc (t, mem, slots) == bl_ok)) {
      *tag = alloc_tag_tls;
      return bl_ok;
    }
  }
  bl_err err = bl_alloc;
  if (m->cfg.fixed_allocator_bytes > 0) {
    err = boundedb_alloc (&m->bb, mem, slots);
    if (likely (!err)) {
      return bl_ok;
    }
  }
  if (m->cfg.msg_allocator) {
    *mem = bl_alloc (m->cfg.msg_allocator, slots * alloc_slot_size);
    *tag = alloc_tag_heap;
    return *mem ? bl_ok : bl_alloc;
  }
  return err;
}
/*----------------------------------------------------------------------------*/
void memory_dealloc (memory* m, u8* mem, alloc_tag tag, u32 slots)
{
  switch (tag) {
  case alloc_tag_tls:
    tls_buffer_dealloc (mem, slots, alloc_slot_size);
    break;
  case alloc_tag_bounded:
    boundedb_dealloc (&m->bb, mem, slots);
    break;
  case alloc_tag_heap:
    bl_dealloc (m->cfg.msg_allocator, mem);
    break;
  default:
    bl_assert (false && "bug");
    break;
  }
}
/*----------------------------------------------------------------------------*/

#endif /* __MALC_MEMORY__ */
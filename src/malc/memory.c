#ifndef __MALC_MEMORY__
#define __MALC_MEMORY__

#include <string.h>

#include <bl/base/thread.h>
#include <bl/base/assert.h>
#include <bl/base/utility.h>
#include <bl/base/integer_manipulation.h>

#include <malc/common.h>
#include <malc/memory.h>
#include <malc/tls_buffer.h>

bl_declare_dynarray_funcs (mem_array, void*)
/*----------------------------------------------------------------------------*/
bl_err memory_init (memory* m, bl_alloc_tbl const* alloc)
{
  bl_err err = bl_tss_init (&m->tss_key, &tls_buffer_out_of_scope_destroy);
  if (err.bl) {
    return err;
  }
  m->cfg.msg_allocator             = alloc;
  m->cfg.slot_size                 = 64;
  m->cfg.fixed_allocator_bytes     = 0;
  m->cfg.fixed_allocator_max_slots = 0;
  m->cfg.fixed_allocator_per_cpu   = 0;
  boundedb_init (&m->bb);
  mem_array_init_empty (&m->tss_list);
  tls_buffer_thread_local_set (nullptr); /* for smoke testing mostly */
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
void memory_destroy (memory* m, bl_alloc_tbl const* alloc)
{
  bl_tss_destroy (m->tss_key);
  boundedb_destroy (&m->bb, alloc);
  mem_array_destroy (&m->tss_list, alloc);
}
/*----------------------------------------------------------------------------*/
bl_err memory_tls_init_unregistered(
  memory*             m,
  bl_u32              bytes,
  bl_alloc_tbl const* alloc,
  tls_destructor      destructor_fn,
  void*               destructor_context,
  void**              tls_buffer_addr
  )
{
  if (tls_buffer_thread_local_get ()) {
    return bl_mkerr (bl_locked);
  }
  tls_buffer* t;
  bl_u32 slots  = bl_div_ceil (bytes, (bl_u32) m->cfg.slot_size);
  bl_err err = tls_buffer_init(
    &t, m->cfg.slot_size, slots, alloc, destructor_fn, destructor_context
    );
  if (err.bl) {
    return err;
  }
  err = bl_tss_set (m->tss_key, t);
  if (err.bl) {
    bl_dealloc (alloc, t);
    return err;
  }
  *tls_buffer_addr = (void*) t;
  tls_buffer_thread_local_set ((void*) t);
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
bl_err memory_bounded_buffer_init (memory* m, bl_alloc_tbl const* alloc)
{
  return boundedb_reset(
    &m->bb,
    alloc,
    m->cfg.fixed_allocator_bytes,
    m->cfg.slot_size,
    m->cfg.fixed_allocator_max_slots,
    m->cfg.fixed_allocator_per_cpu
    );
}
/*----------------------------------------------------------------------------*/
bl_err memory_tls_register (memory* m, void* mem, bl_alloc_tbl const* alloc)
{
  bl_dynarray_foreach (mem_array, void*, &m->tss_list, it) {
    if (*it == nullptr) {
      *it = mem;
      return bl_mkok();
    }
  }
  bl_err err = mem_array_grow (&m->tss_list, 1, alloc);
  if (err.bl) {
    return err;
  }
  *mem_array_last (&m->tss_list) = mem;
  return err;
}
/*----------------------------------------------------------------------------*/
bool memory_tls_destroy (memory* m, void* mem, bl_alloc_tbl const* alloc)
{
  bl_dynarray_foreach (mem_array, void*, &m->tss_list, it) {
    if (*it == mem) {
      bl_dealloc (alloc, mem);
      *it = nullptr;
      return true;
    }
  }
  return false;
}
/*----------------------------------------------------------------------------*/
void memory_tls_destroy_all (memory* m, bl_alloc_tbl const* alloc)
{
  bl_dynarray_foreach (mem_array, void*, &m->tss_list, it) {
    if (*it != nullptr) {
      bl_dealloc (alloc, *it);
      *it = nullptr;
    }
  }
}
/*----------------------------------------------------------------------------*/
extern bl_err memory_tls_try_run_destructor (memory* m)
{
  tls_buffer_out_of_scope_destroy (tls_buffer_thread_local_get());
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
bl_err memory_alloc (memory* m, bl_u8** mem, alloc_tag* tag, bl_u32 slots)
{
  bl_assert (m && mem && tag && slots);
  bl_err err = tls_buffer_alloc (mem, slots);
  if (bl_likely (!err.bl)) {
    *tag = alloc_tag_tls;
    return bl_mkok();
  }
  if (m->cfg.fixed_allocator_bytes > 0) {
    err = boundedb_alloc (&m->bb, mem, slots);
    *tag = alloc_tag_bounded;
    if (bl_likely (!err.bl)) {
      return bl_mkok();
    }
  }
  if (m->cfg.msg_allocator) {
    *mem = bl_alloc (m->cfg.msg_allocator, slots * m->cfg.slot_size);
    *tag = alloc_tag_heap;
    return bl_mkerr (*mem ? bl_ok : bl_alloc);
  }
  return err;
}
/*----------------------------------------------------------------------------*/
void memory_dealloc (memory* m, bl_u8* mem, alloc_tag tag, bl_u32 slots)
{
  switch (tag) {
  case alloc_tag_tls:
    tls_buffer_dealloc (mem, slots, m->cfg.slot_size);
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
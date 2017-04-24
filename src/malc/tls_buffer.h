#ifndef __MALC_TLS_BUFFER_H__
#define __MALC_TLS_BUFFER_H__

#include <bl/base/integer.h>
#include <bl/base/allocator.h>
#include <bl/base/error.h>
#include <bl/base/thread.h>

/*----------------------------------------------------------------------------*/
extern bl_thread_local void* malc_tls;
/*----------------------------------------------------------------------------*/
typedef void (*tls_destructor) (void* mem, void* context);
/*----------------------------------------------------------------------------*/
typedef struct tls_buffer {
  tls_destructor   destructor_fn;
  void*            destructor_context;
  u8*              mem;
  u8*              mem_end;
  u8*              slot;
  uword            slot_count;
  uword            slot_size;
}
tls_buffer;
/*----------------------------------------------------------------------------*/
extern bl_err tls_buffer_init(
  tls_buffer**     t,
  u32              slot_size_and_align,
  u32              slot_count,
  alloc_tbl const* alloc,
  tls_destructor   destructor_fn,
  void*            destructor_context
  );
/*----------------------------------------------------------------------------*/
extern void bl_tss_dtor_callconv tls_buffer_destroy (void* opaque);
/*----------------------------------------------------------------------------*/
extern bl_err tls_buffer_alloc (tls_buffer* t, u8** mem, u32 slots);
/*----------------------------------------------------------------------------*/
extern bl_err tls_buffer_expand(
  tls_buffer* t, u8** mem, u8* old, u32 expand_slots
  );
/*----------------------------------------------------------------------------*/
extern void tls_buffer_dealloc (void* mem, u32 slots, u32 slot_size);
/*----------------------------------------------------------------------------*/

#endif
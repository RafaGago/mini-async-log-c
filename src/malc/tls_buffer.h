#ifndef __BL_TSS_BUFFER_H__
#define __BL_TSS_BUFFER_H__

#include <bl/base/integer.h>
#include <bl/base/allocator.h>
#include <bl/base/error.h>
#include <bl/base/thread.h>

/*----------------------------------------------------------------------------*/
typedef struct tls_buffer {
  alloc_tbl const* alloc;
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
  alloc_tbl const* alloc
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
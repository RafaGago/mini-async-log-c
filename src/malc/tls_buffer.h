#ifndef __MALC_TLS_BUFFER_H__
#define __MALC_TLS_BUFFER_H__

#include <bl/base/integer.h>
#include <bl/base/allocator.h>
#include <bl/base/error.h>
#include <bl/base/thread.h>

/* This trivial (but very specialized) SPSC algorithm relies on
   TLS_BUFFER_FREE_UWORD being a forbidden value on the first bl_word of then
   allocated buffer data, and that the deallocating thread will know the
   size (in slots) of the allocation.

   TLS_BUFFER_FREE_UWORD is set to 1, so the first value on the memory contents
   can be a pointer on the target OS architectures. Don't use tagged pointers
   that can set a ptr to 1 when using this.

   This makes it a suitable allocator for intrusive list nodes provided that the
   first bl_word contains the pointer (the allocation size has to be stored on the
   node too to be able to deallocate) and the slot count is stored elsewhere.
   */

#define TLS_BUFFER_FREE_UWORD ((bl_uword) 1)
/*----------------------------------------------------------------------------*/
typedef void (*tls_destructor) (void* mem, void* context);
/*----------------------------------------------------------------------------*/
typedef struct tls_buffer {
  tls_destructor   destructor_fn;
  void*            destructor_context;
  bl_u8*           mem;
  bl_u8*           mem_end;
  bl_u8*           slot;
  bl_uword         slot_count;
  bl_uword         slot_size;
}
tls_buffer;
/*----------------------------------------------------------------------------*/
extern bl_err tls_buffer_init(
  tls_buffer**        t,
  bl_u32              slot_size_and_align,
  bl_u32              slot_count,
  bl_alloc_tbl const* alloc,
  tls_destructor      destructor_fn, /* executed when out of scope */
  void*               destructor_context /* will be passed to "destructor_fn" */
  );
/*----------------------------------------------------------------------------*/
extern void tls_buffer_thread_local_set (void* mem);
/*----------------------------------------------------------------------------*/
extern void* tls_buffer_thread_local_get (void);
/*----------------------------------------------------------------------------*/
extern void bl_tss_dtor_callconv tls_buffer_out_of_scope_destroy (void* opaque);
/*----------------------------------------------------------------------------*/
extern bl_err tls_buffer_alloc (bl_u8** mem, bl_u32 slots);
/*----------------------------------------------------------------------------*/
extern void tls_buffer_dealloc (void* mem, bl_u32 slots, bl_u32 slot_size);
/*----------------------------------------------------------------------------*/

#endif
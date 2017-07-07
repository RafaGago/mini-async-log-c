#ifndef __MALC_TLS_BUFFER_H__
#define __MALC_TLS_BUFFER_H__

#include <bl/base/integer.h>
#include <bl/base/allocator.h>
#include <bl/base/error.h>
#include <bl/base/thread.h>

/* this trivial (but very specialized) SPSC algorithm relies on
   TLS_BUFFER_FREE_UWORD being a forbidded value on the first word of an
   allocated buffer and that the deallocating thread will know the
   size.

   TLS_BUFFER_FREE_UWORD is set to 1, so the first value on the chunk can be
   a pointer. 1 is a value that will never be returned by the OS. Don't use
   tagged pointers that can set a ptr to 1 when using this.

   This makes it a suitable allocator for intrusive list nodes provided that the
   first word contains the pointer (the allocation size has to be stored on the
   node too to be able to deallocate).
   */

#define TLS_BUFFER_FREE_UWORD ((uword) 1)
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
extern void tls_buffer_thread_local_set (void* mem);
/*----------------------------------------------------------------------------*/
extern void* tls_buffer_thread_local_get (void);
/*----------------------------------------------------------------------------*/
extern void bl_tss_dtor_callconv tls_buffer_out_of_scope_destroy (void* opaque);
/*----------------------------------------------------------------------------*/
extern bl_err tls_buffer_alloc (u8** mem, u32 slots);
/*----------------------------------------------------------------------------*/
extern void tls_buffer_dealloc (void* mem, u32 slots, u32 slot_size);
/*----------------------------------------------------------------------------*/

#endif
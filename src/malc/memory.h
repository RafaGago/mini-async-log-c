#ifndef __MALC_ALLOCATOR__
#define __MALC_ALLOCATOR__

#include <bl/base/integer.h>
#include <bl/base/allocator.h>
#include <bl/base/error.h>
#include <bl/base/static_integer_math.h>

#include <malc/cfg.h>

/*----------------------------------------------------------------------------*/
extern const uword alloc_slot_size      = 64;
extern const uword alloc_slot_size_log2 = static_log2_ceil_u (alloc_slot_size);
/*----------------------------------------------------------------------------*/
enum alloc_tags {
  alloc_tag_free    = 0,
  alloc_tag_tls     = 1,
  alloc_tag_heap    = 2,
  alloc_tag_bounded = 3,
  alloc_tag_total   = 4,
};
typedef u8 alloc_tag;
extern const uword alloc_tag_bits = static_log2_ceil_u (alloc_tag_total);
/*----------------------------------------------------------------------------*/
typedef struct memory {
  malc_aloc_cfg cfg;
  alloc_tbl     default_allocator;
  bl_tss        tss_key;
}
memory;
/*----------------------------------------------------------------------------*/
extern bl_err memory_init (memory* m);
/*----------------------------------------------------------------------------*/
extern void memory_destroy (memory* m);
/*----------------------------------------------------------------------------*/
extern bl_err memory_tls_alloc (memory* m, u8** mem, u32 slots);
/*----------------------------------------------------------------------------*/
extern bl_err memory_tls_expand(
  memory* m, u8** mem, u8* oldmem, u32 expand_slots
  );
/*----------------------------------------------------------------------------*/
extern void memory_tls_dealloc (memory* m, u8* mem, u32 slots);
/*----------------------------------------------------------------------------*/
extern bl_err memory_tls_init (memory* m, u32 bytes, alloc_tbl const* alloc);
/*----------------------------------------------------------------------------*/
extern bl_err memory_alloc (memory* m, u8** mem, alloc_tag* tag, u32 bytes);
/*----------------------------------------------------------------------------*/
extern void memory_dealloc (memory* m, u8* mem, alloc_tag tag);
/*----------------------------------------------------------------------------*/

#endif /* __MALC_ALLOCATOR__ */
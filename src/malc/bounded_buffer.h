#ifndef __MALC_FIXED_BUFFER_H__
#define __MALC_FIXED_BUFFER_H__

#include <bl/base/dynarray.h>
#include <bl/nonblock/mpmc_bpm.h>

bl_define_dynarray_types(cpuq, bl_mpmc_bpm)

/*----------------------------------------------------------------------------*/
typedef struct boundedb {
  cpuq queues;
}
boundedb;
/*---------------------------------------------------------------------------*/
extern void boundedb_init (boundedb* b);
/*---------------------------------------------------------------------------*/
bl_err boundedb_reset(
  boundedb*           b,
  bl_alloc_tbl const* alloc,
  bl_u32              bytes,
  bl_u32              slot_size,
  bl_u32              max_slots,
  bool                per_cpu
  );
/*---------------------------------------------------------------------------*/
void boundedb_destroy (boundedb* b, bl_alloc_tbl const* alloc);
/*---------------------------------------------------------------------------*/
bl_err boundedb_alloc (boundedb* b, bl_u8** mem, bl_u32 slots);
/*---------------------------------------------------------------------------*/
void boundedb_dealloc (boundedb* b, bl_u8* mem, bl_u32 slots);
/*---------------------------------------------------------------------------*/

#endif

#include <string.h>

#include <bl/base/integer.h>
#include <bl/base/thread.h>
#include <bl/base/assert.h>

#include <malc/bounded_buffer.h>

/* Preliminary implementation based on a modded D.Vjukov mpmc type of queue */

declare_dynarray_funcs (cpuq, mpmc_bpm)

/*---------------------------------------------------------------------------*/
typedef struct b_cpu_data {
  u16 cpu;
  u16 calls;
}
b_cpu_data;
/*---------------------------------------------------------------------------*/
static bl_thread_local b_cpu_data b_cpu = { 0, 0 };
/*---------------------------------------------------------------------------*/
void boundedb_init (boundedb* b)
{
  memset (b, 0, sizeof *b);
}
/*---------------------------------------------------------------------------*/
bl_err boundedb_reset(
  boundedb*        b,
  alloc_tbl const* alloc,
  u32              bytes,
  u32              slot_size,
  u32              max_slots,
  bool             per_cpu
  )
{
  bl_assert (b && alloc);
  bl_err err  = bl_ok;
  uword slot_count = div_ceil (bytes, slot_size);
  if (slot_count == 0) {
    err = bl_ok;
    goto do_destroy;
  }
  if (max_slots == 0) {
    return bl_invalid;
  }
  boundedb_destroy (b, alloc);
  uword count = per_cpu ? bl_get_cpu_count() : 1;
  for (uword i = 0; i < count; ++i) {
    err = cpuq_grow (&b->queues, 1, alloc);
    if (err) {
      goto do_destroy;
    }
    mpmc_bpm* q = cpuq_at (&b->queues, i);
    err  = mpmc_bpm_init(
      q, alloc, slot_count, max_slots, slot_size, slot_size, false
      );
    if (err) {
      goto do_destroy;
    }
  }
  return err;
do_destroy:
  boundedb_destroy (b, alloc);
  return err;
}
/*---------------------------------------------------------------------------*/
void boundedb_destroy (boundedb* b, alloc_tbl const* alloc)
{
  for (uword i = 0; i < cpuq_size (&b->queues); ++i) {
    mpmc_bpm_destroy (cpuq_at (&b->queues, i), alloc);
  }
  cpuq_destroy (&b->queues, alloc);
}
/*---------------------------------------------------------------------------*/
bl_err boundedb_alloc (boundedb* b, u8** mem, u32 slots)
{
  uword size = cpuq_size (&b->queues);
  bl_assert (size);
  uword qidx = 0;
  if (size > 1) {
    if ((b_cpu.calls & u_lsb_set (6)) == 0) {
      b_cpu.cpu = bl_get_cpu();
    }
    ++b_cpu.calls;
    qidx = b_cpu.cpu;
  }
  *mem = mpmc_bpm_alloc (cpuq_at (&b->queues, qidx), slots);
  return *mem ? bl_ok : bl_alloc;
}
/*---------------------------------------------------------------------------*/
void boundedb_dealloc (boundedb* b, u8* mem, u32 slots)
{
  for (uword i = 0; i < cpuq_size (&b->queues); ++i) {
    mpmc_bpm* q = cpuq_at (&b->queues, i);
    if (mpmc_bpm_allocation_is_in_range (q, mem)) {
      mpmc_bpm_dealloc (q, mem, slots);
      return;
    }
  }
  bl_assert (false);
}
/*---------------------------------------------------------------------------*/

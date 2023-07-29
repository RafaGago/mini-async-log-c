#include <string.h>

#include <bl/base/thread.h>
#include <bl/base/assert.h>

#include <malc/bounded_buffer.h>

/* Preliminary implementation based on a modded D.Vjukov bl_mpmc type of queue */

bl_declare_dynarray_funcs (cpuq, bl_mpmc_bpm)

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
  boundedb*           b,
  bl_alloc_tbl const* alloc,
  u32                 bytes,
  u32                 slot_size,
  u32                 max_slots,
  bool                per_cpu
  )
{
  bl_assert (b && alloc);
  bl_err err  = bl_mkok();
  uword slot_count = bl_div_ceil (bytes, slot_size);
  if (slot_count == 0) {
    err = bl_mkok();
    goto do_destroy;
  }
  if (max_slots == 0) {
    return bl_mkerr (bl_invalid);
  }
  boundedb_destroy (b, alloc);
  uword count = per_cpu ? bl_get_cpu_count() : 1;
  for (uword i = 0; i < count; ++i) {
    err = cpuq_grow (&b->queues, 1, alloc);
    if (err.own) {
      goto do_destroy;
    }
    bl_mpmc_bpm* q = cpuq_at (&b->queues, i);
    err  = bl_mpmc_bpm_init(
      q, alloc, slot_count, max_slots, slot_size, 16, false
      );
    if (err.own) {
      goto do_destroy;
    }
  }
  return err;
do_destroy:
  boundedb_destroy (b, alloc);
  return err;
}
/*---------------------------------------------------------------------------*/
void boundedb_destroy (boundedb* b, bl_alloc_tbl const* alloc)
{
  for (uword i = 0; i < cpuq_size (&b->queues); ++i) {
    bl_mpmc_bpm_destroy (cpuq_at (&b->queues, i), alloc);
  }
  cpuq_destroy (&b->queues, alloc);
}
/*---------------------------------------------------------------------------*/
bl_err boundedb_alloc (
  boundedb* b, u8** mem, u32* slots, u32 n_bytes, u32 max_n_slots
  )
{
  uword size = cpuq_size (&b->queues);
  bl_assert (size);
  uword qidx = 0;
  if (size > 1) {
    if ((b_cpu.calls & bl_u_lsb_set (6)) == 0) {
      b_cpu.cpu = bl_get_cpu();
    }
    ++b_cpu.calls;
    qidx = b_cpu.cpu;
  }
  bl_mpmc_bpm* q = cpuq_at (&b->queues, qidx);
  *slots = bl_mpmc_bpm_required_slots (q, n_bytes);
  if (*slots > max_n_slots) {
    return bl_mkerr (bl_range);
  }
  *mem = bl_mpmc_bpm_alloc (q, *slots);
  return bl_mkerr (*mem ? bl_ok : bl_alloc);
}
/*---------------------------------------------------------------------------*/
void boundedb_dealloc (boundedb* b, u8* mem, u32 slots)
{
  for (uword i = 0; i < cpuq_size (&b->queues); ++i) {
    bl_mpmc_bpm* q = cpuq_at (&b->queues, i);
    if (bl_mpmc_bpm_allocation_is_in_range (q, mem)) {
      bl_mpmc_bpm_dealloc (q, mem, slots);
      return;
    }
  }
  bl_assert (false);
}
/*---------------------------------------------------------------------------*/

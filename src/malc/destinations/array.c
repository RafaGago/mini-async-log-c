#include <string.h>

#include <bl/base/utility.h>
#include <bl/base/error.h>
#include <bl/base/assert.h>

#include <malc/destinations/array.h>

/*----------------------------------------------------------------------------*/
struct malc_array_dst {
  char*  mem;
  size_t mem_entries;
  size_t entry_chars;
  size_t tail;
  size_t size;
};
/*----------------------------------------------------------------------------*/
static bl_err malc_array_dst_init (void* instance, bl_alloc_tbl const* alloc)
{
  malc_array_dst* d = (malc_array_dst*) instance;
  memset (d, 0, sizeof *d);
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static bl_err malc_array_dst_write(
    void* instance, bl_u64 nsec, unsigned sev_val, malc_log_strings const* strs
    )
{
  malc_array_dst* d = (malc_array_dst*) instance;
  bl_assert (d->mem && d->mem_entries > 1 && d->entry_chars > 1);
  size_t idx       = 0;
  size_t eoffset   = d->tail * d->entry_chars;
  size_t entry_len = d->entry_chars - 1;
  size_t cp;
  cp   = bl_min (entry_len, strs->timestamp_len);
  memcpy (&d->mem[eoffset + idx], strs->timestamp, cp);
  idx += cp;
  cp   = bl_min (entry_len - idx, strs->sev_len);
  memcpy (&d->mem[eoffset + idx], strs->sev, cp);
  idx += cp;
  cp   = bl_min (entry_len - idx, strs->text_len);
  memcpy (&d->mem[eoffset + idx], strs->text, cp);
  idx += cp;
  d->mem[eoffset + idx] = 0;
  ++d->tail;
  d->tail %= d->mem_entries;
  d->mem[d->tail * d->entry_chars] = 0;
  ++d->size;
  d->size = bl_min (d->size, d->mem_entries - 1);
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT const struct malc_dst malc_array_dst_tbl = {
  sizeof (malc_array_dst), /*size_of*/
  &malc_array_dst_init,
  nullptr,                 /* terminate */
  nullptr,                 /* flush */
  nullptr,                 /* idle task */
  &malc_array_dst_write
};
/*----------------------------------------------------------------------------*/
MALC_EXPORT void malc_array_dst_set_array(
    malc_array_dst* d, char* mem, size_t mem_entries, size_t entry_chars
    )
{
  bl_assert (d);
  d->mem         = mem;
  d->mem_entries = mem_entries;
  d->entry_chars = entry_chars;
  d->tail        = 0;
  d->size        = 0;
  memset (mem, 0, mem_entries * entry_chars);
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT size_t malc_array_dst_size (malc_array_dst const* d)
{
  return d->size;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT size_t malc_array_dst_capacity (malc_array_dst const* d)
{
  return d->mem_entries - 1;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT char const* malc_array_dst_get_entry(
  malc_array_dst const* d, size_t idx
  )
{
  if (bl_likely(
    d->mem &&
    d->mem_entries != 0 &&
    d->entry_chars != 0 &&
    idx < d->size
    )) {
    bl_uword pos = (d->tail - d->size + idx) % d->mem_entries;
    return &d->mem[pos * d->entry_chars];
  }
  return nullptr;
}
/*----------------------------------------------------------------------------*/

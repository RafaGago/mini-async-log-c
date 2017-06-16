#include <string.h>

#include <bl/base/utility.h>
#include <bl/base/error.h>
#include <bl/base/assert.h>

#include <malc/destinations/array.h>

/*----------------------------------------------------------------------------*/
struct malc_array_dst {
  char* mem;
  uword mem_entries;
  uword entry_chars;
  uword tail;
  uword size;
};
/*----------------------------------------------------------------------------*/
bl_err malc_array_dst_init (void* instance, alloc_tbl const* alloc)
{
  malc_array_dst* d = (malc_array_dst*) instance;
  memset (d, 0, sizeof *d);
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
static bl_err malc_array_dst_write(
    void*       instance,
    tstamp      now,
    char const* timestamp,
    uword       timestamp_len,
    char const* severity,
    uword       severity_len,
    char const* text,
    uword       text_len
    )
{
  malc_array_dst* d = (malc_array_dst*) instance;
  bl_assert (d->mem && d->mem_entries > 1 && d->entry_chars > 1);
  uword idx       = 0;
  uword eoffset   = d->tail * d->entry_chars;
  uword entry_len = d->entry_chars - 1;
  uword cp;
  cp   = bl_min (entry_len, timestamp_len);
  memcpy (&d->mem[eoffset + idx], timestamp, cp);
  idx += cp;
  cp   = bl_min (entry_len - idx, severity_len);
  memcpy (&d->mem[eoffset + idx], severity, cp);
  idx += cp;
  cp   = bl_min (entry_len - idx, text_len);
  memcpy (&d->mem[eoffset + idx], text, cp);
  idx += cp;
  d->mem[eoffset + idx] = 0;
  ++d->tail;
  d->tail %= d->mem_entries;
  d->mem[d->tail * d->entry_chars] = 0;
  ++d->size;
  d->size = bl_min (d->size, d->mem_entries - 1);
  return bl_ok;
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
    malc_array_dst* d, char* mem, uword mem_entries, uword entry_chars
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
MALC_EXPORT uword malc_array_dst_size (malc_array_dst* d)
{
  return d->size;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT uword malc_array_dst_capacity (malc_array_dst* d)
{
  return d->mem_entries - 1;
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT char const* malc_array_dst_get_entry (malc_array_dst* d, uword idx)
{
  if (likely(
    d->mem &&
    d->mem_entries != 0 &&
    d->entry_chars != 0 &&
    idx < d->size
    )) {
    uword pos = (d->tail - d->size + idx) % d->mem_entries;
    return &d->mem[pos * d->entry_chars];
  }
  return nullptr;
}
/*----------------------------------------------------------------------------*/

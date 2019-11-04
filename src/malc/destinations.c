#include <stdio.h>
#include <string.h>

#include <malc/destinations.h>

#include <bl/base/assert.h>
#include <bl/base/deadline.h>
#include <bl/base/time.h>
#include <bl/base/utility.h>
#include <bl/base/static_integer_math.h>
#include <bl/base/integer_math.h>

bl_define_ringb_funcs (past_entries, past_entry)

#define DEFAULT_SEVERITY malc_sev_warning
/*----------------------------------------------------------------------------*/
#define MAX_ALIGN    8 /* correct almost always TODO: add in base_library*/
/*----------------------------------------------------------------------------*/
#define SIZEOF_DEST_MAX_ALIGN\
  (bl_round_to_next_multiple (sizeof (destination), MAX_ALIGN))
/*----------------------------------------------------------------------------*/
#define FOREACH_DESTINATION(mem, dest)\
  for ((dest) = (destination*) (mem);\
       (dest);\
       (dest) = (dest)->next_offset != 0 ?\
        (destination*) ((bl_u8*) (dest) + (dest)->next_offset) :\
        nullptr\
       )
/*----------------------------------------------------------------------------*/
static inline void* destination_get_instance (destination* d)
{
  return (void*) ((bl_u8*) d + SIZEOF_DEST_MAX_ALIGN);
}
/*----------------------------------------------------------------------------*/
static inline void update_severity_from_file (malc_dst_cfg* c)
{
  FILE* f = fopen (c->severity_file_path, "rb");
  if (!f) {
    return;
  }
  char buff[16];
  memset (buff, 0, sizeof buff);
  (void) fread (buff, 1, sizeof buff, f);
  if (ferror (f)) {
    goto done;
  }
  if (bl_lit_strcmp (buff, "debug") == 0) {
    c->severity = malc_sev_debug;
  }
  else if (bl_lit_strcmp (buff, "trace") == 0) {
    c->severity = malc_sev_trace;
  }
  else if (bl_lit_strcmp (buff, "note") == 0) {
    c->severity = malc_sev_note;
  }
  else if (bl_lit_strcmp (buff, "warning") == 0) {
    c->severity = malc_sev_warning;
  }
  else if (bl_lit_strcmp (buff, "error") == 0) {
    c->severity = malc_sev_error;
  }
  else if (bl_lit_strcmp (buff, "critical") == 0) {
    c->severity = malc_sev_critical;
  }
done:
  fclose (f);
}
/*----------------------------------------------------------------------------*/
void destinations_init (destinations* d, bl_alloc_tbl const* alloc)
{
  memset (d, 0, sizeof *d);
  d->alloc        = alloc;
  d->min_severity = DEFAULT_SEVERITY;
  d->filter_min_severity = malc_sev_note;
}
/*----------------------------------------------------------------------------*/
void destinations_destroy (destinations* d)
{
  (void) destinations_terminate (d);
  d->alloc = nullptr;
}
/*----------------------------------------------------------------------------*/
bl_err destinations_add (destinations* d, bl_u32* dest_id, malc_dst const* dst)
{
  bl_assert (dest_id && dst);
  if (bl_unlikely (!dst || !dst->write)) {
    return bl_mkerr (bl_invalid);
  }
  bl_uword size = SIZEOF_DEST_MAX_ALIGN +
    bl_round_to_next_multiple (dst->size_of, MAX_ALIGN);
  /* stored contiguosly because "write" is going to be called at data-rate */
  void* mem  = bl_realloc (d->alloc, d->mem, d->size + size);
  if (!mem) {
    return bl_mkerr (bl_alloc);
  }
  d->mem = mem;
  destination* dest = (destination*) (((bl_u8*) d->mem) + d->size);
  dest->next_offset = 0;
  dest->dst         = *dst;

  dest->cfg.show_timestamp = true;
  dest->cfg.show_severity  = true;
  dest->cfg.severity       = DEFAULT_SEVERITY;
  dest->cfg.severity_file_path   = nullptr;
  dest->cfg.log_rate_filter_time = 0;

  if (dst->init) {
    bl_err err = dst->init (destination_get_instance (dest), d->alloc);
    if (err.bl) {
      /* memory chunk is left as-is without shrinking it */
      return err;
    }
  }
  if (d->count) {
    destination* last = nullptr;
    FOREACH_DESTINATION (d->mem, dest) {
      last = dest;
    }
    bl_assert (last);
    last->next_offset = (bl_u8*) d->mem + d->size - (bl_u8*) last;
  }
  *dest_id = d->count;
  ++d->count;
  d->size += size;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static bl_err destinations_set_rate_limit_settings_impl(
  destinations* d, malc_security const* sec, bool validate_only
  )
{
  bool enabled = sec->log_rate_filter_watch_count != 0;
  if (enabled) {
    destination* dest;
    enabled = false;
    FOREACH_DESTINATION (d->mem, dest) {
      if (dest->cfg.log_rate_filter_time != 0) {
        enabled = true;
        break;
      }
    }
  }
  bl_uword wc = 0;
  if (enabled) {
    wc = bl_round_next_pow2_u32 (sec->log_rate_filter_watch_count);
    if (wc > bl_arr_elems_member (destinations, pe_buffer)) {
      return bl_mkerr (bl_invalid);
    }
    if (!malc_is_valid_severity (sec->log_rate_filter_min_severity)) {
      return bl_mkerr (bl_invalid);
    }
  }
  if (validate_only) {
    return bl_mkok();
  }
  if (!enabled) {
    d->filter_watch_count = 0;
    return bl_mkok();
  }
  d->filter_watch_count  = wc;
  d->filter_max_time     = 0;
  d->filter_min_severity = sec->log_rate_filter_min_severity;

  destination* dest;
  FOREACH_DESTINATION (d->mem, dest) {
    d->filter_max_time =
      bl_max (d->filter_max_time, dest->cfg.log_rate_filter_time);
  }
  return past_entries_init_extern (&d->pe, d->pe_buffer, wc);
}
/*----------------------------------------------------------------------------*/
bl_err destinations_validate_rate_limit_settings(
  destinations* d, malc_security const* sec
  )
{
  return destinations_set_rate_limit_settings_impl (d, sec, true);
}
/*----------------------------------------------------------------------------*/
bl_err destinations_set_rate_limit_settings(
  destinations* d, malc_security const* sec
  )
{
  return destinations_set_rate_limit_settings_impl (d, sec, false);
}
/*----------------------------------------------------------------------------*/
void destinations_get_rate_limit_settings(
  destinations const* d, malc_security* sec
  )
{
  sec->log_rate_filter_watch_count  = d->filter_watch_count;
  sec->log_rate_filter_min_severity = d->filter_min_severity;
}
/*----------------------------------------------------------------------------*/
void destinations_terminate (destinations* d)
{
  destination* dest;
  FOREACH_DESTINATION (d->mem, dest) {
    if (dest->dst.flush) {
      dest->dst.flush (destination_get_instance (dest));
    }
    if (dest->dst.terminate) {
      dest->dst.terminate (destination_get_instance (dest));
    }
  }
  bl_dealloc (d->alloc, d->mem);
  d->mem   = nullptr;
  d->size  = 0;
  d->count = 0;
}
/*----------------------------------------------------------------------------*/
static void entry_filter_idle (destinations* d, bl_timept64 now)
{
  for (bl_uword i = 0; i < past_entries_size (&d->pe); ++i) {
    past_entry* pe = past_entries_at (&d->pe, i);
    bl_timept64 t_allowed = pe->tprev + d->filter_max_time;
    if (bl_unlikely (bl_timept64_get_diff (now, t_allowed) >= 0)) {
      past_entries_drop (&d->pe, i);
      --i;
    }
  }
  if (past_entries_size (&d->pe) == 0) {
    bl_ringb_set_start_position (&d->pe, 0);
  }
}
/*----------------------------------------------------------------------------*/
void destinations_idle_task (destinations* d, bl_timept64 now)
{
  entry_filter_idle (d, now);
  destination* dest;
  FOREACH_DESTINATION (d->mem, dest) {
    if (dest->cfg.severity_file_path) {
      update_severity_from_file (&dest->cfg);
    }
    if (dest->dst.idle_task) {
      (void) dest->dst.idle_task (destination_get_instance (dest));
    }
  }
}
/*----------------------------------------------------------------------------*/
void destinations_flush (destinations* d)
{
  destination* dest;
  FOREACH_DESTINATION (d->mem, dest) {
    if (dest->dst.flush) {
      (void) dest->dst.flush (destination_get_instance (dest));
    }
  }
}
/*----------------------------------------------------------------------------*/
static malc_log_strings get_entry_strings(
  destination* dest, malc_log_strings const* strs
  )
{
  malc_log_strings s = *strs;
  s.timestamp_len = dest->cfg.show_timestamp ? strs->timestamp_len : 0;
  s.sev_len    = dest->cfg.show_severity ? strs->sev_len : 0;
  return s;
}
/*----------------------------------------------------------------------------*/
void destinations_write(
  destinations*           d,
  bl_uword                   entry_id,
  bl_timept64                  now,
  bl_uword                   sev,
  malc_log_strings const* strs
  )
{
  destination* dest;
  if (d->filter_watch_count == 0 || sev < d->filter_min_severity) {
    /*filtering inactive*/
    FOREACH_DESTINATION (d->mem, dest) {
      bl_assert (dest->dst.write);
      if (sev >= dest->cfg.severity) {
        malc_log_strings s = get_entry_strings (dest, strs);
        (void) dest->dst.write (destination_get_instance (dest), now, sev, &s);
      }
    }
  }
  else {
    /*filtering active*/
    past_entry* pe = nullptr;
    /* small contiguous cache-friendly structure: linear search */
    for (bl_uword i = 0; i < past_entries_size (&d->pe); ++i) {
      past_entry* e = past_entries_at (&d->pe, i);
      if (e->entry_id == entry_id) {
        pe = e;
        break;
      }
    }
    FOREACH_DESTINATION (d->mem, dest) {
      bl_assert (dest->dst.write);
      if (sev >= dest->cfg.severity) {
        if (pe && dest->cfg.log_rate_filter_time != 0) {
          bl_timept64 allowed = pe->tprev + dest->cfg.log_rate_filter_time;
          if (bl_unlikely (bl_timept64_get_diff (now, allowed) < 0)) {
            continue;
          }
        }
        malc_log_strings s = get_entry_strings (dest, strs);
        (void) dest->dst.write (destination_get_instance (dest), now, sev, &s);
      }
    }
    if (!pe) {
      if (!past_entries_can_insert (&d->pe)) {
        past_entries_drop_head (&d->pe);
      }
      past_entries_expand_tail_n (&d->pe, 1);
      pe = past_entries_at_tail (&d->pe);
      pe->entry_id  = entry_id;
    }
    pe->tprev = now;
  }
}
/*----------------------------------------------------------------------------*/
bl_err destinations_get_instance(
  destinations const* d, void** instance, bl_u32 dest_id
  )
{
  destination* dest;
  bl_uword        id = 0;
  FOREACH_DESTINATION (d->mem, dest) {
    if (id == dest_id) {
      *instance = destination_get_instance (dest);
      return bl_mkok();
    }
    ++id;
  }
  return bl_mkerr (bl_invalid);
}
/*----------------------------------------------------------------------------*/
bl_err destinations_get_cfg(
  destinations const* d, malc_dst_cfg* cfg, bl_u32 dest_id
  )
{
  destination* dest;
  bl_uword        id = 0;
  FOREACH_DESTINATION (d->mem, dest) {
    if (id == dest_id) {
      *cfg = dest->cfg;
      return bl_mkok();
    }
    ++id;
  }
  return bl_mkerr (bl_invalid);
}
/*----------------------------------------------------------------------------*/
bl_err destinations_set_cfg (
  destinations* d, malc_dst_cfg const* cfg, bl_u32 dest_id
  )
{
  destination* dest;
  bl_uword        id = 0;
  FOREACH_DESTINATION (d->mem, dest) {
    if (id == dest_id) {
      dest->cfg = *cfg;
      break;
    }
    ++id;
  }
  if (bl_unlikely (!dest)) {
    return bl_mkerr (bl_invalid);
  }
  d->min_severity    = (bl_uword) -1;
  d->filter_max_time = 0;
  FOREACH_DESTINATION (d->mem, dest) {
    d->min_severity = bl_min (d->min_severity, dest->cfg.severity);
    d->filter_max_time =
      bl_max (d->filter_max_time, dest->cfg.log_rate_filter_time);
  }
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/

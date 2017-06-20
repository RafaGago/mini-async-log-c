#include <stdio.h>
#include <string.h>

#include <malc/destinations.h>

#include <bl/base/assert.h>
#include <bl/base/utility.h>
#include <bl/base/static_integer_math.h>

#define DEFAULT_SEVERITY malc_sev_warning
/*----------------------------------------------------------------------------*/
#define MAX_ALIGN    8 /* correct almost always TODO: add in base_library*/
/*----------------------------------------------------------------------------*/
#define SIZEOF_DEST_MAX_ALIGN\
  (round_to_next_multiple (sizeof (destination), MAX_ALIGN))
/*----------------------------------------------------------------------------*/
#define FOREACH_DESTINATION(mem, dest)\
  for ((dest) = (destination*) (mem);\
       (dest);\
       (dest) = (dest)->next_offset != 0 ?\
        (destination*) ((u8*) (dest) + (dest)->next_offset) :\
        nullptr\
       )
/*----------------------------------------------------------------------------*/
static inline void* destination_get_instance (destination* d)
{
  return (void*) ((u8*) d + SIZEOF_DEST_MAX_ALIGN);
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
  if (lit_strcmp (buff, "debug") == 0) {
    c->severity = malc_sev_debug;
  }
  else if (lit_strcmp (buff, "trace") == 0) {
    c->severity = malc_sev_trace;
  }
  else if (lit_strcmp (buff, "note") == 0) {
    c->severity = malc_sev_note;
  }
  else if (lit_strcmp (buff, "warning") == 0) {
    c->severity = malc_sev_warning;
  }
  else if (lit_strcmp (buff, "error") == 0) {
    c->severity = malc_sev_error;
  }
  else if (lit_strcmp (buff, "critical") == 0) {
    c->severity = malc_sev_critical;
  }
done:
  fclose (f);
}
/*----------------------------------------------------------------------------*/
void destinations_init (destinations* d, alloc_tbl const* alloc)
{
  memset (d, 0, sizeof *d);
  d->alloc        = alloc;
  d->min_severity = DEFAULT_SEVERITY;
  }
/*----------------------------------------------------------------------------*/
void destinations_destroy (destinations* d)
{
  (void) destinations_terminate (d);
  d->alloc = nullptr;
}
/*----------------------------------------------------------------------------*/
bl_err destinations_add (destinations* d, u32* dest_id, malc_dst const* dst)
{
  bl_assert (dest_id && dst);
  if (unlikely (!dst || !dst->write)) {
    return bl_invalid;
  }
  uword size = SIZEOF_DEST_MAX_ALIGN +
    round_to_next_multiple (dst->size_of, MAX_ALIGN);
  /* stored contiguosly because "write" is going to be called at data-rate */
  void* mem  = bl_realloc (d->alloc, d->mem, d->size + size);
  if (!mem) {
    return bl_alloc;
  }
  d->mem = mem;
  destination* dest = (destination*) (((u8*) d->mem) + d->size);
  dest->next_offset = 0;
  dest->dst         = *dst;

  dest->cfg.show_timestamp     = true;
  dest->cfg.show_severity      = true;
  dest->cfg.severity           = DEFAULT_SEVERITY;
  dest->cfg.severity_file_path = nullptr;

  if (dst->init) {
    bl_err err = dst->init (destination_get_instance (dest), d->alloc);
    if (err) {
      /* memory chunk is left as-is without shrinking it */
      return err;
    }
  }
  if (d->count) {
    destination* last;
    FOREACH_DESTINATION (d->mem, dest) {
      last = dest;
    }
    bl_assert (last);
    last->next_offset = size;
  }
  *dest_id = d->count;
  ++d->count;
  d->size += size;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
bl_err destinations_validate_rate_limit_settings(
  destinations* d, malc_security const* sec
  )
{
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
bl_err destinations_set_rate_limit_settings(
  destinations* d, malc_security const* sec
  )
{
  bl_err err = destinations_validate_rate_limit_settings (d, sec);
  if (err) {
    return err;
  }
  d->filter_time_us     = sec->log_rate_filter_time_us;
  d->filter_max         = sec->log_rate_filter_max;
  d->filter_watch_count = sec->log_rate_filter_watch_count;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
void destinations_get_rate_limit_settings (destinations* d, malc_security* sec)
{
  sec->log_rate_filter_time_us     = d->filter_time_us;
  sec->log_rate_filter_max         = d->filter_max;
  sec->log_rate_filter_watch_count = d->filter_watch_count;
}
/*----------------------------------------------------------------------------*/
void destinations_terminate (destinations* d)
{
  destination* dest;
  FOREACH_DESTINATION (d->mem, dest) {
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
void destinations_idle_task (destinations* d)
{
  /* TODO check fds */
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
void destinations_write(
  destinations* d, uword sev, tstamp now, log_strings strs
  )
{
  destination* dest;
  FOREACH_DESTINATION (d->mem, dest) {
    bl_assert (dest->dst.write);
    if (sev >= dest->cfg.severity) {
      (void) dest->dst.write(
        destination_get_instance (dest),
        now,
        strs.tstamp,
        dest->cfg.show_timestamp ? strs.tstamp_len : 0,
        strs.sev,
        dest->cfg.show_severity ? strs.sev_len : 0,
        strs.text,
        strs.text_len
        );
    }
  }
}
/*----------------------------------------------------------------------------*/
bl_err destinations_get_instance (destinations* d, void** instance, u32 dest_id)
{
  destination* dest;
  uword        id = 0;
  FOREACH_DESTINATION (d->mem, dest) {
    if (id == dest_id) {
      *instance = destination_get_instance (dest);
      return bl_ok;
    }
    ++id;
  }
  return bl_invalid;
}
/*----------------------------------------------------------------------------*/
bl_err destinations_get_cfg (destinations* d, malc_dst_cfg* cfg, u32 dest_id)
{
  destination* dest;
  uword        id = 0;
  FOREACH_DESTINATION (d->mem, dest) {
    if (id == dest_id) {
      *cfg = dest->cfg;
      return bl_ok;
    }
    ++id;
  }
  return bl_invalid;
}
/*----------------------------------------------------------------------------*/
bl_err destinations_set_cfg (
  destinations* d, malc_dst_cfg const* cfg, u32 dest_id
  )
{
  destination* dest;
  uword        id = 0;
  FOREACH_DESTINATION (d->mem, dest) {
    if (id == dest_id) {
      dest->cfg = *cfg;
      break;
    }
    ++id;
  }
  if (unlikely (!dest)) {
    return bl_invalid;
  }
  FOREACH_DESTINATION (d->mem, dest) {
    d->min_severity = bl_min (d->min_severity, dest->cfg.severity);
  }
  return bl_ok;
}
/*----------------------------------------------------------------------------*/

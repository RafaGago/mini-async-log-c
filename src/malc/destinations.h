#ifndef __MALC_DESTINATIONS_H__
#define __MALC_DESTINATIONS_H__

#include <bl/base/platform.h>
#include <bl/base/time.h>
#include <bl/base/error.h>
#include <bl/base/ringbuffer.h>

#include <malc/malc.h>

/*----------------------------------------------------------------------------*/
typedef struct past_entry {
  bl_uword  entry_id;
  bl_timept64 tprev;
}
past_entry;
/*----------------------------------------------------------------------------*/
typedef struct destination {
  bl_uword     next_offset;
  malc_dst     dst;
  malc_dst_cfg cfg;
}
destination;
/*----------------------------------------------------------------------------*/
typedef struct destinations {
  void*               mem;
  bl_alloc_tbl const* alloc;
  bl_uword            min_severity;
  bl_uword            count;
  bl_uword            size;
  bl_timept64         filter_max_time;
  bl_u32              filter_watch_count;
  bl_u32              filter_min_severity;
  bl_ringb            pe;
  past_entry          pe_buffer[64];
}
destinations;
/*----------------------------------------------------------------------------*/
static inline bl_uword destinations_min_severity (destinations const* d)
{
  return d->min_severity;
}
/*----------------------------------------------------------------------------*/
extern void destinations_init (destinations* d, bl_alloc_tbl const* alloc);
/*----------------------------------------------------------------------------*/
extern void destinations_destroy (destinations* d);
/*----------------------------------------------------------------------------*/
extern bl_err destinations_add(
  destinations* d, bl_u32* dest_id, malc_dst const* dst
  );
/*----------------------------------------------------------------------------*/
extern bl_err destinations_validate_rate_limit_settings(
  destinations* d, malc_security const* sec
  );
/*----------------------------------------------------------------------------*/
extern bl_err destinations_set_rate_limit_settings(
  destinations* d, malc_security const* sec
  );
/*----------------------------------------------------------------------------*/
extern void destinations_get_rate_limit_settings(
  destinations const* d, malc_security* sec
  );
/*----------------------------------------------------------------------------*/
extern void destinations_terminate (destinations* d);
/*----------------------------------------------------------------------------*/
extern void destinations_idle_task (destinations* , bl_timept64 now);
/*----------------------------------------------------------------------------*/
extern void destinations_flush (destinations* d);
/*----------------------------------------------------------------------------*/
extern void destinations_write(
  destinations*           d,
  bl_uword                entry_id,
  bl_timept64             now,
  bl_uword                sev,
  malc_log_strings const* strs
  );
/*----------------------------------------------------------------------------*/
extern bl_err destinations_get_instance(
  destinations const* d, void** instance, bl_u32 dest_id
  );
/*----------------------------------------------------------------------------*/
extern bl_err destinations_get_cfg(
  destinations const* d, malc_dst_cfg* cfg, bl_u32 dest_id
  );
/*----------------------------------------------------------------------------*/
extern bl_err destinations_set_cfg(
  destinations* d, malc_dst_cfg const* cfg, bl_u32 dest_id
  );
/*----------------------------------------------------------------------------*/

#endif
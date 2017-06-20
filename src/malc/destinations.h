#ifndef __MALC_DESTINATIONS_H__
#define __MALC_DESTINATIONS_H__

#include <bl/base/platform.h>
#include <bl/base/time.h>
#include <bl/base/error.h>

#include <malc/malc.h>
#include <malc/log_strings.h>

/*----------------------------------------------------------------------------*/
typedef struct destination {
  uword        next_offset;
  malc_dst     dst;
  malc_dst_cfg cfg;
}
destination;
/*----------------------------------------------------------------------------*/
typedef struct destinations {
  void*                mem;
  alloc_tbl const*     alloc;
  uword                min_severity;
  uword                count;
  uword                size;
  u32                  filter_time_us;
  u32                  filter_max;
  u32                  filter_watch_count;
}
destinations;
/*----------------------------------------------------------------------------*/
static inline uword destinations_min_severity (destinations const* d)
{
  return d->min_severity;
}
/*----------------------------------------------------------------------------*/
extern void destinations_init (destinations* d, alloc_tbl const* alloc);
/*----------------------------------------------------------------------------*/
extern void destinations_destroy (destinations* d);
/*----------------------------------------------------------------------------*/
extern bl_err destinations_add(
  destinations* d, u32* dest_id, malc_dst const* dst
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
  destinations* d, malc_security* sec
  );
/*----------------------------------------------------------------------------*/
extern void destinations_terminate (destinations* d);
/*----------------------------------------------------------------------------*/
extern void destinations_idle_task (destinations* d);
/*----------------------------------------------------------------------------*/
extern void destinations_flush (destinations* d);
/*----------------------------------------------------------------------------*/
extern void destinations_write(
  destinations* d, uword sev, tstamp now, log_strings strs
  );
/*----------------------------------------------------------------------------*/
extern bl_err destinations_get_instance(
  destinations* d, void** instance, u32 dest_id
  );
/*----------------------------------------------------------------------------*/
extern bl_err destinations_get_cfg(
  destinations* d, malc_dst_cfg* cfg, u32 dest_id
  );
/*----------------------------------------------------------------------------*/
extern bl_err destinations_set_cfg(
  destinations* d, malc_dst_cfg const* cfg, u32 dest_id
  );
/*----------------------------------------------------------------------------*/

#endif
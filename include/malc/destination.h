#ifndef __MALC_DESTINATION_H__
#define __MALC_DESTINATION_H__

#include <bl/base/platform.h>
#include <bl/base/error.h>
#include <bl/base/integer.h>
#include <bl/base/allocator.h>

/*----------------------------------------------------------------------------*/
typedef struct malc_dst_cfg {
  bool        show_timestamp;
  bool        show_severity;
  u8          severity;
  char const* severity_file_path;
}
malc_dst_cfg;
/*----------------------------------------------------------------------------*/
typedef struct malc_dst {
  bl_err (*create)       (void** logdst, alloc_tbl const* alloc);
  bl_err (*destroy)      (void* logdst, alloc_tbl const* alloc);
  bl_err (*init)         (void* logdst);
  bl_err (*terminate)    (void* logdst);
  bl_err (*get_cfg)      (void const* logdst, malc_dst_cfg* cfg);
  bl_err (*set_cfg)      (void* logdst, malc_dst_cfg const* cfg);
  bl_err (*get_specific) (void const* logdst, void* cfg);
  bl_err (*set_specific) (void* logdst, void const* cfg);
  bl_err (*write)        (
    void* logdst, char const* timestamp, char const* severity, char const* info
    );
}
malc_dst;
/*----------------------------------------------------------------------------*/

#endif /* __MALC_DESTINATION_H__ */

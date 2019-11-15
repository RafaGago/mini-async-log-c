#ifndef __MALC_LOGGING_INTERNAL__
#define __MALC_LOGGING_INTERNAL__

#include <bl/base/error.h>
#include <bl/base/integer.h>

#ifdef __cplusplus
  extern "C" {
#endif
/*----------------------------------------------------------------------------*/
struct malc;
struct malc_serializer;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_log_entry_prepare(
  struct malc*            l,
  struct malc_serializer* ext_ser,
  malc_const_entry const* entry,
  bl_uword                payload_size
  );
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT void malc_log_entry_commit(
  struct malc* l, struct malc_serializer const* ext_ser
  );
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_uword malc_get_min_severity (struct malc const* l);
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif

#endif /* #define __MALC_LOGGING_INTERNAL__ */
#ifndef __MALC_SERIALIZATION_H__
#define __MALC_SERIALIZATION_H__

#include <stdarg.h>

#include <bl/base/platform.h>
#include <bl/base/integer.h>
#include <bl/base/autoarray.h>

#include <malc/malc.h>
#include <malc/log_entry.h>

/*----------------------------------------------------------------------------*/
define_autoarray_types (log_args, log_argument);
define_autoarray_types (log_refs, malc_ref);
/*----------------------------------------------------------------------------*/
typedef struct compressed_header {
  u8*   hdr;
  uword idx;
}
compressed_header;
/*----------------------------------------------------------------------------*/
#ifdef MALC_NO_COMPRESSION
/*----------------------------------------------------------------------------*/
typedef struct serializer {
  malc_const_entry const* entry;
  bool                    has_tstamp;
  tstamp                  t;
  uword                   extra_size;
  compressed_header*      ch;
}
serializer;
/*----------------------------------------------------------------------------*/
static inline uword serializer_hdr_size (serializer const* se)
{
  return 0;
}
/*----------------------------------------------------------------------------*/
#else /* MALC_NO_COMPRESSION */
/*----------------------------------------------------------------------------*/
typedef struct serializer {
  malc_const_entry const* entry;
  bool                    has_tstamp;
  malc_compressed_64      t;
  uword                   extra_size;
  compressed_header*      ch;
  malc_compressed_ptr     comp_entry;
  uword                   hdr_size;
  compressed_header       chval;
}
serializer;
/*----------------------------------------------------------------------------*/
static inline uword serializer_hdr_size (serializer const* se)
{
  return se->hdr_size;
}
/*----------------------------------------------------------------------------*/
#endif /* MALC_NO_COMPRESSION */
/*----------------------------------------------------------------------------*/
extern void serializer_init(
  serializer* se, malc_const_entry const* entry, bool has_tstamp
  );
/*----------------------------------------------------------------------------*/
static inline uword serializer_log_entry_size(
  serializer const* se, uword payload
  )
{
  return payload + serializer_hdr_size (se) + se->extra_size;
}
/*----------------------------------------------------------------------------*/
extern uword serializer_execute (serializer* se, u8* mem, va_list vargs);
/*----------------------------------------------------------------------------*/
typedef struct deserializer {
  log_args                args;
  log_refs                refs;
  malc_refdtor            refdtor;
  malc_const_entry const* entry;
  tstamp                  t;
  compressed_header*      ch;
#ifndef MALC_NO_COMPRESSION
  compressed_header       chval;
#endif
}
deserializer;
/*----------------------------------------------------------------------------*/
extern bl_err deserializer_init (deserializer* ds, alloc_tbl const* alloc);
/*----------------------------------------------------------------------------*/
extern void deserializer_destroy (deserializer* ds, alloc_tbl const* alloc);
/*----------------------------------------------------------------------------*/
extern void deserializer_reset (deserializer* ds);
/*----------------------------------------------------------------------------*/
extern bl_err deserializer_execute(
  deserializer*    ds,
  u8*              mem,
  u8*              mem_end,
  bool             has_timestamp,
  alloc_tbl const* alloc
  );
/*----------------------------------------------------------------------------*/
extern log_entry deserializer_get_log_entry (deserializer const* ds);
/*----------------------------------------------------------------------------*/
#endif

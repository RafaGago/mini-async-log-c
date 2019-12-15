#ifndef __MALC_SERIALIZATION_H__
#define __MALC_SERIALIZATION_H__

#include <stdarg.h>

#include <bl/base/platform.h>
#include <bl/base/integer_short.h>
#include <bl/base/autoarray.h>

#include <malc/malc.h>
#include <malc/log_entry.h>
#include <malc/impl/serialization.h>

/*----------------------------------------------------------------------------*/
bl_define_autoarray_types (log_args, log_argument);
bl_define_autoarray_types (log_refs, malc_ref);
/*----------------------------------------------------------------------------*/
typedef struct compressed_header {
  bl_u8*   hdr;
  bl_uword idx;
}
compressed_header;
/*----------------------------------------------------------------------------*/
#if MALC_BUILTIN_COMPRESSION == 0
/*----------------------------------------------------------------------------*/
typedef struct serializer {
  malc_const_entry const* entry;
  bool                    has_tstamp;
  bl_timept64             t;
  bl_uword                internal_fields_size;
  compressed_header*      ch;
}
serializer;
/*----------------------------------------------------------------------------*/
#else /* MALC_BUILTIN_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
typedef struct serializer {
  malc_const_entry const* entry;
  bool                    has_tstamp;
  malc_compressed_64      t;
  bl_uword                internal_fields_size;
  compressed_header*      ch;
  bl_uword                comp_hdr_size;
  compressed_header       chval;
}
serializer;
/*----------------------------------------------------------------------------*/
#endif /* MALC_BUILTIN_COMPRESSION == 0 */
/*----------------------------------------------------------------------------*/
extern void serializer_init(
  serializer* se, malc_const_entry const* entry, bool has_tstamp
  );
/*----------------------------------------------------------------------------*/
extern malc_serializer serializer_prepare_external_serializer(
  serializer* ser, bl_u8* node_mem, bl_u8* mem
  );
/*----------------------------------------------------------------------------*/
static inline bl_uword serializer_log_entry_size(
  serializer const* se, bl_uword payload
  )
{
  return payload +
#if MALC_BUILTIN_COMPRESSION != 0
    se->comp_hdr_size +
#endif
    se->internal_fields_size;
}
/*----------------------------------------------------------------------------*/
typedef struct deserializer {
  log_args                args;
  log_refs                refs;
  malc_refdtor            refdtor;
  malc_const_entry const* entry;
  bl_timept64             t;
  compressed_header*      ch;
#if MALC_BUILTIN_COMPRESSION
  compressed_header       chval;
#endif
}
deserializer;
/*----------------------------------------------------------------------------*/
extern bl_err deserializer_init (deserializer* ds, bl_alloc_tbl const* alloc);
/*----------------------------------------------------------------------------*/
extern void deserializer_destroy (deserializer* ds, bl_alloc_tbl const* alloc);
/*----------------------------------------------------------------------------*/
extern void deserializer_reset (deserializer* ds);
/*----------------------------------------------------------------------------*/
extern bl_err deserializer_execute(
  deserializer*       ds,
  bl_u8*              mem,
  bl_u8*              mem_end,
  bool                has_timestamp,
  bl_alloc_tbl const* alloc
  );
/*----------------------------------------------------------------------------*/
extern log_entry deserializer_get_log_entry (deserializer const* ds);
/*----------------------------------------------------------------------------*/
#endif

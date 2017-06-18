#ifndef __MALC_DESTINATION_H__
#define __MALC_DESTINATION_H__

#include <bl/base/platform.h>
#include <bl/base/error.h>
#include <bl/base/integer.h>
#include <bl/base/allocator.h>
#include <bl/base/time.h>

/*----------------------------------------------------------------------------*/
typedef struct malc_dst_cfg {
  bool        show_timestamp;
  bool        show_severity;
  u8          severity;
  char const* severity_file_path; /* null or a file to read the severity from */
}
malc_dst_cfg;
/*----------------------------------------------------------------------------*/
/* malc_dst: Data table of a malc destination. */
/*----------------------------------------------------------------------------*/
typedef struct malc_dst {
  /* sizeof the struct, will be used by malc to allocate */
  uword size_of;
  /* initialization: instance will contain a raw memory chunk of "size_of"
     bytes. "alloc" is a provided memory allocator in case it's necessary (it
     will be the same one passed to malc on "malc_create"). The allocator can
     be stored for later usage. It can be set to null if nothing relevant
     happens on initialization */
  bl_err (*init)      (void* instance, alloc_tbl const* alloc);
  /* termination before memory deallocation. can be set to null if there is no
     termination required */
  void   (*terminate) (void* instance);
  /* flush all data in case of the "write" function being buffered. It can be
     set to null if there is no flush. */
  bl_err (*flush)     (void* instance);
  /* will be invoked when the logger is idle, e.g. to do low priority tasks. It
     can be set to null if there is no idle task. */
  bl_err (*idle_task) (void* instance);
  bl_err (*write)(
    void*       instance,
    tstamp      now,           /* current timestamp, can be used for timeouts */
    char const* timestamp,     /* timestamp string */
    uword       timestamp_len, /* timestamp string length. can be zero */
    char const* severity,      /* severity string*/
    uword       severity_len,  /* severity string length. can be zero */
    char const* text,          /* log entry text */
    uword       text_len       /* log entry text length. can't be zero */
    );
}
malc_dst;
/*----------------------------------------------------------------------------*/

#endif /* __MALC_DESTINATION_H__ */

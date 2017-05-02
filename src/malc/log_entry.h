#ifndef __MALC_LOG_ENTRY_H__
#define __MALC_LOG_ENTRY_H__

#include <bl/base/integer.h>
#include <bl/base/autoarray.h>
#include <bl/base/time.h>

#include <malc/malc.h>

/*----------------------------------------------------------------------------*/
typedef union log_argument {
  u8       vu8;
  u32      vu32;
  u16      vu16;
  u64      vu64;
  i8       vi8;
  float    vfloat;
  i32      vi32;
  i16      vi16;
  i64      vi64;
  double   vdouble;
  void*    vptr;
  malc_str vstr;
  malc_lit vlit;
  malc_mem vmem;
}
log_argument;
/*----------------------------------------------------------------------------*/
define_autoarray_types (log_args, log_argument);
declare_autoarray_funcs (log_args, log_argument);
/*----------------------------------------------------------------------------*/
typedef struct log_entry {
  malc_const_entry const* entry;
  tstamp                  timestamp;
  log_args const*         args;
}
log_entry;
/*----------------------------------------------------------------------------*/

#endif
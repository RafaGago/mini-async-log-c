#ifndef __MALC_LOG_ENTRY_H__
#define __MALC_LOG_ENTRY_H__

#include <bl/base/integer.h>
#include <bl/base/time.h>

#include <malc/malc.h>

/*----------------------------------------------------------------------------*/
typedef union log_argument {
  u8           vu8;
  u32          vu32;
  u16          vu16;
  u64          vu64;
  i8           vi8;
  float        vfloat;
  i32          vi32;
  i16          vi16;
  i64          vi64;
  double       vdouble;
  void*        vptr;
  malc_lit     vlit;
  malc_strcp   vstrcp;
  malc_strref  vstrref;
  malc_memcp   vmemcp;
  malc_memref  vmemref;
}
log_argument;
/*----------------------------------------------------------------------------*/
typedef struct log_entry {
  malc_const_entry const* entry;
  tstamp                  timestamp;
  log_argument const*     args;
  uword                   args_count;
  malc_ref const*         refs;
  uword                   refs_count;
  malc_refdtor            refdtor;
}
log_entry;
/*----------------------------------------------------------------------------*/

#endif
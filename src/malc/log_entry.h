#ifndef __MALC_LOG_ENTRY_H__
#define __MALC_LOG_ENTRY_H__

#include <bl/base/integer.h>
#include <bl/base/time.h>

#include <malc/malc.h>

/*----------------------------------------------------------------------------*/
typedef union log_argument {
  bl_u8         vu8;
  bl_u32        vu32;
  bl_u16        vu16;
  bl_u64        vu64;
  bl_i8         vi8;
  float         vfloat;
  bl_i32        vi32;
  bl_i16        vi16;
  bl_i64        vi64;
  double        vdouble;
  void*         vptr;
  malc_lit      vlit;
  malc_strcp    vstrcp;
  malc_strref   vstrref;
  malc_memcp    vmemcp;
  malc_memref   vmemref;
  malc_obj      vobj;
  malc_obj_flag vobjflag;
  malc_obj_ctx  vobjctx;
}
log_argument;
/*----------------------------------------------------------------------------*/
typedef struct log_entry {
  malc_const_entry const* entry;
  bl_timept64             timestamp;
  log_argument const*     args;
  bl_uword                args_count;
  malc_ref const*         refs;
  bl_uword                refs_count;
  malc_refdtor            refdtor;
}
log_entry;
/*----------------------------------------------------------------------------*/

#endif
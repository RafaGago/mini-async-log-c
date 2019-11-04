#ifndef __MALC_ALLTYPES_H__
#define __MALC_ALLTYPES_H__

#include <malc/malc.h>

typedef struct alltypes {
  double       vdouble;
  bl_u8        vu8;
  bl_u32       vu32;
  bl_u16       vu16;
  bl_u64       vu64;
  bl_i8        vi8;
  bl_i32       vi32;
  bl_i16       vi16;
  bl_i64       vi64;
  float        vfloat;
  void*        vptr;
  malc_lit     vlit;
  malc_strcp   vstrcp;
  malc_memcp   vmemcp;
  malc_strref  vstrref;
  malc_memref  vmemref;
  malc_refdtor vrefdtor;
}
alltypes;

#endif
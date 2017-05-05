#ifndef __MALC_ALLTYPES_H__
#define __MALC_ALLTYPES_H__

#include <malc/malc.h>

typedef struct alltypes {
  double       vdouble;
  u8           vu8;
  u32          vu32;
  u16          vu16;
  u64          vu64;
  i8           vi8;
  i32          vi32;
  i16          vi16;
  i64          vi64;
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
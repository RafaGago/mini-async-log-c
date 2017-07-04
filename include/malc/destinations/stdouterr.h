#ifndef __MALC_STDOUTERR_DESTINATION_H__
#define __MALC_STDOUTERR_DESTINATION_H__

#include <malc/libexport.h>
#include <malc/destination.h>

/*----------------------------------------------------------------------------*/
typedef struct malc_stdouterr_dst malc_stdouterr_dst;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT const struct malc_dst malc_stdouterr_dst_tbl;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_stdouterr_set_stderr_severity(
  malc_stdouterr_dst* d, uword sev
  );
/*----------------------------------------------------------------------------*/

#endif /* __MALC_STDOUTERR_DESTINATION_H__ */

#ifndef __MALC_FILE_DESTINATION_H__
#define __MALC_FILE_DESTINATION_H__

#include <malc/libexport.h>
#include <malc/common.h>

#ifdef __cplusplus
  extern "C" {
#endif

/*----------------------------------------------------------------------------*/
typedef struct malc_file_dst malc_file_dst;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT const struct malc_dst malc_file_dst_tbl;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_file_set_cfg(
  malc_file_dst* d, malc_file_cfg const* cfg
  );
/*----------------------------------------------------------------------------*/
/* TODO const* d */
extern MALC_EXPORT bl_err malc_file_get_cfg(
  malc_file_dst* d, malc_file_cfg* cfg
  );
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
 } // extern "C"
#endif

#endif /* __MALC_FILE_DESTINATION_H__ */

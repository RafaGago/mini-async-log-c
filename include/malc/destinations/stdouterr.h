#ifndef __MALC_STDOUTERR_DESTINATION_H__
#define __MALC_STDOUTERR_DESTINATION_H__

#include <malc/libexport.h>
#include <malc/destination.h>

#ifdef __cplusplus
  extern "C" {
#endif

/*----------------------------------------------------------------------------*/
typedef struct malc_stdouterr_dst malc_stdouterr_dst;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT const struct malc_dst malc_stdouterr_dst_tbl;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_stdouterr_set_stderr_severity(
  malc_stdouterr_dst* d, bl_uword sev
  );
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
 } // extern "C"

class malc_wrapper;
/*----------------------------------------------------------------------------*/
class malc_stdouterr_dst_adapter {
public:
  /*--------------------------------------------------------------------------*/
  bl_err set_stderr_severity (bl_uword sev) noexcept
  {
    return malc_stdouterr_set_stderr_severity ((malc_stdouterr_dst*) this, sev);
  }
  /*--------------------------------------------------------------------------*/
private:
  /*--------------------------------------------------------------------------*/
  friend class malc_wrapper;
  static malc_dst get_dst_tbl()
  {
    return malc_stdouterr_dst_tbl;
  }
  /*--------------------------------------------------------------------------*/
};
/*----------------------------------------------------------------------------*/
#endif /* __cplusplus */
#endif /* __MALC_STDOUTERR_DESTINATION_H__ */

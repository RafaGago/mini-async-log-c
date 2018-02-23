#include <stdio.h>

#include <bl/base/utility.h>

#include <malc/malc.h>
#include <malc/destinations/stdouterr.h>

/*----------------------------------------------------------------------------*/
struct malc_stdouterr_dst {
  uword stderr_sev;
};
/*----------------------------------------------------------------------------*/
static bl_err malc_stdouterr_dst_init (void* instance, alloc_tbl const* alloc)
{
  malc_stdouterr_dst* d = (malc_stdouterr_dst*) instance;
  d->stderr_sev = malc_sev_error;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static bl_err malc_stdouterr_dst_write(
    void* instance, tstamp now, uword sev_val, malc_log_strings const* strs
    )
{
  malc_stdouterr_dst* d = (malc_stdouterr_dst*) instance;
  FILE* dst = sev_val < d->stderr_sev ? stdout : stderr;
  fwrite (strs->tstamp, 1, strs->tstamp_len, dst);
  fwrite (strs->sev, 1, strs->sev_len, dst);
  fwrite (strs->text, 1, strs->text_len, dst);
  fwrite ("\n", 1, lit_len ("\n"), dst);
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
static bl_err malc_stdouterr_dst_flush (void* instance)
{
  malc_stdouterr_dst* d = (malc_stdouterr_dst*) instance;
  if (d->stderr_sev != malc_sev_debug) {
    fflush (stdout);
  }
  if (d->stderr_sev != malc_sev_off) {
    fflush (stderr);
  }
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/
MALC_EXPORT const struct malc_dst malc_stdouterr_dst_tbl = {
  sizeof (malc_stdouterr_dst), /*size_of*/
  &malc_stdouterr_dst_init,
  nullptr,                   /* terminate */
  &malc_stdouterr_dst_flush,
  nullptr,                   /* idle task */
  &malc_stdouterr_dst_write
};
/*----------------------------------------------------------------------------*/
MALC_EXPORT bl_err malc_stdouterr_set_stderr_severity(
  malc_stdouterr_dst* d, uword sev
  )
{
  if (!malc_is_valid_severity (sev) && sev != malc_sev_off) {
    return bl_mkerr (bl_invalid);
  }
  d->stderr_sev = sev;
  return bl_mkok();
}
/*----------------------------------------------------------------------------*/

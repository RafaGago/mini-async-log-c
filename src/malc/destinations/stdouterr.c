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
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
static bl_err malc_stdouterr_dst_write(
    void*       instance,
    tstamp      now,
    uword       severity_val,
    char const* timestamp,
    uword       timestamp_len,
    char const* severity,
    uword       severity_len,
    char const* text,
    uword       text_len
    )
{
  malc_stdouterr_dst* d = (malc_stdouterr_dst*) instance;
  FILE* dst = severity_val < d->stderr_sev ? stdout : stderr;
  fwrite (timestamp, 1, timestamp_len, dst);
  fwrite (severity, 1, severity_len, dst);
  fwrite (text, 1, text_len, dst);
  fwrite ("\n", 1, lit_len ("\n"), dst);
  return bl_ok;
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
  return bl_ok;
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
    return bl_invalid;
  }
  d->stderr_sev = sev;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/

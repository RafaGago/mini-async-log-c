#ifndef __MALC_LOG_STRINGS_H__
#define __MALC_LOG_STRINGS_H__

#include <bl/base/integer.h>

/*----------------------------------------------------------------------------*/
typedef struct log_strings {
  char const* tstamp;
  uword       tstamp_len;
  char const* sev;
  uword       sev_len;
  char const* text;
  uword       text_len;
}
log_strings;
/*----------------------------------------------------------------------------*/

#endif /* __MALC_LOG_STRINGS_H__ */

#ifndef __MALC_FILE_DESTINATION_H__
#define __MALC_FILE_DESTINATION_H__

#include <bl/base/integer.h>

#include <malc/libexport.h>
#include <malc/destination.h>

/*------------------------------------------------------------------------------
name_prefix:
extension:
  The file names have this form:
    "name_prefix"_"wall clock"_"monotonic clock"."extension"

  Where wall clock and monotonic clock are the values of the clocks at file
  creation time written as an unsigned 64 bit integer coded as hexadecimal (16
  chars).

  All the log entries inside a file are relative to the monotonic clock. A
  simple script or program can convert them at log archivation/visualization
  time by using OS calendar primitives.

file_size:
  The approximate maximum log file size in KB. It will be rounded to contain
  full log entries so if the maximum file size for an empty log file is 1KB and
  it comes a log entry of 6KB that file size will be 6KB. All the log files will
  be this size at minimum. 0 disables breaking log files.

max_log_files:
  The maximum number of log files to keep. Once this number is reached the log
  files will start rotating. Requires a non-zero "file_size". 0 disables log
  rotation.
------------------------------------------------------------------------------*/
typedef struct malc_file_cfg {
  char const* name_prefix;
  char const* extension;
  uword       file_size;
  uword       max_log_files;
}
malc_file_cfg;
/*----------------------------------------------------------------------------*/
typedef struct malc_file_dst malc_file_dst;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT const struct malc_dst malc_file_dst_tbl;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_file_set_cfg(
  malc_file_dst* d, malc_file_cfg const* cfg
  );
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_err malc_file_get_cfg(
  malc_file_dst* d, malc_file_cfg* cfg
  );
/*----------------------------------------------------------------------------*/

#endif /* __MALC_FILE_DESTINATION_H__ */

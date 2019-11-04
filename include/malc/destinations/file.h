#ifndef __MALC_FILE_DESTINATION_H__
#define __MALC_FILE_DESTINATION_H__

#include <bl/base/integer.h>

#include <malc/libexport.h>
#include <malc/destination.h>

#ifdef __cplusplus
  extern "C" {
#endif

/*------------------------------------------------------------------------------
prefix:
suffix:
time_based_name:

  The file names have this form when "time_based_name" is "true":
    "prefix"_"(system clock)"_"(monotonic clock)""suffix"

  Or this one when "time_based_name" is "false":
    "prefix"_"(number)""suffix"

  "prefix" can include a folder, but the folder won't be created, it has to
  exist before.

  "system clock" is the value of the system clock in nanoseconds coded as a
  64bit zero-filled hexadecimal number.

  "monotonic clock" is the value of a clock that always goes forward, is not
  affected by DST or wall clock time changes and starts at some arbitrary point
  of time. It is in nanoseconds units too, coded as a 64bit zero-filled
  hexadecimal number.

  All the log entries inside a file are relative to the monotonic clock.
  Expressed in seconds as floating point numbers.

  For a demo on how to convert the dates to calendar, see
  "scripts/malc-overview-date-converter.sh"

max_file_size:

  The approximate maximum log file size in KB. It will be rounded to contain
  full log entries. This size will never be surpassed except for files with a
  single log entry bigger than this size. 0 disables splitting log files in
  chunks.

max_log_files:

  The maximum number of log files to keep. Once this number is reached the log
  files will start rotating. Requires a non-zero "file_size". 0 disables log
  rotation.

can_remove_old_data_on_full_disk:

  If the disk is full the logger starts erasing either the oldest log files
  (when "max_log_files" is non zero) or it deletes the current log file when
  "max_log_files" is zero.
------------------------------------------------------------------------------*/
typedef struct malc_file_cfg {
  char const* prefix;
  char const* suffix;
  bool        time_based_name;
  bool        can_remove_old_data_on_full_disk;
  bl_uword    max_file_size;
  bl_uword    max_log_files;
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

#ifdef __cplusplus
  }
#endif

#endif /* __MALC_FILE_DESTINATION_H__ */

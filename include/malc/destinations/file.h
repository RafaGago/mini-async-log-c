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

  "wall clock" and "monotonic clock" are the values of the clocks at file
  creation time written as an unsigned 64 bit integer coded as hexadecimal (16
  chars). They are get through "bl_get_timestamp" and
  "bl_get_sysclock_timestamp". You can check <bl/base/time.h> for more details
  on how they map together depending on the platform.

  All the log entries inside a file are relative to the monotonic clock. A
  simple script or program can convert them at log archivation/visualization
  time by using OS calendar primitives.

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
  uword       max_file_size;
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

#ifdef __cplusplus
  }
#endif

#endif /* __MALC_FILE_DESTINATION_H__ */

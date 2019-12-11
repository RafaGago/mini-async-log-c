#ifndef __MALC_ARRAY_DESTINATION_H__
#define __MALC_ARRAY_DESTINATION_H__

#include <stddef.h>
#include <malc/libexport.h>
#include <malc/common.h>

#ifdef __cplusplus
  extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/* A memory destination designed for malc smoke testing. It might be used for
multithreaded application debugging when printf timings affect the behavior, but
be aware of the asynchronous nature of malc: the last entries aren't guaranteed
to be logged yet when you hit e.g. a breakpoint.

This destination stores log entries in an external array of char arrays, so it
wastes space on the tail of each fixed-size entry. The array is circular and
loses the oldest entry when full. The entries can be easily indexed and seen
with a debugger. It always keeps an empty entry at the tail to be able to see
where the tail is located by just looking at the memory array itself.
*/
/*----------------------------------------------------------------------------*/
typedef struct malc_array_dst malc_array_dst;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT const struct malc_dst malc_array_dst_tbl;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT void malc_array_dst_set_array(
  malc_array_dst* d, char* mem, size_t mem_entries, size_t entry_chars
  );
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT size_t malc_array_dst_size (malc_array_dst const* d);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT size_t malc_array_dst_capacity (malc_array_dst const* d);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT char const* malc_array_dst_get_entry(
  malc_array_dst const* d, size_t idx
  );
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
 } // extern "C"
#endif

#endif

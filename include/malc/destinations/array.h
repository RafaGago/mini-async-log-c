#ifndef __MALC_ARRAY_DESTINATION_H__
#define __MALC_ARRAY_DESTINATION_H__

#include <malc/libexport.h>
#include <malc/destination.h>

/*----------------------------------------------------------------------------*/
/* A memory logger designed for malc smoke testing. It might be used for
multithreaded application debugging when printf affects behavior, but be aware
of the asynchronous nature of malc: the last entries aren't guaranteed to be
logged yet when you hit e.g. a breakpoint.

It stores log entries in an external array of char arrays, so it wastes space on
the tail of each entry but the entries can be easily indexed and seen with a
debugger. It always keeps an empty entry at the tail to be able to see where the
tail is located without (array rotates).
*/
/*----------------------------------------------------------------------------*/
typedef struct malc_array_dst malc_array_dst;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT const struct malc_dst malc_array_dst_tbl;
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT void malc_array_dst_set_array(
  malc_array_dst* d, char* mem, uword mem_entries, uword entry_chars
  );
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT uword malc_array_dst_size (malc_array_dst* d);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT uword malc_array_dst_capacity (malc_array_dst* d);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT char const* malc_array_dst_get_entry(
  malc_array_dst* d, uword idx
  );
/*----------------------------------------------------------------------------*/

#endif
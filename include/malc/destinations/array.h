#ifndef __MALC_ARRAY_DESTINATION_H__
#define __MALC_ARRAY_DESTINATION_H__

#include <malc/libexport.h>
#include <malc/destination.h>

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
  malc_array_dst* d, char* mem, bl_uword mem_entries, bl_uword entry_chars
  );
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_uword malc_array_dst_size (malc_array_dst const* d);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT bl_uword malc_array_dst_capacity (malc_array_dst const* d);
/*----------------------------------------------------------------------------*/
extern MALC_EXPORT char const* malc_array_dst_get_entry(
  malc_array_dst const* d, bl_uword idx
  );
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
 } // extern "C"

class malc_wrapper;
/*----------------------------------------------------------------------------*/
class malc_array_dst_adapter {
public:
  /*--------------------------------------------------------------------------*/
  void set_array (char* mem, bl_uword mem_entries, bl_uword entry_chars) noexcept
  {
    malc_array_dst_set_array(
      (malc_array_dst*) this, mem, mem_entries, entry_chars
      );
  }
  /*--------------------------------------------------------------------------*/
  bl_uword size() const noexcept
  {
    return malc_array_dst_size ((malc_array_dst const*) this);
  }
  /*--------------------------------------------------------------------------*/
  bl_uword capacity() const noexcept
  {
    return malc_array_dst_capacity ((malc_array_dst const*) this);
  }
  /*--------------------------------------------------------------------------*/
  char const* get_entry (bl_uword idx) const noexcept
  {
    return malc_array_dst_get_entry ((malc_array_dst const*) this, idx);
  }
  /*--------------------------------------------------------------------------*/
private:
  /*--------------------------------------------------------------------------*/
  friend class malc_wrapper;
  static malc_dst get_dst_tbl()
  {
    return malc_array_dst_tbl;
  }
  /*--------------------------------------------------------------------------*/
};
/*----------------------------------------------------------------------------*/
#endif /* __cplusplus */
#endif

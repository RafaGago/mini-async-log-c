#ifndef __MALC_CPP_DESTINATION_ADAPTER_HPP__
#define __MALC_CPP_DESTINATION_ADAPTER_HPP__

#include <malc/destination.h>
#include <malc/destinations/array.h>
#include <malc/destinations/file.h>
#include <malc/destinations/stdouterr.h>

/* machinery to avoid all the destination boilerplate on C++ and be able to
make an straight-to-the-point destination implementation.

class required_destination_methods {
  required_destination_methods (const bl_alloc_tbl& alloc);
  bool flush();
  bool idle_task();
  bool write (bl_u64 nsec, bl_uword severity, malc_log_strings const& strs);
}
*/
/*----------------------------------------------------------------------------*/
class malc_wrapper;
/*----------------------------------------------------------------------------*/
template <class uppermost_derivation>
class malc_destination_adapter : public uppermost_derivation {
private:
  friend class malc_wrapper;
  /*--------------------------------------------------------------------------*/
  static malc_dst get_dst_tbl()
  {
    malc_dst d;
    d.size_of   = sizeof (uppermost_derivation);
    d.init      = fwd_init;
    d.terminate = fwd_terminate;
    d.flush     = fwd_flush;
    d.idle_task = fwd_idle_task;
    d.write     = fwd_write;
    return d;
  }
  /*--------------------------------------------------------------------------*/
  static bl_err fwd_init (void* instance, bl_alloc_tbl const* alloc)
  {
    return try_catch_wrap ([=](){
      new (instance) uppermost_derivation (*alloc);
      return bl_mkok();
    });
  }
  /*--------------------------------------------------------------------------*/
  static void fwd_terminate (void* instance)
  {
    try_catch_wrap ([=](){
      derived (instance).~uppermost_derivation();
      return bl_mkok();
    });
  }
  /*--------------------------------------------------------------------------*/
  static bl_err fwd_flush (void* instance)
  {
    return try_catch_wrap ([=](){
      return derived (instance).flush() ? bl_mkok() : bl_mkerr (bl_error);
    });
  }
  /*--------------------------------------------------------------------------*/
  static bl_err fwd_idle_task (void* instance)
  {
    return try_catch_wrap ([=](){
      return derived (instance).idle_task() ? bl_mkok() : bl_mkerr (bl_error);
    });
  }
  /*--------------------------------------------------------------------------*/
  static bl_err fwd_write(
    void*                   instance,
    bl_u64                  nsec,
    bl_uword                severity,
    malc_log_strings const* strs
    )
  {
    return try_catch_wrap ([=](){
      return derived (instance).write (nsec, severity, *strs) ?
        bl_mkok() : bl_mkerr (bl_error);
    });
  }
  /*--------------------------------------------------------------------------*/
  static inline uppermost_derivation& derived(void* instance)
  {
    return *static_cast<uppermost_derivation*> (instance);
  }
  /*--------------------------------------------------------------------------*/
  template <class runnable>
  static inline bl_err try_catch_wrap (const runnable& r)
  {
    try {
      return r();
    }
    catch (...) {
      return bl_mkerr (bl_error);
    }
  }
};
/*----------------------------------------------------------------------------*/
template <class malc_cpp_destination>
class malc_cpp_destination_adapt :
  public malc_destination_adapter<malc_cpp_destination> {};
/*----------------------------------------------------------------------------*/
template<>
struct malc_cpp_destination_adapt<malc_array_dst_adapter> :
  public malc_array_dst_adapter {};
/*----------------------------------------------------------------------------*/
template<>
struct malc_cpp_destination_adapt<malc_file_dst_adapter> :
  public malc_file_dst_adapter {};
/*----------------------------------------------------------------------------*/
template<>
struct malc_cpp_destination_adapt<malc_stdouterr_dst_adapter> :
  public malc_stdouterr_dst_adapter {};
/*----------------------------------------------------------------------------*/
#endif

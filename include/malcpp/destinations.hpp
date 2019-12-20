#ifndef __MALC_DESTINATION_HPP__
#define __MALC_DESTINATION_HPP__

#include <cstddef>
#include <cstdint>

#include <bl/base/platform.h>
#include <bl/base/error.h>

#include <malc/libexport.h>
#include <malc/common.h>

namespace malcpp {

struct malc_file_cfg;
typedef malc_file_cfg file_dst_cfg;
/*----------------------------------------------------------------------------*/
class wrapper;
/*----------------------------------------------------------------------------*/
class MALC_EXPORT file_dst {
public:
  /*--------------------------------------------------------------------------*/
  bl_err set_cfg (file_dst_cfg const& cfg) noexcept;
  /*--------------------------------------------------------------------------*/
  bl_err get_cfg (file_dst_cfg& cfg) const noexcept;
  /*--------------------------------------------------------------------------*/
private:
  /*--------------------------------------------------------------------------*/
  friend class wrapper;
  static malc_dst get_dst_tbl();
  /*--------------------------------------------------------------------------*/
};
/*----------------------------------------------------------------------------*/
class MALC_EXPORT stdouterr_dst {
public:
  /*--------------------------------------------------------------------------*/
  bl_err set_stderr_severity (unsigned sev) noexcept;
  /*--------------------------------------------------------------------------*/
private:
  /*--------------------------------------------------------------------------*/
  friend class wrapper;
  static malc_dst get_dst_tbl();
  /*--------------------------------------------------------------------------*/
};
/*----------------------------------------------------------------------------*/
class MALC_EXPORT array_dst {
public:
  /*--------------------------------------------------------------------------*/
  void
    set_array(char* mem, size_t mem_entries, size_t entry_chars) noexcept;
  /*--------------------------------------------------------------------------*/
  size_t size() const noexcept;
  /*--------------------------------------------------------------------------*/
  size_t capacity() const noexcept;
  /*--------------------------------------------------------------------------*/
  char const* get_entry (size_t idx) const noexcept;
  /*--------------------------------------------------------------------------*/
  char const* operator[] (size_t idx) const noexcept
  {
    return get_entry (idx);
  }
  /*--------------------------------------------------------------------------*/
private:
  /*--------------------------------------------------------------------------*/
  friend class wrapper;
  static malc_dst get_dst_tbl();
  /*--------------------------------------------------------------------------*/
};
/*----------------------------------------------------------------------------*/
/* machinery to avoid all the destination boilerplate on C++ and be able to
make an straight-to-the-point destination implementation.

class required_destination_methods {
  required_destination_methods (const bl_alloc_tbl& alloc);
  bool flush();
  bool idle_task();
  bool write (uint64_t nsec, size_t severity, malc_log_strings const& strs);
}
------------------------------------------------------------------------------*/
template <class uppermost_derivation>
class destination_adapter : public uppermost_derivation {
private:
  friend class wrapper;
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
    uint64_t                nsec,
    unsigned                severity,
    malc_log_strings const* strs
    )
  {
    return try_catch_wrap ([=](){
      return derived (instance).write (nsec, severity, *strs) ?
        bl_mkok() : bl_mkerr (bl_error);
    });
  }
  /*--------------------------------------------------------------------------*/
  static inline uppermost_derivation& derived (void* instance)
  {
    return *static_cast<uppermost_derivation*> (instance);
  }
  /*--------------------------------------------------------------------------*/
  template <class runnable>
  static inline bl_err try_catch_wrap (const runnable& r)
  {
    BL_TRY {
      return r();
    }
    BL_CATCH (...) {
      return bl_mkerr (bl_error);
    }
  }
};
/*----------------------------------------------------------------------------*/
namespace detail {

template <class destination>
struct destination_adapt {
  typedef destination_adapter<destination> type;
};
/*----------------------------------------------------------------------------*/
template <>
struct destination_adapt<array_dst> {
  typedef array_dst type;
};
/*----------------------------------------------------------------------------*/
template <>
struct destination_adapt<file_dst> {
  typedef file_dst type;
};
/*----------------------------------------------------------------------------*/
template <>
struct destination_adapt<stdouterr_dst> {
  typedef stdouterr_dst type;
};
/*----------------------------------------------------------------------------*/

}} // namespace detail // namespace malcpp

#endif

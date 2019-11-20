#ifndef __MALC_HPP__
#define __MALC_HPP__

#include <new>
#include <stdexcept>
#include <type_traits>
#include <cassert>

/* including the common structs in malcpp's namespace */
#include <bl/base/default_allocator.h>

#define MALC_COMMON_NAMESPACED 1

#ifndef MALC_LIBRARY_COMPILATION
  #include <malc/config.h>
#endif
#include <malc/destinations.hpp>
#include <malc/log_macros.h>
#include <malc/impl/c++11.hpp>

struct malc;

/* implementation note: some of the classes here are implemented in a separate
object (cpp) instead of on the headers to require including as little C
definitions as possible into the global namespace, not to hide the
implementation, a this is only wrapping C in C++ .*/

namespace malcpp {

typedef malc_cfg         cfg;
typedef malc_dst_cfg     dst_cfg;
typedef malc_log_strings log_strings;

enum {
  sev_debug    = malc_sev_debug,
  sev_trace    = malc_sev_trace,
  sev_note     = malc_sev_note,
  sev_warning  = malc_sev_warning,
  sev_error    = malc_sev_error,
  sev_critical = malc_sev_critical,
  sev_off      = malc_sev_off,
};

class wrapper;
/*----------------------------------------------------------------------------*/
class exception : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};
/*----------------------------------------------------------------------------*/
namespace detail {
/* created to avoid including C headers for destinations */
class MALC_EXPORT dst_access_untyped {
public:
  /*--------------------------------------------------------------------------*/
  dst_access_untyped() noexcept;
  /*--------------------------------------------------------------------------*/
  bl_err get_cfg (dst_cfg& c) const noexcept;
  /*--------------------------------------------------------------------------*/
  bl_err set_cfg (dst_cfg const& c) const noexcept;
/*----------------------------------------------------------------------------*/
  bool is_valid() const noexcept;
  /*----------------------------------------------------------------------------*/
  malc* owner() const noexcept;
  /*----------------------------------------------------------------------------*/
  bl_u32 id() const noexcept;
  /*--------------------------------------------------------------------------*/
protected:
  friend class malcpp::wrapper;
  /*--------------------------------------------------------------------------*/
  bl_err untyped_try_get (void*& instance) const noexcept;
  /*--------------------------------------------------------------------------*/
  malc*  m_owner;
  bl_u32 m_id;
};

static void throw_if_error (bl_err err)
{
  if (err.own) {
    throw exception (bl_strerror (err));
  }
}
} // namespace detail
/*----------------------------------------------------------------------------*/
/* A class to be returned by the "wrapper" to encapsulate destination
related configuration. */
/*----------------------------------------------------------------------------*/
template <class T>
class dst_access : public detail::dst_access_untyped {
public:
  /*--------------------------------------------------------------------------*/
  /* Gets the underlying instance owned by malc. Don't store the pointer
  returned by this function until you have finished adding destinations, as malc
  internally stores them in a contiguous array (for memory locality) and uses
  realloc.*/
  T* try_get() const noexcept
  {
    void* inst;
    bl_err e = this->untyped_try_get (inst);
    return !e.own ? static_cast<T*> (inst) :nullptr;
  }
};
/*----------------------------------------------------------------------------*/
/* throwing version of "dst_access"*/
/*----------------------------------------------------------------------------*/
template <class T>
class dst_access_throw : protected dst_access<T> {
public:
  dst_access_throw (const dst_access<T>& i)
  {
    this->m_owner = i.owner();
    this->m_id    = i.id();
  }
  /*--------------------------------------------------------------------------*/
  /* Gets the underlying instance owned by malc. Don't store the pointer
  returned by this function until you have finished adding destinations, as malc
  internally stores them in a contiguous array (for memory locality) and uses
  realloc.*/
  T& get() const
  {
    if (!is_valid()) {
      throw exception("\"get\" error on dst_access_throw instance: invalid");
    }
    void* inst;
    detail::throw_if_error (this->untyped_try_get (inst));
    return *static_cast<T*> (inst);
  }
  /*--------------------------------------------------------------------------*/
  dst_cfg get_cfg() const
  {
    dst_cfg r;
    detail::throw_if_error (dst_access<T>::get_cfg (r));
    return r;
  }
  /*--------------------------------------------------------------------------*/
  void set_cfg (dst_cfg const& c) const
  {
    detail::throw_if_error (dst_access<T>::set_cfg (c));
  }
  /*--------------------------------------------------------------------------*/
  using dst_access<T>::owner;
  using dst_access<T>::id;
  using dst_access<T>::is_valid;
  /*--------------------------------------------------------------------------*/
};
/*----------------------------------------------------------------------------*/
/* Very thin wrapper without ownership. Constructors and destructors don't
   invoke malc_create / malc_destroy

   For documentation on the methods here, look at the underlying C header on
   "malc/malc.h".
   */
/*----------------------------------------------------------------------------*/
class MALC_EXPORT wrapper {
public:
  /*--------------------------------------------------------------------------*/
  wrapper (malc* ptr) noexcept;
  /*--------------------------------------------------------------------------*/
  ~wrapper();
  /*--------------------------------------------------------------------------*/
  bl_err get_cfg (cfg& c) const noexcept;
  /*--------------------------------------------------------------------------*/
  bl_err init (cfg const& c) noexcept;
    /*--------------------------------------------------------------------------*/
  bl_err init() noexcept;
  /*--------------------------------------------------------------------------*/
  bl_err flush() noexcept;
  /*--------------------------------------------------------------------------*/
  bl_err terminate (bool dontblock = false) noexcept;
  /*--------------------------------------------------------------------------*/
  bl_err producer_thread_local_init (bl_u32 bytes) noexcept;
  /*--------------------------------------------------------------------------*/
  bl_err run_consume_task (bl_uword timeout_us) noexcept;
  /*--------------------------------------------------------------------------*/
  bl_err add_destination (bl_u32& dest_id, malc_dst const& dst) noexcept;
  /*--------------------------------------------------------------------------*/
  bl_err
    get_destination_instance (void** instance, bl_u32 dest_id) const noexcept;
  /*--------------------------------------------------------------------------*/
  bl_err get_destination_cfg (dst_cfg& c, bl_u32 dest_id) const noexcept;
  /*--------------------------------------------------------------------------*/
  bl_err set_destination_cfg (dst_cfg const& c, bl_u32 dest_id) noexcept;
  /*----------------------------------------------------------------------------
  This function accepts either one of the provided destinations as a template
  parameter:

  - stdouterr_dst
  - file_dst
  - array_dst

  Or a custom implementation with the next interface:

  class custom_dst_interface {
    custom_dst_interface (const bl_alloc_tbl& alloc);
    bool flush();
    bool idle_task();
    bool write (bl_u64 nsec, bl_uword severity, malc_log_strings const& strs);

  This method has to be explicitly given the template paramter, as it is
  malc who will own the instance.

  This method returns a "dst_access<T>"" class which simplifies
  managing the destination configuration compared with using the C interface.

  See the "cpp-wrapper.cpp" and the "cpp-custom-destination-cpp" examples.
  ----------------------------------------------------------------------------*/
  template <class T>
  dst_access<T> add_destination() noexcept
  {
    bl_u32 id;
    dst_access<T> ret;
    malc_dst tbl = detail::destination_adapt<T>::type::get_dst_tbl();
    bl_err err   = add_destination_impl (id, tbl);
    if (!err.own) {
      ret.m_owner = handle();
      ret.m_id = id;
    }
    return ret;
  }
  /*--------------------------------------------------------------------------*/
  malc* handle() const noexcept;
  /*--------------------------------------------------------------------------*/
protected:
  void set_handle (malc* h);
private:
  bl_err add_destination_impl(bl_u32& id, malc_dst const& tbl);
  malc* m_ptr;
};
/*------------------------------------------------------------------------------
  Same as wrapper, but with exceptions enabled and a non error-based interface.
------------------------------------------------------------------------------*/
class throwing_wrapper : private wrapper
{
public:
  using wrapper::wrapper;
  using wrapper::handle;
  /*--------------------------------------------------------------------------*/
  cfg get_cfg() const
  {
    cfg r;
    detail::throw_if_error (wrapper::get_cfg (r));
    return r;
  }
  /*--------------------------------------------------------------------------*/
  void init (cfg const& cfg)
  {
    detail::throw_if_error (wrapper::init (cfg));
  }
  /*--------------------------------------------------------------------------*/
  void init ()
  {
    detail::throw_if_error (wrapper::init());
  }
  /*--------------------------------------------------------------------------*/
  void flush()
  {
    detail::throw_if_error (wrapper::init());
  }
  /*--------------------------------------------------------------------------*/
  void terminate (bool dontblock = false)
  {
    detail::throw_if_error (wrapper::terminate (dontblock));
  }
  /*--------------------------------------------------------------------------*/
  void producer_thread_local_init (bl_u32 bytes)
  {
    detail::throw_if_error (wrapper::producer_thread_local_init (bytes));
  }
  /*--------------------------------------------------------------------------*/
  bool run_consume_task (bl_uword timeout_us)
  {
    bl_err err = wrapper::run_consume_task (timeout_us);
    if (!err.own || err.own == bl_nothing_to_do) {
      return true;
    }
    if (err.own == bl_preconditions) {
      return false;
    }
    detail::throw_if_error (err);
    return false; /* unreachable */
  }
  /*--------------------------------------------------------------------------*/
  bl_u32 add_destination (malc_dst const& dst)
  {
    bl_u32 r;;
    detail::throw_if_error (wrapper::add_destination (r, dst));
    return r;
  }
  /*--------------------------------------------------------------------------*/
  void* get_destination_instance (bl_u32 dest_id) const
  {
    void* r;
    detail::throw_if_error (wrapper::get_destination_instance (&r, dest_id));
    return r;
  }
  /*--------------------------------------------------------------------------*/
  dst_cfg get_destination_cfg (bl_u32 dest_id) const
  {
    dst_cfg r;
    detail::throw_if_error (wrapper::get_destination_cfg (r, dest_id));
    return r;
  }
  /*--------------------------------------------------------------------------*/
  void set_destination_cfg (dst_cfg const& c, bl_u32 dest_id)
  {
    detail::throw_if_error (wrapper::set_destination_cfg (c, dest_id));
  }
  /*--------------------------------------------------------------------------*/
  template <class T>
  dst_access_throw<T> add_destination()
  {
    auto tmp = wrapper::add_destination<T>();
    if (!tmp.is_valid()) {
      throw exception ("unable to create destination");
    }
    return dst_access_throw<T> (tmp);
  }
  /*--------------------------------------------------------------------------*/
protected:
  using wrapper::set_handle;
  /*--------------------------------------------------------------------------*/
};
/*----------------------------------------------------------------------------*/
namespace detail {

template <bool enable_except>
struct wrapper_selector {
  typedef wrapper type;
};
/*----------------------------------------------------------------------------*/
template <>
struct wrapper_selector<true> {
  typedef throwing_wrapper type;
};
/*----------------------------------------------------------------------------*/
template <class impl, bool except, bool autoconstruct>
class construct_enable {};
/*----------------------------------------------------------------------------*/
template <class impl>
class construct_enable<impl, true, false> {
public:
  void construct (bl_alloc_tbl alloc = bl_get_default_alloc())
  {
    static_cast<impl*> (this)->construct_throw_impl (alloc);
  }
};
/*----------------------------------------------------------------------------*/
template <class impl>
class construct_enable<impl, false, false> {
public:
  bl_err construct (bl_alloc_tbl alloc = bl_get_default_alloc()) noexcept
  {
    return static_cast<impl*> (this)->construct_impl (alloc);
  }
};
/*----------------------------------------------------------------------------*/
template <class impl, bool autodestroy>
class destroy_enable {};
/*----------------------------------------------------------------------------*/
template <class impl>
class destroy_enable<impl, false> {
public:
  void destroy() noexcept
  {
    return static_cast<impl*> (this)->destroy_impl();
  }
};

} // namespace detail {
/*------------------------------------------------------------------------------
  The thin wrappers defined above (wrapper/throw_wrapper)+ instance ownership

  It can be configured to throw, have automatic construction and automatic
  destruction. Automatic construction requires exceptions, as constructors
  can't return error codes.
------------------------------------------------------------------------------*/
template<
  bool use_except = true, bool autoconstruct = true, bool autodestroy = true
  >
class MALC_EXPORT malcpp :
  public detail::wrapper_selector<use_except>::type,
  public detail::construct_enable<
    malcpp<use_except, autoconstruct, autodestroy>, use_except, autoconstruct
    >,
  public detail::destroy_enable<
    malcpp<use_except, autoconstruct, autodestroy>, autodestroy
    >
  {
private:
  static_assert(
    !autoconstruct || (autoconstruct && use_except),
    "for this class to be automatically constructed exceptions are necessary"
    );
  typedef typename detail::wrapper_selector<use_except>::type base;
public:
  /*--------------------------------------------------------------------------*/
  static constexpr bool throws = use_except;
  /*--------------------------------------------------------------------------*/
  malcpp (bl_alloc_tbl alloc = bl_get_default_alloc()) : base (nullptr)
  {
    if (autoconstruct && use_except) {
      construct_throw_impl (alloc);
    }
  }
  /*--------------------------------------------------------------------------*/
  ~malcpp() noexcept
  {
    if (autodestroy) {
      destroy_impl();
    }
  }
  /*--------------------------------------------------------------------------*/
  malcpp (malcpp&& mv) noexcept : base (nullptr)
  {
    this->set_handle (mv.handle());
    mv.set_handle (nullptr);
  }
  /*--------------------------------------------------------------------------*/
  malcpp& operator= (malcpp&& mv) noexcept
  {
    if (&mv == this) {
      return *this;
    }
    this->set_handle (mv.handle());
    mv.set_handle (nullptr);
    return *this;
  }
  /*--------------------------------------------------------------------------*/
protected:
  template <class, bool, bool> friend class detail::construct_enable;
  template <class, bool, bool> friend class detail::destroy_enable;
  /*--------------------------------------------------------------------------*/
  void destroy_impl() noexcept;
  /*--------------------------------------------------------------------------*/
  bl_err construct_impl (bl_alloc_tbl alloc) noexcept;
  /*--------------------------------------------------------------------------*/
  void construct_throw_impl (bl_alloc_tbl alloc)
  {
    bl_err e = construct_impl (alloc);
    if (e.own) {
      throw exception(
        this->handle()
        ? "malcpp already constructed"
        : "Unable to create malc instance"
        );
    }
  }
private:
  /*--------------------------------------------------------------------------*/
  malcpp(const malcpp&) = delete;
  malcpp& operator=(const malcpp&) = delete;
  /*--------------------------------------------------------------------------*/
  bl_alloc_tbl* get_alloc_tbl() noexcept;
};
/*----------------------------------------------------------------------------*/

/* functions to use with the logging types */

/*------------------------------------------------------------------------------
Passes a literal to malc. By using this function you tell the logger that
this string will outlive the data logger and doesn't need to be copied, it can
be taken by reference/pointer.

This is mainly intended to be used for passing string literals, but it can be
used (with caution) for non-literal strings that are known to outlive the data
logger unmodified. That's why the function is actually allowing the literal to
decay to char const*.
------------------------------------------------------------------------------*/
static inline detail::malc_lit lit (char const* literal)
{
  bl_assert (literal);
  detail::malc_lit l = { literal };
  return l;
}
/*------------------------------------------------------------------------------
Passes a string by value (deep copy) to malc.
------------------------------------------------------------------------------*/
static inline detail::malc_strcp strcp (char const* str, bl_u16 len)
{
  bl_assert ((str && len) || len == 0);
  detail::malc_strcp s = { str, len };
  return s;
}
/*----------------------------------------------------------------------------*/
static inline detail::malc_strcp strcpl (char const* str)
{
  bl_uword len = strlen (str);
  return strcp (str, (bl_u16) (len < 65536 ? len : 65535));
}
/*------------------------------------------------------------------------------
Passes a memory area by value (deep copy) to malc. It will be printed as hex.
------------------------------------------------------------------------------*/
static inline detail::malc_memcp memcp (void const* mem, bl_u16 size)
{
  bl_assert ((mem && size) || size == 0);
  detail::malc_memcp b = { (bl_u8 const*) mem, size };
  return b;
}
/*------------------------------------------------------------------------------
Passes a string by reference to malc. Needs that you provide a destructor for
this dynamic string as the last parameter on the call (see "logrefdtor"). To be
used to avoid copying big chunks of data.
------------------------------------------------------------------------------*/
static inline detail::malc_strref strref (char const* str, bl_u16 len)
{
  bl_assert ((str && len) || len == 0);
  detail::malc_strref s = { str, len };
  return s;
}
/*----------------------------------------------------------------------------*/
static inline detail::malc_strref strrefl (char const* str)
{
  bl_uword len = strlen (str);
  return strref (str, (bl_u16) (len < 65536 ? len : 65535));
}
/*------------------------------------------------------------------------------
Passes a memory area by reference to malc. Needs that you provide a destructor
for this memory area as the last parameter on the call (see "logrefdtor"), To be
used to avoid copying big chunks of data.
------------------------------------------------------------------------------*/
static inline detail::malc_memref memref (void const* mem, bl_u16 size)
{
  bl_assert ((mem && size) || size == 0);
  detail::malc_memref b = { (bl_u8 const*) mem, size };
  return b;
}
/*------------------------------------------------------------------------------
typedef void (*malc_refdtor_fn)(
  void* context, malc_ref const* refs, bl_uword refs_count
  );

Passed by reference parameter destructor.

This is mandatory to be used as the last parameter on log calls that contain a
parameter passed by reference ("logstrref" or "logmemref"). It does make the
consumer (logger) thread to invoke a callback that can be used to do memory
cleanup/recycling/reference decreasing etc.

This means that blocking on the callback will block the logger consumer thread,
so callbacks should be kept short, ideally doing an bl_atomic reference count
decrement, a deallocation or something similar. Thread safety issues should be
considered too.

Note that in case of error or a filtered out severity the destructor will be run
in-place by the calling thread. This can make the feature unusable for some use
cases, e.g. when the deallocation is costly and blocking and the calling thread
has to progress fast.

The copy by value functions don't add any extra operations on failure or
filtering out, so they can be considered an alternative for when the
deallocation behavior described above is a problem.
------------------------------------------------------------------------------*/
static inline detail::malc_refdtor refdtor(
  malc_refdtor_fn func, void* context
  )
{
  detail::malc_refdtor r = { func, context };
  return r;
}
/*----------------------------------------------------------------------------*/
} // namespace malcpp {

#endif
/*----------------------------------------------------------------------------*/
/* Note: An improved C++ version of "logstrcpy" and "logstrref" are not possible
   only with thin wrappers:

   logstrcpy: The user could send an rvalue, this would break the copying macro,
     as the string would have been destructed before doing the copy operation.
     Support for it would require a transform returning a std::string
     (with move&& and reference overloads), a check for size < 65536 and
     explicit serializer support. It may be done as a separate commit.

   logstrref: Would need explicit support too as one can't get the std::string
     object from its c string pointer (c_str()) */
/*----------------------------------------------------------------------------*/

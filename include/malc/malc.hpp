#ifndef __MALC_HPP__
#define __MALC_HPP__

#include <new>
#include <stdexcept>
#include <type_traits>
#include <cassert>

#include <malc/malc.h>
#include <malc/destinations.hpp>
#include <bl/base/default_allocator.h>

namespace malcpp {

enum {
  sev_debug    = malc_sev_debug,
  sev_trace    = malc_sev_trace,
  sev_note     = malc_sev_note,
  sev_warning  = malc_sev_warning,
  sev_error    = malc_sev_error,
  sev_critical = malc_sev_critical,
  sev_off      = malc_sev_off,
};

typedef malc_cfg         cfg;
typedef malc_dst_cfg     dst_cfg;
typedef malc_log_strings log_strings;

class wrapper;
/*----------------------------------------------------------------------------*/
class exception : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};
/*----------------------------------------------------------------------------*/
namespace detail {

static void throw_if_error (bl_err err)
{
  if (err.bl) {
    throw exception (bl_strerror (err));
  }
}

} // namespace detail
/*----------------------------------------------------------------------------*/
/* A class to be returned by the "wrapper" to encapsulate destination
related configuration. */
/*----------------------------------------------------------------------------*/
template <class T>
class dst_access {
public:
  /*--------------------------------------------------------------------------*/
  dst_access() noexcept
  {
    m_owner = nullptr;
    m_id    = (bl_u32) -1ll;
  }
  /*--------------------------------------------------------------------------*/
  /* Gets the underlying instance owned by malc. Don't store the pointer
  returned by this function until you have finished adding destinations, as malc
  internally stores them in a contiguous array (for memory locality) and uses
  realloc.*/
  T* try_get() const noexcept
  {
    if (!is_valid()) {
      return nullptr;
    }
    void* inst;
    bl_err e = malc_get_destination_instance (owner(), &inst, id());
    return e.bl ? nullptr : static_cast<T*> (inst);
  }
  /*--------------------------------------------------------------------------*/
  bl_err get_cfg (dst_cfg& c) const noexcept
  {
    if (!is_valid()) {
      return bl_mkerr (bl_invalid);
    }
    return malc_get_destination_cfg (owner(), &c, id());
  }
  /*--------------------------------------------------------------------------*/
  bl_err set_cfg (dst_cfg const& c) const noexcept
  {
    if (!is_valid()) {
      return bl_mkerr (bl_invalid);
    }
    return malc_set_destination_cfg (owner(), &c, id());
  }
  /*--------------------------------------------------------------------------*/
  bool is_valid() const noexcept
  {
    return m_owner != nullptr;
  }
  /*--------------------------------------------------------------------------*/
  malc* owner() const noexcept
  {
    return m_owner;
  }
  /*--------------------------------------------------------------------------*/
  bl_u32 id() const noexcept
  {
    return m_id;
  }
  /*--------------------------------------------------------------------------*/
protected:
  friend class wrapper;
  malc*  m_owner;
  bl_u32 m_id;
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
      throw exception("\"get\" error on dst_throw instance");
    }
    void* inst;
    detail::throw_if_error(
      malc_get_destination_instance (owner(), &inst, id())
      );
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
private:
  friend class wrapper;
};
/*----------------------------------------------------------------------------*/
/* Very thin wrapper without ownership. Constructors and destructors don't
   invoke malc_create / malc_destroy

   For documentation on the methods here, look at the underlying C header on
   "malc/malc.h".
   */
/*----------------------------------------------------------------------------*/
class wrapper {
public:
  /*--------------------------------------------------------------------------*/
  wrapper (malc* ptr) noexcept
  {
    m_ptr = ptr;
  }
  /*--------------------------------------------------------------------------*/
  ~wrapper()
  {
    m_ptr = nullptr;
  }
  /*--------------------------------------------------------------------------*/
  bl_err get_cfg (cfg& c) const noexcept
  {
    assert (m_ptr);
    return malc_get_cfg (handle(), &c);
  }
  /*--------------------------------------------------------------------------*/
  bl_err init (cfg const& c) noexcept
  {
    assert (m_ptr);
    return malc_init (handle(), &c);
  }
    /*--------------------------------------------------------------------------*/
  bl_err init() noexcept
  {
    assert (m_ptr);
    return malc_init (handle(), nullptr);
  }
  /*--------------------------------------------------------------------------*/
  bl_err flush() noexcept
  {
    assert (m_ptr);
    return malc_flush (handle());
  }
  /*--------------------------------------------------------------------------*/
  bl_err terminate (bool dontblock = false) noexcept
  {
    assert (m_ptr);
    return malc_terminate (handle(), dontblock);
  }
  /*--------------------------------------------------------------------------*/
  bl_err producer_thread_local_init (bl_u32 bytes) noexcept
  {
    assert (m_ptr);
    return malc_producer_thread_local_init (handle(), bytes);
  }
  /*--------------------------------------------------------------------------*/
  bl_err run_consume_task (bl_uword timeout_us) noexcept
  {
    assert (m_ptr);
    return malc_run_consume_task (handle(), timeout_us);
  }
  /*--------------------------------------------------------------------------*/
  bl_err add_destination (bl_u32& dest_id, malc_dst const& dst) noexcept
  {
    assert (m_ptr);
    return malc_add_destination (handle(), &dest_id, &dst);
  }
  /*--------------------------------------------------------------------------*/
  bl_err
    get_destination_instance (void** instance, bl_u32 dest_id) const noexcept
  {
    assert (m_ptr);
    return malc_get_destination_instance (handle(), instance, dest_id);
  }
  /*--------------------------------------------------------------------------*/
  bl_err get_destination_cfg (dst_cfg& c, bl_u32 dest_id) const noexcept
  {
    assert (m_ptr);
    return malc_get_destination_cfg (handle(), &c, dest_id);
  }
  /*--------------------------------------------------------------------------*/
  bl_err set_destination_cfg (dst_cfg const& c, bl_u32 dest_id) noexcept
  {
    assert (m_ptr);
    return malc_set_destination_cfg (handle(), &c, dest_id);
  }
  /*----------------------------------------------------------------------------
  This function accepts either one of the provided destinations as template
  parameter:

  - stdouterr_ds
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
    bl_err err   = malc_add_destination (handle(), &id, &tbl);
    if (!err.bl) {
      ret.m_owner = handle();
      ret.m_id    = id;
    }
    return ret;
  }
  /*--------------------------------------------------------------------------*/
  malc* handle() const noexcept
  {
    return m_ptr;
  }
  /*--------------------------------------------------------------------------*/
protected:
  void set_handle (malc* h)
  {
    m_ptr = h;
  }
private:
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
    if (!err.bl || err.bl == bl_nothing_to_do) {
      return true;
    }
    if (err.bl == bl_preconditions) {
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
template <class impl, bool autodestruct>
class destruct_enable {};
/*----------------------------------------------------------------------------*/
template <class impl>
class destruct_enable<impl, false> {
public:
  void destruct() noexcept
  {
    return static_cast<impl*> (this)->destruct_impl();
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
  bool use_except = true, bool autoconstruct = true, bool autodestruct = true
  >
class malcpp :
  public detail::wrapper_selector<use_except>::type,
  public detail::construct_enable<
    malcpp<use_except, autoconstruct, autodestruct>, use_except, autoconstruct
    >,
  public detail::destruct_enable<
    malcpp<use_except, autoconstruct, autodestruct>, autodestruct
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
  malcpp (bl_alloc_tbl alloc = bl_get_default_alloc()) : base (nullptr)
  {
    if (autoconstruct && use_except) {
      construct_throw_impl (alloc);
    }
  }
  /*--------------------------------------------------------------------------*/
  ~malcpp() noexcept
  {
    if (autodestruct) {
      destruct_impl();
    }
  }
  /*--------------------------------------------------------------------------*/
  malcpp (malcpp&& mv) noexcept : base (nullptr)
  {
    set_handle (mv.handle());
    mv.set_handle (nullptr);
  }
  /*--------------------------------------------------------------------------*/
  malcpp& operator= (malcpp&& mv) noexcept
  {
    if (&mv == this) {
      return *this;
    }
    set_handle (mv.handle());
    mv.set_handle (nullptr);
    return *this;
  }
  /*--------------------------------------------------------------------------*/
protected:
  template <class, bool, bool> friend class detail::construct_enable;
  template <class, bool, bool> friend class detail::destruct_enable;
  /*--------------------------------------------------------------------------*/
  void destruct_impl() noexcept
  {
    if (!this->handle()) {
      return;
    }
    (void) malc_destroy (this->handle());
    bl_dealloc (get_alloc_tbl(), this->handle());
    this->set_handle (nullptr);
  }
  /*--------------------------------------------------------------------------*/
  bl_err construct_impl (bl_alloc_tbl alloc) noexcept
  {
    if (this->handle()) {
      return bl_mkerr (bl_invalid);
    }
    void* ptr = (malc*) bl_alloc(
      &alloc,
      malc_get_size() + std::alignment_of<bl_alloc_tbl>::value + sizeof alloc
      );
    if (!ptr) {
      return bl_mkerr (bl_alloc);
    }
    this->set_handle ((malc*) ptr);
    *get_alloc_tbl() = alloc;
    bl_err err = malc_create (this->handle(), get_alloc_tbl());
    if (err.bl) {
      bl_dealloc (get_alloc_tbl(), ptr);
      this->set_handle (nullptr);
    }
    return err;
  }
  /*--------------------------------------------------------------------------*/
  void construct_throw_impl (bl_alloc_tbl alloc)
  {
    bl_err e = construct_impl (alloc);
    if (e.bl) {
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
  /* packing an alloc table together with malc's memory */
  bl_alloc_tbl* get_alloc_tbl()
  {
    bl_uword tbl_addr = ((bl_uword) this->handle()) + malc_get_size();
    tbl_addr         += std::alignment_of<bl_alloc_tbl>::value;
    tbl_addr         &= ~(std::alignment_of<bl_alloc_tbl>::value - 1);
    return (bl_alloc_tbl*) tbl_addr;
  }
};
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

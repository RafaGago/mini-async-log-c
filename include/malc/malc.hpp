#ifndef __MALC_HPP__
#define __MALC_HPP__

#include <new>
#include <cassert>
#include <stdexcept>
#include <type_traits>
//#include <string>
#include <exception>

#include <malc/malc.h>
#include <malc/cpp_destination_adapter.hpp>
#include <bl/base/default_allocator.h>


class malc_wrapper;
/*----------------------------------------------------------------------------*/
class malc_exception : public std::runtime_error
{
  /*C++ compiler writers may not be the happiest people on earth... */
  using std::runtime_error::runtime_error;
};
/*----------------------------------------------------------------------------*/
/* A class to be returned by the "malc_wrapper" to encapsulate destination
related configuration. */
/*----------------------------------------------------------------------------*/
template <class T>
class malc_dst_access {
public:
  /*--------------------------------------------------------------------------*/
  malc_dst_access() noexcept
  {
    m_owner = nullptr;
    m_id    = (bl_u32) -1ll;
  }
  /*--------------------------------------------------------------------------*/
  /* Gets the underlying instance owned by malc. Don't store the pointer until
  you have finished adding destinations, as malc is free to move them around in
  memory.*/
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
  bl_err get_cfg (malc_dst_cfg& cfg) const noexcept
  {
    if (!is_valid()) {
      return bl_mkerr (bl_invalid);
    }
    return malc_get_destination_cfg (owner(), &cfg, id());
  }
  /*--------------------------------------------------------------------------*/
  bl_err set_cfg (malc_dst_cfg const& cfg) const noexcept
  {
    if (!is_valid()) {
      return bl_mkerr (bl_invalid);
    }
    return malc_set_destination_cfg (owner(), &cfg, id());
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
  friend class malc_wrapper;
  malc*  m_owner;
  bl_u32 m_id;
};
/*----------------------------------------------------------------------------*/
/* throwing version of "malc_dst_access"*/
/*----------------------------------------------------------------------------*/
template <class T>
class malc_dst_access_throw : protected malc_dst_access<T> {
public:
  malc_dst_access_throw (const malc_dst_access<T>& i)
  {
    this->m_owner = i.owner();
    this->m_id    = i.id();
  }
  /*--------------------------------------------------------------------------*/
  /* Gets the underlying instance owned by malc. Don't store the pointer until
  you have finished adding destinations, as malc is free to move them around in
  memory.*/
  T& get() const
  {
    if (!is_valid()) {
      throw malc_exception("\"get\" error on malc_dst_throw instance");
    }
    void* inst;
    throw_if_error (malc_get_destination_instance (owner(), &inst, id()));
    return *static_cast<T*> (inst);
  }
  /*--------------------------------------------------------------------------*/
  malc_dst_cfg get_cfg() const
  {
    malc_dst_cfg r;
    throw_if_error (malc_dst_access<T>::get_cfg (r));
    return r;
  }
  /*--------------------------------------------------------------------------*/
  void set_cfg (malc_dst_cfg const& cfg) const
  {
    throw_if_error (malc_dst_access<T>::set_cfg (cfg));
  }
  /*--------------------------------------------------------------------------*/
  using malc_dst_access<T>::owner;
  using malc_dst_access<T>::id;
  using malc_dst_access<T>::is_valid;
  /*--------------------------------------------------------------------------*/
private:
  friend class malc_wrapper;

  void throw_if_error (bl_err err) const
  {
    if (err.bl) {
      throw malc_exception (bl_strerror (err));
    }
  }
};
/*----------------------------------------------------------------------------*/
/* Very thin wrapper without ownership. Constructors and destructors don't
   invoke malc_create / malc_destroy

   For documentation on the methods here, look at the underlying C header on
   "malc/malc.h".
   */
/*----------------------------------------------------------------------------*/
class malc_wrapper {
public:
  /*--------------------------------------------------------------------------*/
  malc_wrapper (malc* ptr) noexcept
  {
    m_ptr = ptr;
  }
  /*--------------------------------------------------------------------------*/
  ~malc_wrapper()
  {
    m_ptr = nullptr;
  }
  /*--------------------------------------------------------------------------*/
  bl_err get_cfg (malc_cfg& cfg) const noexcept
  {
    assert (m_ptr);
    return malc_get_cfg (handle(), &cfg);
  }
  /*--------------------------------------------------------------------------*/
  bl_err init (malc_cfg const& cfg) noexcept
  {
    assert (m_ptr);
    return malc_init (handle(), &cfg);
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
  bl_err get_destination_cfg (malc_dst_cfg& cfg, bl_u32 dest_id) const noexcept
  {
    assert (m_ptr);
    return malc_get_destination_cfg (handle(), &cfg, dest_id);
  }
  /*--------------------------------------------------------------------------*/
  bl_err set_destination_cfg (malc_dst_cfg const& cfg, bl_u32 dest_id) noexcept
  {
    assert (m_ptr);
    return malc_set_destination_cfg (handle(), &cfg, dest_id);
  }
  /*----------------------------------------------------------------------------
  This class accepts either one of the provided destinations as template
  parameter:

  - malc_stdouterr_dst_adapter
  - malc_file_dst_adapter
  - malc_array_dst_adapter

  Or a custom implementation with the next interface:

  class custom_dst_interface {
    custom_dst_interface (const bl_alloc_tbl& alloc);
    bool flush();
    bool idle_task();
    bool write (bl_u64 nsec, bl_uword severity, malc_log_strings const& strs);

  This method has to be explicitly given the template paramter, as it is
  malc who will own the instance.

  This method returns a "malc_dst_access<T>"" class which simplifies
  managing the destination configuration compared with using the C interface.

  See the "cpp-wrapper.cpp" and the "cpp-custom-destination-cpp" examples.
  ----------------------------------------------------------------------------*/
  template <class T>
  malc_dst_access<T> add_destination() noexcept
  {
    bl_u32 id;
    malc_dst_access<T> ret;
    malc_dst tbl = malc_cpp_destination_adapt<T>::get_dst_tbl();
    bl_err err = malc_add_destination (handle(), &id, &tbl);
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
  Same as malc_wrapper, but with instance ownership.
------------------------------------------------------------------------------*/
class malc_throwing_wrapper : private malc_wrapper
{
public:
  using malc_wrapper::malc_wrapper;
  using malc_wrapper::handle;
  /*--------------------------------------------------------------------------*/
  malc_cfg get_cfg() const
  {
    malc_cfg r;
    throw_if_error (malc_wrapper::get_cfg (r));
    return r;
  }
  /*--------------------------------------------------------------------------*/
  void init (malc_cfg const& cfg)
  {
    throw_if_error (malc_wrapper::init (cfg));
  }
  /*--------------------------------------------------------------------------*/
  void init ()
  {
    throw_if_error (malc_wrapper::init());
  }
  /*--------------------------------------------------------------------------*/
  void flush()
  {
    throw_if_error (malc_wrapper::init());
  }
  /*--------------------------------------------------------------------------*/
  void terminate (bool dontblock = false)
  {
    throw_if_error (malc_wrapper::terminate (dontblock));
  }
  /*--------------------------------------------------------------------------*/
  void producer_thread_local_init (bl_u32 bytes)
  {
    throw_if_error (malc_wrapper::producer_thread_local_init (bytes));
  }
  /*--------------------------------------------------------------------------*/
  bool run_consume_task (bl_uword timeout_us)
  {
    bl_err err = malc_wrapper::run_consume_task (timeout_us);
    if (!err.bl || err.bl == bl_nothing_to_do) {
      return true;
    }
    if (err.bl == bl_preconditions) {
      return false;
    }
    throw_if_error (err);
    return false; /* unreachable */
  }
  /*--------------------------------------------------------------------------*/
  bl_u32 add_destination (malc_dst const& dst)
  {
    bl_u32 r;;
    throw_if_error (malc_wrapper::add_destination (r, dst));
    return r;
  }
  /*--------------------------------------------------------------------------*/
  void* get_destination_instance (bl_u32 dest_id) const
  {
    void* r;
    throw_if_error (malc_wrapper::get_destination_instance (&r, dest_id));
    return r;
  }
  /*--------------------------------------------------------------------------*/
  malc_dst_cfg get_destination_cfg (bl_u32 dest_id) const
  {
    malc_dst_cfg r;
    throw_if_error (malc_wrapper::get_destination_cfg (r, dest_id));
    return r;
  }
  /*--------------------------------------------------------------------------*/
  void set_destination_cfg (malc_dst_cfg const& cfg, bl_u32 dest_id)
  {
    throw_if_error (malc_wrapper::set_destination_cfg (cfg, dest_id));
  }
  /*--------------------------------------------------------------------------*/
  template <class T>
  malc_dst_access_throw<T> add_destination()
  {
    auto tmp = malc_wrapper::add_destination<T>();
    if (!tmp.is_valid()) {
      throw malc_exception ("unable to create destination");
    }
    return malc_dst_access_throw<T> (tmp);
  }
  /*--------------------------------------------------------------------------*/
protected:
  using malc_wrapper::set_handle;
  /*--------------------------------------------------------------------------*/
private:
  void throw_if_error (bl_err err) const
  {
    if (err.bl) {
      throw malc_exception (bl_strerror (err));
    }
  }
  /*--------------------------------------------------------------------------*/
};
/*----------------------------------------------------------------------------*/
template <bool throws>
struct malc_wrapper_selector {
  typedef malc_wrapper type;
};
/*----------------------------------------------------------------------------*/
template <>
struct malc_wrapper_selector<true> {
  typedef malc_throwing_wrapper type;
};
/*------------------------------------------------------------------------------
  Same as malc_wrapper, but with instance ownership.
------------------------------------------------------------------------------*/
template <bool do_construct, bool do_destruct, bool throw_ops>
class malc_owner_impl : public malc_wrapper_selector<throw_ops>::type {
private:
  typedef typename malc_wrapper_selector<throw_ops>::type base;
public:
  class throw_tag {};
  /*--------------------------------------------------------------------------*/
  malc_owner_impl() : base (nullptr)
  {
    if (do_construct) {
      construct (throw_tag());
    }
  }
  /*--------------------------------------------------------------------------*/
  malc_owner_impl (malc_owner_impl&& mv) noexcept : base (nullptr)
  {
    this->set_handle (mv.handle());
    mv.set_handle (nullptr);
  }
  /*--------------------------------------------------------------------------*/
  malc_owner_impl& operator= (malc_owner_impl&& mv) noexcept
  {
    if (&mv == this) {
      return *this;
    }
    this->set_handle (mv.handle());
    mv.set_handle (nullptr);
    return *this;
  }
  /*--------------------------------------------------------------------------*/
  ~malc_owner_impl() noexcept
  {
    if (do_destruct) {
      destruct();
    }
  }
  /*--------------------------------------------------------------------------*/
  bl_err construct() noexcept
  {
    if (this->handle()) {
      return bl_mkerr (bl_invalid);
    }
    bl_alloc_tbl alloc;
    alloc = bl_get_default_alloc();
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
  void construct (throw_tag d)
  {
    bl_err e = construct();
    if (e.bl) {
      throw malc_exception(
        this->handle()
        ? "malc_owner already constructed"
        : "Unable to create malc instance"
        );
    }
  }
  /*--------------------------------------------------------------------------*/
  void destruct() noexcept
  {
    if (!this->handle()) {
      return;
    }
    (void) malc_destroy (this->handle());
    bl_dealloc (get_alloc_tbl(), this->handle());
    this->set_handle (nullptr);
  }
  /*--------------------------------------------------------------------------*/
private:
  /*--------------------------------------------------------------------------*/
  malc_owner_impl(const malc_owner_impl&) = delete;
  malc_owner_impl& operator=(const malc_owner_impl&) = delete;
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
typedef malc_owner_impl<true, true, false>   malc_owner;
typedef malc_owner_impl<false, true, false>  malc_owner_no_construct;
typedef malc_owner_impl<false, false, false> malc_owner_no_construct_destruct;
typedef malc_owner_impl<true, true, true>    malc_owner_throw;
typedef malc_owner_impl<false, true, true>   malc_owner_no_construct_throw;
typedef malc_owner_impl<false, false, true>
  malc_owner_no_construct_destruct_throw;

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

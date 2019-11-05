#ifndef __MALC_HPP__
#define __MALC_HPP__

#include <new>
#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <string>

#include <malc/malc.h>
#include <bl/base/default_allocator.h>

class malc_wrapper;
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
private:
  friend class malc_wrapper;
  malc*  m_owner;
  bl_u32 m_id;
};
/*----------------------------------------------------------------------------*/
/* Very thin wrapper without ownership. Constructors and destructors don't
   invoke malc_create / malc_destroy */
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
  bl_err flush() noexcept
  {
    assert (m_ptr);
    return malc_flush (handle());
  }
  /*--------------------------------------------------------------------------*/
  bl_err terminate (bool is_consume_task_thread) noexcept
  {
    assert (m_ptr);
    return malc_terminate (handle(), is_consume_task_thread);
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
  /*--------------------------------------------------------------------------*/
  template <class T>
  malc_dst_access<T> add_destination() noexcept
  {
    bl_u32 id;
    malc_dst_access<T> ret;
    malc_dst tbl = T::get_dst_tbl();
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
  malc* m_ptr;
};
/*------------------------------------------------------------------------------
  Same as malc_wrapper, but with instance ownership.
------------------------------------------------------------------------------*/
template <bool do_construct, bool do_destruct>
class malc_owner_impl : public malc_wrapper {
public:
  class throw_tag {};
  /*--------------------------------------------------------------------------*/
  malc_owner_impl() : malc_wrapper (nullptr)
  {
    if (do_construct) {
      construct (throw_tag());
    }
  }
  /*--------------------------------------------------------------------------*/
  malc_owner_impl (malc_owner_impl&& mv) noexcept : malc_wrapper (nullptr)
  {
    m_ptr    = mv.m_ptr;
    mv.m_ptr = nullptr;
  }
  /*--------------------------------------------------------------------------*/
  malc_owner_impl& operator= (malc_owner_impl&& mv) noexcept
  {
    if (&mv == this) {
        return *this;
    }
    m_ptr    = mv.m_ptr;
    mv.m_ptr = nullptr;
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
    if (m_ptr) {
      return bl_mkerr (bl_invalid);
    }
    bl_alloc_tbl alloc;
    alloc = bl_get_default_alloc();
    m_ptr = (malc*) bl_alloc(
      &alloc,
      malc_get_size() + std::alignment_of<bl_alloc_tbl>::value + sizeof alloc
      );
    if (!m_ptr) {
      return bl_mkerr (bl_alloc);
    }
    *get_alloc_tbl() = alloc;
    bl_err err = malc_create (handle(), get_alloc_tbl());
    if (err.bl) {
      bl_dealloc (get_alloc_tbl(), m_ptr);
      m_ptr = nullptr;
    }
    return err;
  }
  /*--------------------------------------------------------------------------*/
  void construct (throw_tag d)
  {
    bl_err e = construct();
    if (e.bl) {
      throw std::runtime_error(
        m_ptr
        ? "malc_owner already constructed"
        : "Unable to create malc instance"
        );
    }
  }
  /*--------------------------------------------------------------------------*/
  void destruct() noexcept
  {
    if (!m_ptr) {
      return;
    }
    (void) malc_destroy (handle());
    bl_dealloc (get_alloc_tbl(), m_ptr);
    m_ptr = nullptr;
  }
  /*--------------------------------------------------------------------------*/
private:
  /*--------------------------------------------------------------------------*/
  malc_owner_impl(const malc_owner_impl&) = delete;
  malc_owner_impl& operator=(const malc_owner_impl&) = delete;
  /*--------------------------------------------------------------------------*/
  bl_alloc_tbl* get_alloc_tbl()
  {
    bl_uword tbl_addr = ((bl_uword) m_ptr) + malc_get_size();
    tbl_addr      += std::alignment_of<bl_alloc_tbl>::value;
    tbl_addr      &= ~(std::alignment_of<bl_alloc_tbl>::value - 1);
    return (bl_alloc_tbl*) tbl_addr;
  }
};
/*----------------------------------------------------------------------------*/
typedef malc_owner_impl<true, true>   malc_owner;
typedef malc_owner_impl<false, true>  malc_owner_no_construct;
typedef malc_owner_impl<false, false> malc_owner_no_construct_destruct;

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

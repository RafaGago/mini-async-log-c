#ifndef __MALC_HPP__
#define __MALC_HPP__

#include <new>
#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <string>

#include <malc/malc.h>
#include <bl/base/default_allocator.h>

/*----------------------------------------------------------------------------*/
/* Note: logstrcpy and logstrref are not possible only with thin wrappers:

   logstrcpy: Could send an rvalue, this would break the copying macro, as the
     string would have been destructed before doing the copy operation. Support
     for it would require a transform returning a std::string (with move&& and
     reference overloads) a, check for size < 65536 and explicit serializer
     support. It may be done as a seprate commit.

   logstrref: Would need explicit support too as one can't get the std::string
     object from its c string pointer (c_str()) */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Very thin wrapper without ownership. Constructors and destructors don't
   invoke malc_create / malc_destroy */
/*----------------------------------------------------------------------------*/
class malc_wrapper {
public:
  /*--------------------------------------------------------------------------*/
  malc_wrapper (malc* ptr)
  {
    m_ptr = ptr;
  }
  /*--------------------------------------------------------------------------*/
  ~malc_wrapper()
  {
    m_ptr = nullptr;
  }
  /*--------------------------------------------------------------------------*/
  bl_err get_cfg (malc_cfg& cfg) const
  {
    assert (m_ptr);
    return malc_get_cfg (handle(), &cfg);
  }
  /*--------------------------------------------------------------------------*/
  bl_err init (malc_cfg const& cfg)
  {
    assert (m_ptr);
    return malc_init (handle(), &cfg);
  }
  /*--------------------------------------------------------------------------*/
  bl_err flush()
  {
    assert (m_ptr);
    return malc_flush (handle());
  }
  /*--------------------------------------------------------------------------*/
  bl_err terminate (bool is_consume_task_thread)
  {
    assert (m_ptr);
    return malc_terminate (handle(), is_consume_task_thread);
  }
  /*--------------------------------------------------------------------------*/
  bl_err producer_thread_local_init (u32 bytes)
  {
    assert (m_ptr);
    return malc_producer_thread_local_init (handle(), bytes);
  }
  /*--------------------------------------------------------------------------*/
  bl_err run_consume_task (uword timeout_us)
  {
    assert (m_ptr);
    return malc_run_consume_task (handle(), timeout_us);
  }
  /*--------------------------------------------------------------------------*/
  bl_err add_destination (u32& dest_id, malc_dst const& dst)
  {
    assert (m_ptr);
    return malc_add_destination (handle(), &dest_id, &dst);
  }
  /*--------------------------------------------------------------------------*/
  bl_err get_destination_instance (void** instance, u32 dest_id) const
  {
    assert (m_ptr);
    return malc_get_destination_instance (handle(), instance, dest_id);
  }
  /*--------------------------------------------------------------------------*/
  bl_err get_destination_cfg (malc_dst_cfg& cfg, u32 dest_id) const
  {
    assert (m_ptr);
    return malc_get_destination_cfg (handle(), &cfg, dest_id);
  }
  /*--------------------------------------------------------------------------*/
  bl_err set_destination_cfg (malc_dst_cfg const& cfg, u32 dest_id)
  {
    assert (m_ptr);
    return malc_set_destination_cfg (handle(), &cfg, dest_id);
  }
  /*--------------------------------------------------------------------------*/
  malc* handle() const
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
  /*--------------------------------------------------------------------------*/
  malc_owner_impl() : malc_wrapper (nullptr)
  {
    if (do_construct) {
      construct (true);
    }
  }
  /*--------------------------------------------------------------------------*/
  malc_owner_impl (malc_owner_impl&& mv) : malc_wrapper (nullptr)
  {
    m_ptr    = mv.m_ptr;
    mv.m_ptr = nullptr;
  }
  /*--------------------------------------------------------------------------*/
  malc_owner_impl& operator= (malc_owner_impl&& mv)
  {
    if (&mv == this) {
        return *this;
    }
    m_ptr    = mv.m_ptr;
    mv.m_ptr = nullptr;
    return *this;
  }
  /*--------------------------------------------------------------------------*/
  ~malc_owner_impl()
  {
    if (do_destruct) {
      destruct();
    }
  }
  /*--------------------------------------------------------------------------*/
  bl_err construct (bool do_throw = false)
  {
    if (m_ptr) {
      if (do_throw) {
        return bl_invalid;
      }
      else {
        throw std::runtime_error ("malc_owner already constructed");
      }
    }
    alloc_tbl alloc;
    alloc = get_default_alloc();
    m_ptr = (malc*) bl_alloc(
      &alloc,
      malc_get_size() + std::alignment_of<alloc_tbl>::value + sizeof alloc
      );
    if (!m_ptr) {
      if (do_throw) {
        throw std::bad_alloc();
      }
      else {
        return bl_alloc;
      }
    }
    *get_alloc_tbl() = alloc;
    bl_err err = malc_create (handle(), get_alloc_tbl());
    if (err) {
      bl_dealloc (get_alloc_tbl(), m_ptr);
      m_ptr = nullptr;
      if (do_throw) {
        throw std::runtime_error ("Unable to create malc instance");
      }
    }
    return err;
  }
  /*--------------------------------------------------------------------------*/
  void destruct()
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
  alloc_tbl* get_alloc_tbl()
  {
    uword tbl_addr = ((uword) m_ptr) + malc_get_size();
    tbl_addr      += std::alignment_of<alloc_tbl>::value;
    tbl_addr      &= ~(std::alignment_of<alloc_tbl>::value - 1);
    return (alloc_tbl*) tbl_addr;
  }
};
/*----------------------------------------------------------------------------*/
typedef malc_owner_impl<true, true>   malc_owner;
typedef malc_owner_impl<false, true>  malc_owner_no_construct;
typedef malc_owner_impl<false, false> malc_owner_no_construct_destruct;

#endif
